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
#include "gloghelper.h"
#include "account_manager.h"
#include "control_manager.h"
#include "rtsp_manager.h"
#include "snapshot_manager.h"
#include "simple_echo_server.h"
using namespace std;

static void handle_get_controlserver_command(camera_context *context)
{
  const char *data = context->m_account_buffer_queue->top();
  int current_size = context->m_account_buffer_queue->size();

  char recv_buffer[MAX_BUFF_SIZE], send_buffer[MAX_BUFF_SIZE];
  int recv_len = 0, send_len = 0;
  memset(recv_buffer, 0, sizeof(recv_buffer));
  memset(send_buffer, 0, sizeof(send_buffer));

  IoctlMsg *recv_message = (IoctlMsg*)data;
  int message_full_size = 4 + recv_message->size;
  if (current_size < message_full_size) {
    return;
  }
  context->m_account_buffer_queue->pop(recv_buffer, message_full_size);
  recv_len = message_full_size;
  IoctlMsg *send_message = (IoctlMsg*)send_buffer;
  send_len = sizeof(IoctlMsg)+sizeof(IoctlMsgGetControlServerResp)+sizeof(stServerDef);
  send_message->magic[0] = '$';
  send_message->magic[1] = '\0';
  send_message->ioctlCmd = IOCTL_GET_CONTROLSERVER_RESP;
  IoctlMsgGetControlServerResp *resp = (IoctlMsgGetControlServerResp*)send_message->data;
  resp->number = 1;
  stServerDef *server = resp->servers;
  strcpy(server->name, g_local_address);
  server->port = context->m_control_port;
  server->type = server->err = 0;
  send_message->size = send_len - 4;

  write(context->m_account_fd, send_buffer, send_len);
}

void* account_manager::start_account_service(void *arg)
{
  account_manager *manager = account_manager::get_instance();

  manager->m_account_listen_fd = manager->start_listen(g_account_server_port, "account");
  while (manager->m_keep_running) {
    manager->handle_accept_connection(0, manager->m_account_listen_fd, "account");
  }
  LOG(INFO)<<"exit start_account_service"<<endl;
}

void account_manager::run()
{
  start_echo_server();
  pthread_create(&m_account_thread_id, 0, start_account_service, NULL);
}

void account_manager::kill()
{
  stop_echo_server();
  close(m_account_listen_fd);
  m_keep_running = false;
  pthread_join(m_account_thread_id, NULL);

  vector<int> fd_all;
  context_manager::get_instance()->get(fd_all);
  for (size_t i = 0; i < fd_all.size(); i++) {
    camera_context *ctx = context_manager::get_instance()->get(fd_all[i]);
    if (ctx) ctx->m_connected = false;
  }
}

int account_manager::handle_accept_connection(void *arg, int listener, const char *_type)
{
  camera_context context;

  int fd = general_manager::handle_accept_connection(&context, m_account_listen_fd, _type);
  if (fd <= 0) return fd;

  context.m_account_fd = fd;
  context.m_account_listen_fd = m_account_listen_fd;
  context_manager::get_instance()->add(context);

  camera_context *ctx = context_manager::get_instance()->get(fd);

  pthread_t th;
  pthread_create(&th, NULL, start_camera_service, ctx);
  pthread_detach(th);
  ctx->m_thread_id = th;
  LOG(INFO)<<"******* create thread for a new camera "<<th<<endl;

  return fd;
}

int account_manager::handle_read_buffer(void *arg)
{
  camera_context *ctx = (camera_context*)arg;
  if (!ctx || ctx->m_account_fd <= 0) return -1;

  int fd = ctx->m_account_fd;
  int res = general_manager::check_readable(fd, 10*1000);
  if (res <= 0) {
    return res;
  }

  char recv_buffer[MAX_BUFF_SIZE];
  memset(recv_buffer, 0, sizeof(recv_buffer));
  int recv_len = read(fd, recv_buffer, sizeof(recv_buffer));
  if (recv_len <= 0) {
    return recv_len;
  }

  ctx->m_account_buffer_queue->push(recv_buffer, recv_len);
  const char *data = ctx->m_account_buffer_queue->top();
  IoctlMsg *recv_message = (IoctlMsg*)data;

  switch (recv_message->ioctlCmd) {
  case IOCTL_KEEP_ALIVE:
    //LOG(INFO)<<"IOCTL_KEEP_ALIVE "<<fd<<endl;
    handle_keep_alive_command(ctx->m_account_buffer_queue, &(ctx->m_account_update_time));
    break;
  case IOCTL_GET_CONTROLSERVER_REQ:
    //LOG(INFO)<<"IOCTL_GET_CONTROLSERVER_REQ "<<fd<<endl;
    handle_get_controlserver_command(ctx);
    break;
  default:
    //LOG(INFO)<<"UNRECOGNIZED COMMAND "<<fd<<hex<<" 0X"<<recv_message->ioctlCmd<<endl;
    handle_unrecognized_command(ctx->m_account_buffer_queue);
    break;
  }
}

void* account_manager::start_camera_service(void *arg)
{
  camera_context *ctx = (camera_context*)arg;

  if (g_connection_callback) {
    (*g_connection_callback)(ctx->m_conn_ip.c_str(), ctx->m_camera_mac.c_str(), ctx->m_account_fd, true);
  }

  ctx->m_control_listen_fd = control_manager::get_instance()->start_listen(ctx->m_control_port, "control");
  ctx->m_rtsp_listen_fd = rtsp_manager::get_instance()->start_listen(ctx->m_rtsp_port, "rtsp");
  ctx->m_snapshot_listen_fd = snapshot_manager::get_instance()->start_listen(ctx->m_snapshot_port, "snapshot");

  time_t last_check_time = time(NULL);
  while(ctx->m_connected) {
    if (time(NULL) - last_check_time > 10) {
      last_check_time = time(NULL);
      camera_context *ctx = (camera_context*)arg;
      //LOG(INFO)<<"timer checking ["<<ctx->m_conn_ip<<"-"<<ctx->m_camera_mac<<"] now:"<<time(NULL)<<" rtsp:"<<ctx->m_rtsp_update_time<<" snap:"<<ctx->m_snapshot_update_time<<endl;
      if ((g_stream_callback && time(NULL) - ctx->m_rtsp_update_time > 60) &&
          (g_snapshot_callback && time(NULL) - ctx->m_snapshot_update_time > 60))
      {
        ctx->m_connected = false;
        break;
      }
    }
    // handle control accept
    control_manager::get_instance()->handle_accept_connection(arg, 0, "control");
    // handle rtsp accept
    rtsp_manager::get_instance()->handle_accept_connection(arg, 0, "rtsp");
    // handle snapshot accept
    snapshot_manager::get_instance()->handle_accept_connection(arg, 0, "snapshot");
    // handle account read data
    account_manager::get_instance()->handle_read_buffer(arg);
    // handle control read data
    control_manager::get_instance()->handle_read_buffer(arg);
    // handle rtsp read data
    rtsp_manager::get_instance()->handle_read_buffer(arg);
    // handle snapshot read data
    snapshot_manager::get_instance()->handle_read_buffer(arg);
  }

  if (g_connection_callback) {
    (*g_connection_callback)(ctx->m_conn_ip.c_str(), ctx->m_camera_mac.c_str(), ctx->m_account_fd, false);
  }
  context_manager::get_instance()->remove(ctx->m_account_fd);

  LOG(INFO)<<"$$$$$$$$$$$$$$$$$$$$$$$$$ thread exit"<<endl;

  return NULL;
}
