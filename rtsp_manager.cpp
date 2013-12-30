#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "message_types.h"
#include "global_config.h"
#include "global_config.h"
#include "global_net_func.h"
#include "gloghelper.h"
#include "rtsp_manager.h"
#include "rtsp_net_func.h"
using namespace std;

static void handle_connect_rtsp_command(struct bufferevent *bev)
{
  evutil_socket_t fd = bufferevent_getfd(bev);
  general_context *context = NULL;
  string ip;
  uint32_t port = 0;
  if (context = rtsp_manager::get_instance()->get(fd)) {
    ip = context->m_conn_ip;
    port = context->m_conn_port;
  }
  int rtsp_cseq = 1;
  char resp_buffer[MAX_BUFF_SIZE];
  // step 1: options
  rtsp_write(fd, "OPTIONS", "rtsp://10.101.10.189/v05", NULL, rtsp_cseq++);
  rtsp_read(fd, resp_buffer, MAX_BUFF_SIZE);
  // step 2: describe
  char content_length[128], range[128];
  rtsp_write(fd, "DESCRIBE", "rtsp://10.101.10.189/v05", "Accept: application/sdp\r\n", rtsp_cseq++);
  rtsp_read(fd, resp_buffer, MAX_BUFF_SIZE);
  if (find_value((char*)"Content-Length:", resp_buffer, content_length)) {
    rtsp_read(fd, resp_buffer, MAX_BUFF_SIZE);
  }
  find_value("range:", resp_buffer, range);
  // step 3: parsing track information
  int video_track_id = -1, audio_track_id = -2;
  char video_track[512], audio_track[512];
  memset(video_track, 0, sizeof(video_track));
  memset(audio_track, 0, sizeof(audio_track));
  const char *pointer = NULL;
  pointer = strstr(resp_buffer, "m=video");
  if (pointer) {
    pointer = strstr(pointer, "a=control:");
    if (pointer) {
      sscanf(pointer, "a=control:%s", video_track);
    }
  }
  pointer = strstr(resp_buffer, "m=audio");
  if (pointer) {
    pointer = strstr(pointer, "a=control:");
    if (pointer) {
      sscanf(pointer, "a=control:%s", audio_track);
    }
  }
  if (strlen(video_track) > 0) {
    if (pointer = strstr(video_track, "trackID=")) {
      sscanf(pointer, "trackID=%d", &video_track_id);
    }
  }
  if (strlen(audio_track) > 0) {
    if (pointer = strstr(audio_track, "trackID=")) {
      sscanf(pointer, "trackID=%d", &audio_track_id);
    }
  }
  LOG(INFO)<<video_track<<"\t"<<video_track_id<<endl;
  LOG(INFO)<<audio_track<<"\t"<<audio_track_id<<endl;
  // step 4: send setup command
  char setup_message[256], session[128];
  snprintf(setup_message, sizeof(setup_message), "Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n");
  rtsp_write(fd, "SETUP", video_track, setup_message, rtsp_cseq++);
  rtsp_read(fd, resp_buffer, MAX_BUFF_SIZE);
  if (find_value((char*)"Session:", resp_buffer, session)) {
    LOG(INFO)<<"session: "<<session<<endl;
    snprintf(setup_message, sizeof(setup_message), "Transport: RTP/AVP/TCP;unicast;interleaved=2-3\r\nSession:%s\r\n", session);
    rtsp_write(fd, "SETUP", audio_track, setup_message, rtsp_cseq++);
    rtsp_read(fd, resp_buffer, MAX_BUFF_SIZE);
  }
  // step 5: send play command
  char play_message[256];
  snprintf(play_message, sizeof(play_message), "Session:%s\r\nRange:%s\r\n", session, range);
  rtsp_write(fd, "PLAY", "rtsp://10.101.10.189/v05", play_message, rtsp_cseq++);
}

void rtsp_accept_cb(evutil_socket_t listener, short event, void *arg)
{
  struct event_base *base = (struct event_base *)arg;
  evutil_socket_t fd;
  struct sockaddr_in sin;
  socklen_t slen = sizeof(sin);
  fd = accept(listener, (struct sockaddr *)&sin, &slen);
  if (fd < 0) {
    LOG(ERROR)<<"accept error: "<<strerror(errno)<<", fd = "<<fd<<endl;
    return;
  }
  if (fd > FD_SETSIZE) {
    LOG(ERROR)<<"accept error: "<<strerror(errno)<<", fd > FD_SETSIZE ["<<fd<<" > "<<FD_SETSIZE<<"]"<<endl;
    return;
  }

  string ip = inet_ntoa(sin.sin_addr);
  uint32_t port = ntohs(sin.sin_port);
  LOG(INFO)<<"["<<ip<<":"<<port<<" --> localhost.fd="<<fd<<"]"<<" accept connection"<<endl;

  general_context context;
  context.m_conn_ip = ip;
  context.m_conn_port = port;
  rtsp_manager::get_instance()->add(fd, context);

  struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
  bufferevent_setcb(bev, rtsp_read_cb, NULL, rtsp_error_cb, arg);
  bufferevent_enable(bev, EV_READ|EV_WRITE|EV_PERSIST);
}

void rtsp_write_cb(struct bufferevent *bev, void *arg) {}

void rtsp_error_cb(struct bufferevent *bev, short event, void *arg)
{
  evutil_socket_t fd = bufferevent_getfd(bev);
  general_context *context = NULL;
  string ip;
  uint32_t port = 0;
  if (context = rtsp_manager::get_instance()->get(fd)) {
    ip = context->m_conn_ip;
    port = context->m_conn_port;
  }

  if (event & BEV_EVENT_TIMEOUT) {
    //if bufferevent_set_timeouts() called
    LOG(ERROR)<<"["<<ip<<":"<<port<<" --> localhost.fd="<<fd<<"]"<<" read/write time out"<<endl;
  }
  else if (event & BEV_EVENT_EOF) {
    LOG(ERROR)<<"["<<ip<<":"<<port<<" --> localhost.fd="<<fd<<"]"<<" connection closed"<<endl;
  }
  else if (event & BEV_EVENT_ERROR) {
    LOG(ERROR)<<"["<<ip<<":"<<port<<" --> localhost.fd="<<fd<<"]"<<" some other error"<<endl;
  }
  bufferevent_free(bev);
}

void rtsp_read_cb(struct bufferevent *bev, void *arg)
{
  char recv_buffer[MAX_BUFF_SIZE];
  IoctlMsg *recv_message = (IoctlMsg*)recv_buffer;
  int recv_len = 0;
  memset(recv_buffer, 0, sizeof(recv_buffer));

  evutil_socket_t fd = bufferevent_getfd(bev);
  general_context *context = NULL;
  string ip;
  uint32_t port = 0;
  if (context = rtsp_manager::get_instance()->get(fd)) {
    ip = context->m_conn_ip;
    port = context->m_conn_port;
  }

  recv_len = read_event_buffer(bev, recv_buffer, sizeof(recv_buffer));
  LOG(INFO)<<"["<<ip<<":"<<port<<" --> localhost.fd="<<fd<<"]"<<" read "<<recv_len<<" bytes message 0X"<<hex<<recv_message->ioctlCmd<<endl;

  switch (recv_message->ioctlCmd) {
  case IOCTL_RTSP_READY:
    handle_connect_rtsp_command(bev);
    break;
  case IOCTL_GET_PARAMETER_RESP:
    //handle_get_parameter_response(bev, recv_buffer, recv_len);
    //handle_set_rtspserver_command(bev);
    break;
  default:
    //handle_unregcognized_command(bev);
    break;
  }
}
