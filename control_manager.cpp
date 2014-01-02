// Copyright (c) 2012-2013 Hua Xiufeng <huaxiufeng@kaisquare.com.cn>
// Copyright (c) 2013 KAI Square Software Limited
// All rights reserved
//
// Author: Hua Xiufeng

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "message_types.h"
#include "global_config.h"
#include "global_net_func.h"
#include "gloghelper.h"
#include "control_manager.h"
#include "message_server_controller.h"
using namespace std;

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

static void handle_set_rtspserver_command(struct bufferevent *bev);
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
  for (int index = 0; index < current_size-1; index++) {
    if (data[index] == '$' && data[index+1] == '\0') {
      IoctlMsgGetParameterResp *response = (IoctlMsgGetParameterResp*)(data+index+sizeof(IoctlMsg));
      if (0 == total_parameter_size) total_parameter_size = response->total;
      memcpy(parameter+current_parameter_size, response->result, response->count);
      current_parameter_size += response->count;
      end_flag = response->endflag;
      //LOG(INFO)<<"index:"<<(int)response->index<<" end:"<<(int)response->endflag<<" ["<<current_parameter_size<<"/"<<(int)response->total<<"]"<<endl;
      if (current_parameter_size >= total_parameter_size || end_flag) {
        LOG(INFO)<<"["<<ip<<":"<<port<<" --> localhost.fd="<<fd<<"]"<<" get parameter "<<current_parameter_size<<" bytes, total "<<total_parameter_size<<endl;
        break;
      }
      index = index + sizeof(IoctlMsg) + sizeof(IoctlMsgGetParameterResp) + response->count - 1;
    }
  }
  if (current_parameter_size >= total_parameter_size && end_flag) {
    char* pstr = NULL;
    if (pstr = strstr(parameter, "network.hwaddress")) {
      char mac[24];
      sscanf(pstr, "network.hwaddress='%s';", mac);
      mac[17] = 0;
    }
    context->m_buffer_queue->clear();
    handle_set_rtspserver_command(bev);
  }
}

static void handle_set_rtspserver_command(struct bufferevent *bev)
{
  evutil_socket_t fd = bufferevent_getfd(bev);
  general_context *context = control_manager::get_instance()->get(fd);
  if (!context) return;
  string ip = context->m_conn_ip;
  uint32_t port = context->m_conn_port;

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

static void handle_set_rtspserver_response(struct bufferevent *bev)
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
  IoctlMsgSetRtspServerResp *resp = (IoctlMsgSetRtspServerResp*)recv_message->data;
  //LOG(INFO)<<"get set rtspserver response from camera, result: "<<resp->result<<endl;
}

static void handle_set_snapshotserver_command(struct bufferevent *bev)
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
  memset(send_buffer, 0, sizeof(send_buffer));
  send_len = sizeof(IoctlMsg)+sizeof(IoctlMsgGetSnapshotServerResp)+sizeof(stServerDef);;

  send_message->magic[0] = '$';
  send_message->magic[1] = '\0';
  send_message->ioctlCmd = IOCTL_GET_SNAPSHOTSERVER_RESP;
  IoctlMsgGetSnapshotServerResp *resp = (IoctlMsgGetSnapshotServerResp*)send_message->data;
  resp->interval = 30;
  resp->number = 1;
  stServerDef *server = resp->servers;
  strcpy(server->name, g_local_address);
  server->port = g_snapshot_server_port;
  server->type = server->err = 0;
  send_message->size = send_len - 4;

  write_event_buffer(bev, send_buffer, send_len);
}

void control_manager::read_event_callback(struct bufferevent *bev, void *arg)
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
    handle_keep_alive_command(control_manager::get_instance(), bev);
    break;
  case IOCTL_CAM_HELO:
    handle_get_parameter_command(bev);
    break;
  case IOCTL_GET_PARAMETER_RESP:
    handle_get_parameter_response(bev);
    break;
  case IOCTL_SET_RTSPSERVER_RESP:
    handle_set_rtspserver_response(bev);
    break;
  case IOCTL_GET_SNAPSHOTSERVER_REQ:
    handle_set_snapshotserver_command(bev);
    break;
  default:
    handle_unregcognized_command(control_manager::get_instance(), bev);
    break;
  }
}
