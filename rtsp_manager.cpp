#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "message_types.h"
#include "global_config.h"
#include "global_net_func.h"
#include "gloghelper.h"
#include "rtsp_manager.h"
#include "rtsp_net_func.h"
using namespace std;

static void handle_connect_rtsp_command(struct bufferevent *bev)
{
  evutil_socket_t fd = bufferevent_getfd(bev);
  general_context *context = rtsp_manager::get_instance()->get(fd);
  if (!context) return;
  string ip = context->m_conn_ip;
  uint32_t port = context->m_conn_port;

  const char *data = context->m_buffer_queue->top();
  int current_size = context->m_buffer_queue->size();
  int header_size = sizeof(IoctlMsg) +sizeof(IoctlMsgNormal);
  int rtsp_cseq = 0;
  for (int i = current_size-1; i >= 0; i--) {
    const char *pointer = strstr(data+i, "CSeq: ");
    if (pointer) {
      sscanf(pointer, "CSeq: %d", &rtsp_cseq);
      break;
    }
  }

  if (0 == rtsp_cseq) {
    rtsp_write(fd, "OPTIONS", "rtsp://10.101.10.189/v05", NULL, rtsp_cseq+1);
  }
  if (1 == rtsp_cseq) {
    rtsp_write(fd, "DESCRIBE", "rtsp://10.101.10.189/v05", "Accept: application/sdp\r\n", rtsp_cseq+1);
  }
  if (2 == rtsp_cseq && strstr(data+header_size, "a=control")) {
    char result[256];
    memset(result, 0, sizeof(result));
    if (find_value("range:", data+header_size, result)) {
      context->m_stream_range = result;
    }
    memset(result, 0, sizeof(result));
    const char *pointer = strstr(data+header_size, "m=video");
    if (find_value("a=control:", pointer, result)) {
      context->m_video_track = result;
      if (pointer = strstr(result, "trackID=")) {
        sscanf(pointer, "trackID=%d", &(context->m_video_track_id));
      }
    }
    memset(result, 0, sizeof(result));
    pointer = strstr(data+header_size, "m=audio");
    if (find_value("a=control:", pointer, result)) {
      context->m_audio_track = result;
      if (pointer = strstr(result, "trackID=")) {
        sscanf(pointer, "trackID=%d", &(context->m_audio_track_id));
      }
    }
    if (context->m_video_track.length() > 0) {
      rtsp_write(fd, "SETUP", context->m_video_track.c_str(), "Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n", rtsp_cseq+1);
    }
  }
  if (3 == rtsp_cseq && strstr(data+header_size, "Session:")) {
    char result[256];
    memset(result, 0, sizeof(result));
    if (find_value("Session: ", data+header_size, result)) {
      context->m_stream_session = result;
    }
    if (context->m_stream_session.length() > 0 && context->m_stream_range.length() > 0) {
      snprintf(result, sizeof(result), "Session:%s\r\nRange:%s\r\n", context->m_stream_session.c_str(), context->m_stream_range.c_str());
      rtsp_write(fd, "PLAY", "rtsp://10.101.10.189/v05", result, rtsp_cseq+1);
      context->m_buffer_queue->clear();
    }
  }
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
  int recv_len = 0;
  memset(recv_buffer, 0, sizeof(recv_buffer));
  evutil_socket_t fd = bufferevent_getfd(bev);
  general_context *context = rtsp_manager::get_instance()->get(fd);
  if (!context) return;

  recv_len = read_event_buffer(bev, recv_buffer, sizeof(recv_buffer));

  context->m_buffer_queue->push(recv_buffer, recv_len);
  const char *data = context->m_buffer_queue->top();
  IoctlMsg *recv_message = (IoctlMsg*)data;

LOG(INFO)<<"get "<<recv_len<<" bytes"<<endl;

  switch (recv_message->ioctlCmd) {
  case IOCTL_RTSP_READY:
    handle_connect_rtsp_command(bev);
    break;
  default:
    LOG(INFO)<<"hello world"<<endl;
    break;
  }
}
