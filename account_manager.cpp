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
#include "message_server_controller.h"
#include "account_manager.h"
using namespace std;

static void handle_keep_alive_command(struct bufferevent *bev)
{
  evutil_socket_t fd = bufferevent_getfd(bev);
  general_context *context = account_manager::get_instance()->get(fd);
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

static void handle_get_controlserver_command(struct bufferevent *bev)
{
  evutil_socket_t fd = bufferevent_getfd(bev);
  general_context *context = account_manager::get_instance()->get(fd);
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
  send_len = sizeof(IoctlMsg)+sizeof(IoctlMsgGetControlServerResp)+sizeof(stServerDef);
  send_message->magic[0] = '$';
  send_message->magic[1] = '\0';
  send_message->ioctlCmd = IOCTL_GET_CONTROLSERVER_RESP;
  IoctlMsgGetControlServerResp *resp = (IoctlMsgGetControlServerResp*)send_message->data;
  resp->number = 1;
  stServerDef *sever = resp->servers;
  strcpy(sever->name, g_local_address);
  sever->port = g_control_server_port;
  sever->type = sever->err = 0;
  send_message->size = send_len - 4;

  write_event_buffer(bev, send_buffer, send_len);
}

static void handle_unregcognized_command(struct bufferevent *bev)
{
  evutil_socket_t fd = bufferevent_getfd(bev);
  general_context *context = account_manager::get_instance()->get(fd);
  if (!context) return;
  string ip = context->m_conn_ip;
  uint32_t port = context->m_conn_port;

  const char *data = context->m_buffer_queue->top();
  int current_size = context->m_buffer_queue->size();
  IoctlMsg *recv_message = (IoctlMsg*)data;
  LOG(WARNING)<<"account server get unregcognized command 0X"<<hex<<recv_message->ioctlCmd<<"["<<current_size<<" bytes], clear its buffer queue now"<<endl;
  context->m_buffer_queue->clear();
}

void account_manager::read_event_callback(struct bufferevent *bev, void *arg)
{
  char recv_buffer[MAX_BUFF_SIZE];
  int recv_len = 0;
  memset(recv_buffer, 0, sizeof(recv_buffer));
  evutil_socket_t fd = bufferevent_getfd(bev);
  general_context *context = account_manager::get_instance()->get(fd);
  if (!context) return;

  recv_len = read_event_buffer(bev, recv_buffer, sizeof(recv_buffer));

  context->m_buffer_queue->push(recv_buffer, recv_len);
  const char *data = context->m_buffer_queue->top();
  IoctlMsg *recv_message = (IoctlMsg*)data;

  switch (recv_message->ioctlCmd) {
  case IOCTL_KEEP_ALIVE:
    handle_keep_alive_command(bev);
    break;
  case IOCTL_GET_CONTROLSERVER_REQ:
    handle_get_controlserver_command(bev);
    break;
  default:
    handle_unregcognized_command(bev);
    break;
  }
}
