#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "message_types.h"
#include "global_config.h"
#include "global_net_func.h"
#include "gloghelper.h"
#include "control_manager.h"
using namespace std;

static void handle_keep_alive_command(struct bufferevent *bev)
{
  evutil_socket_t fd = bufferevent_getfd(bev);
  general_context *context = control_manager::get_instance()->get(fd);
  if (!context) return;
  string ip = context->m_conn_ip;
  uint32_t port = context->m_conn_port;

  const char *data = context->m_buffer_queue->top();
  int current_size = context->m_buffer_queue->size();

  char recv_buffer[MAX_BUFF_SIZE];
  int recv_len = 0;
  memset(recv_buffer, 0, sizeof(recv_buffer));

  IoctlMsg *recv_message = (IoctlMsg*)data;
  int message_full_size = 4 + recv_message->size;
  if (current_size < message_full_size) {
    return;
  }
  context->m_buffer_queue->pop(recv_buffer, message_full_size);
  recv_len = message_full_size;
}

static void handle_get_parameter_command(struct bufferevent *bev)
{
  evutil_socket_t fd = bufferevent_getfd(bev);
  general_context *context = control_manager::get_instance()->get(fd);
  if (!context) return;
  string ip = context->m_conn_ip;
  uint32_t port = context->m_conn_port;

  const char *data = context->m_buffer_queue->top();
  int current_size = context->m_buffer_queue->size();

  char recv_buffer[MAX_BUFF_SIZE], send_buffer[MAX_BUFF_SIZE];
  int recv_len = 0, send_len = 0;
  memset(recv_buffer, 0, sizeof(recv_buffer));
  memset(send_buffer, 0, sizeof(send_buffer));

  IoctlMsg *recv_message = (IoctlMsg*)data;
  int message_full_size = 4 + recv_message->size;
  if (current_size < message_full_size) {
    return;
  }
  context->m_buffer_queue->pop(recv_buffer, message_full_size);
  recv_len = message_full_size;
  IoctlMsg *send_message = (IoctlMsg*)send_buffer;
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

static void handle_set_rtspserver_command(struct bufferevent *bev)
{
  evutil_socket_t fd = bufferevent_getfd(bev);
  general_context *context = control_manager::get_instance()->get(fd);
  if (!context) return;
  string ip = context->m_conn_ip;
  uint32_t port = context->m_conn_port;

  const char *data = context->m_buffer_queue->top();
  int current_size = context->m_buffer_queue->size();

  char send_buffer[MAX_BUFF_SIZE];
  int send_len = 0;
  memset(send_buffer, 0, sizeof(send_buffer));
  IoctlMsg *send_message = (IoctlMsg*)send_buffer;
  memset(send_buffer, 0, sizeof(send_buffer));
  send_len = sizeof(IoctlMsg)+sizeof(IoctlMsgSetRtspServerReq);

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

static void handle_get_parameter_response(struct bufferevent *bev)
{
  evutil_socket_t fd = bufferevent_getfd(bev);
  general_context *context = control_manager::get_instance()->get(fd);
  if (!context) return;
  string ip = context->m_conn_ip;
  uint32_t port = context->m_conn_port;

  const char *data = context->m_buffer_queue->top();
  int current_size = context->m_buffer_queue->size();
  IoctlMsg *recv_message = (IoctlMsg*)data;
  int message_full_size = 4 + recv_message->size;

  int current_parameter_size = 0, total_parameter_size = 0, end_flag = 0;
  char parameter[64*1024];
  memset(parameter, 0 ,sizeof(parameter));
  for (int index = message_full_size; index < current_size-1; index++) {
    if (data[index] == '$' && data[index+1] == '\0') {
      IoctlMsgGetParameterResp *response = (IoctlMsgGetParameterResp*)(data+index+sizeof(IoctlMsg));
      if (0 == total_parameter_size) total_parameter_size = response->total;
      memcpy(parameter+current_parameter_size, response->result, response->count);
      current_parameter_size += response->count;
      end_flag = response->endflag;
      if (current_parameter_size >= total_parameter_size || response->endflag) {
        LOG(ERROR)<<"["<<ip<<":"<<port<<" --> localhost.fd="<<fd<<"]"<<" get parameter "<<current_parameter_size<<" bytes, total "<<total_parameter_size<<endl;
        break;
      }
      index = index + sizeof(IoctlMsg) + sizeof(IoctlMsgGetParameterResp);
    }
  }
  if (current_parameter_size < total_parameter_size && !end_flag) {
    return;
  }

  char* pstr = NULL;
	if (pstr = strstr(parameter, "network.hwaddress")) {
	  char mac[24];
	  sscanf(pstr, "network.hwaddress='%s';", mac);
	  mac[17] = 0;
	  context->m_camera_mac = mac;
    LOG(INFO)<<mac<<endl;
    context->m_buffer_queue->clear();
    handle_set_rtspserver_command(bev);
	}
}

static void handle_unregcognized_command(struct bufferevent *bev)
{
  evutil_socket_t fd = bufferevent_getfd(bev);
  general_context *context = control_manager::get_instance()->get(fd);
  if (!context) return;
  string ip = context->m_conn_ip;
  uint32_t port = context->m_conn_port;

  const char *data = context->m_buffer_queue->top();
  int current_size = context->m_buffer_queue->size();
  IoctlMsg *recv_message = (IoctlMsg*)data;
  LOG(WARNING)<<"control server get unregcognized command 0X"<<hex<<recv_message->ioctlCmd<<"["<<current_size<<" bytes], clear its buffer queue now"<<endl;
  context->m_buffer_queue->clear();
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
  int recv_len = 0;
  memset(recv_buffer, 0, sizeof(recv_buffer));
  evutil_socket_t fd = bufferevent_getfd(bev);
  general_context *context = control_manager::get_instance()->get(fd);
  if (!context) return;

  recv_len = read_event_buffer(bev, recv_buffer, sizeof(recv_buffer));

  context->m_buffer_queue->push(recv_buffer, recv_len);
  const char *data = context->m_buffer_queue->top();
  IoctlMsg *recv_message = (IoctlMsg*)data;

  switch (recv_message->ioctlCmd) {
  case IOCTL_KEEP_ALIVE:
    handle_keep_alive_command(bev);
    break;
  case IOCTL_REQUEST_TEST_00:
    handle_get_parameter_command(bev);
    break;
  case IOCTL_GET_PARAMETER_RESP:
    handle_get_parameter_response(bev);
    break;
  default:
    handle_unregcognized_command(bev);
    break;
  }
}
