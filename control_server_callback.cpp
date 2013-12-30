#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "message_types.h"
#include "global_config.h"
#include "global_net_func.h"
#include "gloghelper.h"
#include "control_manager.h"
#include "control_server_callback.h"
using namespace std;


static void handle_get_parameter_command(struct bufferevent *bev, const char *_buf, int _len)
{
  evutil_socket_t fd = bufferevent_getfd(bev);
  general_context *context = NULL;
  string ip;
  uint32_t port = 0;
  if (context = control_manager::get_instance()->get(fd)) {
    ip = context->m_conn_ip;
    port = context->m_conn_port;
  }

  char send_buffer[MAX_BUFF_SIZE];
  IoctlMsg *send_message = (IoctlMsg*)send_buffer;
  memset(send_buffer, 0, sizeof(send_buffer));
  int send_len = 0;

  send_message->magic[0] = '$';
  send_message->magic[1] = '\0';
  send_message->ioctlCmd = IOCTL_GET_PARAMETER_REQ;
  IoctlMsgGetParameterReq *resp = (IoctlMsgGetParameterReq*)send_message->data;

  const char* param_request = "group=all";
  strcpy(resp->result, param_request);
	resp->total = resp->count = strlen (param_request);
	resp->index = 0;
	resp->endflag = 1;
	send_len = sizeof(IoctlMsg)+sizeof(IoctlMsgGetParameterReq)+strlen(param_request);
  send_message->size = send_len - 4;

  write_event_buffer(bev, send_buffer, send_len);
}

static void handle_get_parameter_response(struct bufferevent *bev, char *_buf, int _len)
{
  evutil_socket_t fd = bufferevent_getfd(bev);
  general_context *context = NULL;
  string ip;
  uint32_t port = 0;
  if (context = control_manager::get_instance()->get(fd)) {
    ip = context->m_conn_ip;
    port = context->m_conn_port;
  }

  int buffer_cur_size = 0;
  char buffer[64*1024];
  do {
    bool is_full = false;
    int current_parameter_size = 0, total_parameter_size = 0;
    memcpy(buffer+buffer_cur_size, _buf, _len);
    buffer_cur_size += _len;
    int last_index = 0, last_length = 0;
    for (int index = 0; index < buffer_cur_size-1; index++) {
      if (buffer[index] == '$' && buffer[index+1] == '\0') {
        if (index - last_index < last_length) continue;
        IoctlMsgGetParameterResp *response = (IoctlMsgGetParameterResp*)(buffer+index+sizeof(IoctlMsg));
        if (0 == total_parameter_size) total_parameter_size = response->total;
        current_parameter_size += response->count;
        last_index = index, last_length = response->count;
        if (current_parameter_size >= total_parameter_size || response->endflag) {
          LOG(ERROR)<<"["<<ip<<":"<<port<<" --> localhost.fd="<<fd<<"]"<<" get parameter "<<current_parameter_size<<" bytes"<<endl;
          is_full = true;
          break;
        }
      }
    }
    if (is_full) {
      break;
    }
    _len = read(fd, _buf ,MAX_BUFF_SIZE);
  } while (1);

  char parameter[64*1024];
  int current_parameter_size = 0;
  memset(parameter, 0 ,sizeof(parameter));
  for (int index = 0; index < buffer_cur_size; index++) {
    if (buffer[index] == '$' && buffer[index+1] == '\0') {
      IoctlMsgGetParameterResp *response = (IoctlMsgGetParameterResp*)(buffer+index+sizeof(IoctlMsg));
      memcpy(parameter+current_parameter_size, response->result, response->count);
      current_parameter_size += response->count;
      index = index + sizeof(IoctlMsg) + sizeof(IoctlMsgGetParameterResp) + response->count - 1;
    }
  }

  char* pstr = NULL;
	if (pstr = strstr(parameter, "network.hwaddress")) {
	  char mac[24];
	  sscanf(pstr, "network.hwaddress='%s';", mac);
	  mac[17] = 0;
	  context->m_camera_mac = mac;
    LOG(INFO)<<mac<<endl;
	}
}

static void handle_set_rtspserver_command(struct bufferevent *bev)
{
  evutil_socket_t fd = bufferevent_getfd(bev);
  general_context *context = NULL;
  string ip;
  uint32_t port = 0;
  if (context = control_manager::get_instance()->get(fd)) {
    ip = context->m_conn_ip;
    port = context->m_conn_port;
  }

  char send_buffer[MAX_BUFF_SIZE];
  IoctlMsg *send_message = (IoctlMsg*)send_buffer;
  memset(send_buffer, 0, sizeof(send_buffer));
  int send_len = sizeof(IoctlMsg)+sizeof(IoctlMsgSetRtspServerReq);

  send_message->magic[0] = '$';
  send_message->magic[1] = '\0';
  send_message->ioctlCmd = IOCTL_SET_RTSPSERVER_REQ;
  IoctlMsgSetRtspServerReq *resp = (IoctlMsgSetRtspServerReq*)send_message->data;
  strcpy(resp->server.name, g_local_address);
  resp->server.port = g_rtsp_server_port;
  resp->server.type = resp->server.err = 0;
  send_message->size = send_len - 4;

  write_event_buffer(bev, send_buffer, send_len);
}

static void handle_unregcognized_command(struct bufferevent *bev)
{
  evutil_socket_t fd = bufferevent_getfd(bev);
  if (!control_manager::get_instance()->get(fd)) return;
}

void control_accept_cb(evutil_socket_t listener, short event, void *arg)
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
  control_manager::get_instance()->add(fd, context);

  struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
  bufferevent_setcb(bev, control_read_cb, NULL, control_error_cb, arg);
  bufferevent_enable(bev, EV_READ|EV_WRITE|EV_PERSIST);
}

void control_write_cb(struct bufferevent *bev, void *arg) {}

void control_error_cb(struct bufferevent *bev, short event, void *arg)
{
  evutil_socket_t fd = bufferevent_getfd(bev);
  general_context *context = NULL;
  string ip;
  uint32_t port = 0;
  if (context = control_manager::get_instance()->get(fd)) {
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

void control_read_cb(struct bufferevent *bev, void *arg)
{
  char recv_buffer[MAX_BUFF_SIZE];
  IoctlMsg *recv_message = (IoctlMsg*)recv_buffer;
  int recv_len = 0;
  memset(recv_buffer, 0, sizeof(recv_buffer));

  evutil_socket_t fd = bufferevent_getfd(bev);
  general_context *context = NULL;
  string ip;
  uint32_t port = 0;
  if (context = control_manager::get_instance()->get(fd)) {
    ip = context->m_conn_ip;
    port = context->m_conn_port;
  }

  recv_len = read_event_buffer(bev, recv_buffer, sizeof(recv_buffer));
  LOG(INFO)<<"["<<ip<<":"<<port<<" --> localhost.fd="<<fd<<"]"<<" read "<<recv_len<<" bytes message 0X"<<hex<<recv_message->ioctlCmd<<endl;

  switch (recv_message->ioctlCmd) {
  case IOCTL_REQUEST_TEST_00:
    //handle_keep_alive_command(bev);
    handle_get_parameter_command(bev, recv_buffer, recv_len);
    break;
  case IOCTL_GET_PARAMETER_RESP:
    handle_get_parameter_response(bev, recv_buffer, recv_len);
    handle_set_rtspserver_command(bev);
    break;
  default:
    handle_unregcognized_command(bev);
    break;
  }
}
