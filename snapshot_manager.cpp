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
#include "snapshot_manager.h"
using namespace std;

static void handle_set_snapshot_command(struct bufferevent *bev)
{
  evutil_socket_t fd = bufferevent_getfd(bev);
  general_context *context = snapshot_manager::get_instance()->get(fd);
  if (!context) return;
  string ip = context->m_conn_ip;
  uint32_t port = context->m_conn_port;

  const char *data = context->m_buffer_queue->top();
  int current_size = context->m_buffer_queue->size();
  IoctlMsg *recv_message = (IoctlMsg*)data;
  int message_full_size = 4 + recv_message->size;
  if (current_size < message_full_size) {
    return;
  }

  int current_snapshot_size = 0, total_snapshot_size = 0, end_flag = 0;
  char snapshot[MAX_SNAPSHOT_SIZE];
  for (int index = 0; index < current_size-1; index++) {
    if (data[index] == '$' && data[index+1] == '\0') {
      IoctlMsgSetSnapshotReq *response = (IoctlMsgSetSnapshotReq*)(data+index+sizeof(IoctlMsg));
      if (0 == total_snapshot_size) total_snapshot_size = response->total;
      memcpy(snapshot+current_snapshot_size, response->result, response->count);
      current_snapshot_size += response->count;
      end_flag = response->endflag;
      //LOG(INFO)<<"index:"<<(int)response->index<<" end:"<<(int)response->endflag<<" ["<<current_snapshot_size<<"/"<<(int)response->total<<"]"<<endl;
      if (current_snapshot_size >= total_snapshot_size || end_flag) {
        LOG(INFO)<<"["<<ip<<":"<<port<<" --> localhost.fd="<<fd<<"]"<<" get snapshot "<<current_snapshot_size<<" bytes, total "<<total_snapshot_size<<endl;
        break;
      }
      index = index + sizeof(IoctlMsg) + sizeof(IoctlMsgSetSnapshotReq) + response->count - 1;
    }
  }
  if (current_snapshot_size >= total_snapshot_size && end_flag) {
    context->m_buffer_queue->clear();
    if (g_snapshot_callback) {
      (*g_snapshot_callback)(context->m_camera_mac.c_str(), (const unsigned char*)snapshot, current_snapshot_size);
    }
  }
}

void snapshot_manager::read_event_callback(struct bufferevent *bev, void *arg)
{
  char recv_buffer[MAX_BUFF_SIZE];
  int recv_len = 0;
  memset(recv_buffer, 0, sizeof(recv_buffer));
  evutil_socket_t fd = bufferevent_getfd(bev);
  general_context *context = snapshot_manager::get_instance()->get(fd);
  if (!context) return;

  recv_len = read_event_buffer(bev, recv_buffer, sizeof(recv_buffer));

  context->m_buffer_queue->push(recv_buffer, recv_len);
  const char *data = context->m_buffer_queue->top();
  IoctlMsg *recv_message = (IoctlMsg*)data;

  switch (recv_message->ioctlCmd) {
  case IOCTL_CAM_HELO:
  case IOCTL_KEEP_ALIVE:
    handle_keep_alive_command(snapshot_manager::get_instance(), bev);
    break;
  case IOCTL_SET_SNAPSHOT_REQ:
    handle_set_snapshot_command(bev);
    break;
  default:
    handle_unregcognized_command(snapshot_manager::get_instance(), bev);
    break;
  }
}
