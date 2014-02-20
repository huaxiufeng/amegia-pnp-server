// Copyright (c) 2012-2013 Hua Xiufeng <huaxiufeng@kaisquare.com.cn>
// Copyright (c) 2013 KAI Square Software Limited
// All rights reserved
//
// Author: Hua Xiufeng

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <stdio.h>
#include <signal.h>
#include "gloghelper.h"
#include "global_config.h"
#include "message_types.h"
#include "general_manager.h"
#include <algorithm>

camera_context::camera_context(int _account_fd)
{
  m_connected = true;
  m_account_port = g_account_server_port;
  m_account_fd = _account_fd;
  m_account_create_time = time(NULL);
  m_account_update_time = time(NULL);
  m_account_buffer_queue = NULL;

  m_control_port = 0;
  m_control_fd = 0;
  m_control_create_time = time(NULL);
  m_control_update_time = time(NULL);
  m_control_buffer_queue = NULL;

  m_rtsp_port = 0;
  m_rtsp_fd = 0;
  m_rtsp_create_time = time(NULL);
  m_rtsp_update_time = time(NULL);
  m_rtsp_buffer_queue = NULL;
  m_rtsp_frame_count = 0;
  m_rtsp_profile_frame_rate = 25.0;
  m_fragmentation_units = NULL;

  m_snapshot_port = 0;
  m_snapshot_fd = 0;
  m_snapshot_create_time = time(NULL);
  m_snapshot_update_time = time(NULL);
  m_snapshot_buffer_queue = NULL;
  m_snapshot_count = 0;
}

void context_manager::add(camera_context &context)
{
  lock();

  int _fd = context.m_account_fd;
  m_context_table.insert(pair<int,camera_context>(_fd, context));

  std::map<int, camera_context>::iterator itor = m_context_table.begin();
  camera_context* ctx = NULL;
  for (itor = m_context_table.begin(); itor != m_context_table.end(); itor++) {
    if (itor->first == _fd) {
      ctx = &(itor->second);
      break;
    }
  }
  if (!ctx) {
    unlock();
    return;
  }

  std::vector<int> control_port_list;
  for (itor = m_context_table.begin(); itor != m_context_table.end(); itor++) {
    control_port_list.push_back(itor->second.m_control_port);
  }
  for (int i = 0; i < 4096; i++) {
    int control_port = g_account_server_port+i*3+1;
    if (std::find(control_port_list.begin(), control_port_list.end(), control_port) == control_port_list.end()) {
      ctx->m_control_port = control_port;
      ctx->m_rtsp_port = control_port+1;
      ctx->m_snapshot_port = control_port+2;
      break;
    }
  }

  ctx->m_account_buffer_queue = new buffer_queue(4096);
  ctx->m_control_buffer_queue = new buffer_queue(4096);
  ctx->m_rtsp_buffer_queue = new buffer_queue(4096);
  ctx->m_fragmentation_units = new buffer_queue(4096);
  ctx->m_snapshot_buffer_queue = new buffer_queue(4096);

  unlock();
}

camera_context* context_manager::get(int _fd)
{
  camera_context* ctx = NULL;
  lock();

  std::map<int, camera_context>::iterator itor = m_context_table.begin();
  for (;itor != m_context_table.end(); itor++) {
    if (itor->second.m_account_fd == _fd || itor->second.m_control_fd == _fd || itor->second.m_rtsp_fd == _fd || itor->second.m_snapshot_fd == _fd) {
      ctx = &(itor->second);
      break;
    }
  }

  unlock();
  return ctx;
}

size_t context_manager::get(vector<int>& _fd_all)
{
  size_t res = 0;
  lock();

  std::map<int, camera_context>::iterator itor = m_context_table.begin();
  for (;itor != m_context_table.end(); itor++) {
    _fd_all.push_back(itor->second.m_account_fd);
  }

  unlock();
  return res;
}

void context_manager::remove(int _fd)
{
  lock();

  camera_context* ctx = NULL;
  std::map<int, camera_context>::iterator itor = m_context_table.begin();
  for (;itor != m_context_table.end(); itor++) {
    if (itor->second.m_account_fd == _fd || itor->second.m_control_fd == _fd || itor->second.m_rtsp_fd == _fd || itor->second.m_snapshot_fd == _fd) {
      ctx = &(itor->second);
      break;
    }
  }
  if (!ctx) {
    unlock();
    return;
  }

  ctx->m_connected = false;

  if (ctx->m_account_fd) {
    close(ctx->m_account_fd);
  }
  if (ctx->m_control_fd) {
    close(ctx->m_control_fd);
  }
  if (ctx->m_control_listen_fd) {
    close(ctx->m_control_listen_fd);
  }
  if (ctx->m_rtsp_fd) {
    close(ctx->m_rtsp_fd);
  }
  if (ctx->m_rtsp_listen_fd) {
    close(ctx->m_rtsp_listen_fd);
  }
  if (ctx->m_snapshot_fd) {
    close(ctx->m_snapshot_fd);
  }
  if (ctx->m_snapshot_listen_fd) {
    close(ctx->m_snapshot_listen_fd);
  }

  if(ctx->m_account_buffer_queue) {
    delete ctx->m_account_buffer_queue;
  }
  if(ctx->m_control_buffer_queue) {
    delete ctx->m_control_buffer_queue;
  }
  if(ctx->m_rtsp_buffer_queue) {
    delete ctx->m_rtsp_buffer_queue;
  }
  if(ctx->m_snapshot_buffer_queue) {
    delete ctx->m_snapshot_buffer_queue;
  }
  if (ctx->m_fragmentation_units) {
    delete ctx->m_fragmentation_units;
  }

  m_context_table.erase(itor);

  unlock();
}

size_t context_manager::count()
{
  size_t ret = 0;
  ret = m_context_table.size();
  return ret;
}

std::string context_manager::report()
{
  std::string report;
  char line[512];
  snprintf(line, sizeof(line), "%-3s %-15s %-17s %-14s %-8s %-6s %-6s %-6s %-8s %-6s %-6s %-6s %-8s %-6s %-6s %-6s %-9s %-8s %-6s %-6s %-6s %-9s %-8s\n",
           "idx", "ip", "mac", "create", "account", "port", "sfd", "cfd", "control", "port", "sfd", "cfd", "rtsp", "port", "sfd", "cfd", "cnt", "snapshot", "port", "sfd", "cfd", "cnt", "now");
  report += line;
  std::map<int, camera_context>::iterator itor = m_context_table.begin();
  for (int index = 1;itor != m_context_table.end(); itor++, index++) {
    time_t now = time(NULL);
    char create_time[32], account_time[32], control_time[32], rtsp_time[32], snapshot_time[32], cur_time[32];
    strftime(create_time, 32, "%m-%d %H:%M:%S", localtime(&(itor->second.m_account_create_time)));
    strftime(account_time, 32, "%H:%M:%S", localtime(&(itor->second.m_account_update_time)));
    strftime(control_time, 32, "%H:%M:%S", localtime(&(itor->second.m_control_update_time)));
    strftime(rtsp_time, 32, "%H:%M:%S", localtime(&(itor->second.m_rtsp_update_time)));
    strftime(snapshot_time, 32, "%H:%M:%S", localtime(&(itor->second.m_snapshot_update_time)));
    strftime(cur_time, 32, "%H:%M:%S", localtime(&now));
    snprintf(line, sizeof(line), "%-3d %-15s %-17s %-14s %-8s %-6d %-6d %-6d %-8s %-6d %-6d %-6d %-8s %-6d %-6d %-6d %-9lu %-8s %-6d %-6d %-6d %-9lu %-8s\n",
             index, itor->second.m_conn_ip.c_str(), itor->second.m_camera_mac.c_str(),
             create_time,
             account_time, itor->second.m_account_port, itor->second.m_account_listen_fd, itor->second.m_account_fd,
             control_time, itor->second.m_control_port, itor->second.m_control_listen_fd, itor->second.m_control_fd,
             rtsp_time, itor->second.m_rtsp_port, itor->second.m_rtsp_listen_fd, itor->second.m_rtsp_fd, itor->second.m_rtsp_frame_count,
             snapshot_time, itor->second.m_snapshot_port, itor->second.m_snapshot_listen_fd, itor->second.m_snapshot_fd, itor->second.m_snapshot_count,
             cur_time);
    report += line;
  }
  return report;
}

int general_manager::handle_accept_connection(void *arg, int _listener, const char *_type)
{
  camera_context *ctx = (camera_context*)arg;
  if (!_listener) _listener = ctx->m_account_listen_fd;

  int res = check_readable(_listener, 10*1000);
  if (res <= 0) {
    return res;
  }

  struct sockaddr_in sin;
  socklen_t slen = sizeof(sin);
  int fd = accept(_listener, (struct sockaddr *)&sin, &slen);
  if (fd <= 0) {
    LOG(ERROR)<<_type<<" accept error: "<<strerror(errno)<<", fd = "<<fd<<endl;
    return fd;
  }
  if (fd > FD_SETSIZE) {
    LOG(ERROR)<<_type<<" accept error: "<<strerror(errno)<<", fd > FD_SETSIZE ["<<fd<<" > "<<FD_SETSIZE<<"]"<<endl;
    return fd;
  }

  string ip = inet_ntoa(sin.sin_addr);
  uint32_t port = ntohs(sin.sin_port);

  //if (ip != "10.101.10.189") {
  //  close(fd);
  //  return -1;
  //}

  LOG(INFO)<<"["<<ip<<":"<<port<<" --> localhost.fd="<<fd<<"] "<<_type<<" service accept connection from "<<_listener<<endl;

  if (ctx) {
    ctx->m_conn_ip = ip;
  }

  return fd;
}

int general_manager::handle_read_buffer(void *arg)
{
  return 0;
}

int general_manager::check_readable(int _sock_fd, long _timeout_usec)
{
  int res = 0;
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(_sock_fd, &fds);
  struct timeval timeout;
  timeout.tv_sec = _timeout_usec / (1000*1000), timeout.tv_usec = _timeout_usec % (1000*1000);
  switch (select(_sock_fd + 1, &fds, NULL, NULL, &timeout)) {
    case -1:
      res = -2; // select error
      break;
    case 0:
      res = -1; // time out
      break;
    default:
      if(FD_ISSET(_sock_fd, &fds)) res = 1;
      else res = 0;
      break;
  }
  return res;
}

int general_manager::start_listen(int _port, const char *_type)
{
  int listener = socket(AF_INET, SOCK_STREAM, 0);
  if (listener <= 0) {
    LOG(ERROR)<<"["<<_type<<"]"<<" create socket error: "<<strerror(errno)<<", listener = "<<listener<<endl;
    return 0;
  }

  int flags = fcntl(listener, F_GETFL, 0);
  fcntl(listener, F_SETFL, flags | O_NONBLOCK);
  int reuse = 1;
  setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

  struct sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = htonl(INADDR_ANY);
  sin.sin_port = htons(_port);
  bzero(&(sin.sin_zero), 8);
  socklen_t slen = sizeof(sin);
  if (bind(listener, (struct sockaddr *)&sin, slen) < 0) {
    LOG(ERROR)<<"["<<_type<<"]"<<" bind "<<_port<<" error: "<<strerror(errno)<<endl;
    return 0;
  }

  if (listen(listener, 64) < 0) {
    LOG(ERROR)<<"["<<_type<<"]"<<" listen error: "<<strerror(errno)<<endl;
    return 0;
  }

  LOG(INFO)<<listener<<" ["<<_type<<"]"<<" waiting for message at port "<<_port<<endl;
  return listener;
}

void general_manager::handle_keep_alive_command(buffer_queue* _queue, time_t *_time)
{
  *_time = time(NULL);
  if (!_queue) return;

  const char *data = _queue->top();
  int current_size = _queue->size();

  IoctlMsg *recv_message = (IoctlMsg*)data;
  int message_full_size = 4 + recv_message->size;
  if (current_size < message_full_size) {
    return;
  }
  _queue->pop(NULL, message_full_size);
}

void general_manager::handle_unrecognized_command(buffer_queue* _queue)
{
  _queue->clear();
}
