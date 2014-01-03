// Copyright (c) 2012-2013 Hua Xiufeng <huaxiufeng@kaisquare.com.cn>
// Copyright (c) 2013 KAI Square Software Limited
// All rights reserved
//
// Author: Hua Xiufeng

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <errno.h>
#include "global_config.h"
#include "global_net_func.h"
#include "gloghelper.h"
#include "account_manager.h"
#include "control_manager.h"
#include "message_types.h"
#include "rtsp_manager.h"
#include "snapshot_manager.h"
#include "message_server_controller.h"

message_server_controller::message_server_controller()
{
  m_event_base = event_base_new();

  add_listen_event(account_manager::get_instance(), g_account_server_port);
  add_listen_event(control_manager::get_instance(), g_control_server_port);
  add_listen_event(rtsp_manager::get_instance(), g_rtsp_server_port);
  add_listen_event(snapshot_manager::get_instance(), g_snapshot_server_port);
}

message_server_controller::~message_server_controller()
{
  event_base_free(m_event_base);
}

void message_server_controller::run()
{
  event_base_dispatch(m_event_base);
}

void message_server_controller::kill()
{

}

void message_server_controller::add_listen_event(void *_manager, uint32_t _port)
{
  string _type = ((general_manager*)_manager)->get_name();
  evutil_socket_t listener = socket(AF_INET, SOCK_STREAM, 0);
  if (listener <= 0) {
    LOG(ERROR)<<"["<<_type<<"]"<<" create socket error: "<<strerror(errno)<<", listener = "<<listener<<endl;
    return;
  }

  evutil_make_listen_socket_reuseable(listener);
  evutil_make_socket_nonblocking(listener);

  struct sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = 0;
  sin.sin_port = htons(_port);
  if (bind(listener, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
    LOG(ERROR)<<"["<<_type<<"]"<<" bind error: "<<strerror(errno)<<endl;
    return;
  }

  if (listen(listener, 64) < 0) {
    LOG(ERROR)<<"["<<_type<<"]"<<" listen error: "<<strerror(errno)<<endl;
    return;
  }

  LOG(INFO)<<"["<<_type<<"]"<<" waiting for message at port "<<_port<<endl;

  struct event *listen_event = event_new(m_event_base, listener, EV_READ|EV_PERSIST, message_accept_cb, _manager);
  event_add(listen_event, NULL);
  ((general_manager*)_manager)->set_listen_event(listen_event);
  struct event *timer_event = event_new(m_event_base, -1, EV_PERSIST, timer_reached_cb, _manager);
  struct timeval interval = {15, 0};
  event_add(timer_event, &interval);
}

void message_accept_cb(evutil_socket_t listener, short event, void *arg)
{
  general_manager *manager = (general_manager*)arg;
  struct event_base *base = message_server_controller::get_instance()->get_event_base();
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

  string mac = get_peer_mac(fd);
  string ip = inet_ntoa(sin.sin_addr);
  uint32_t port = ntohs(sin.sin_port);
  //LOG(INFO)<<"["<<ip<<":"<<port<<" - "<<mac<<" --> localhost.fd="<<fd<<"] "<<manager->get_name()<<" service accept connection"<<endl;

  struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE|BEV_OPT_DEFER_CALLBACKS);
  bufferevent_setcb(bev, message_read_cb, NULL, message_error_cb, arg);
  bufferevent_enable(bev, EV_READ|EV_WRITE|EV_PERSIST);

  general_context context;
  context.m_conn_ip = ip;
  context.m_conn_port = port;
  context.m_camera_mac = mac;
  manager->add(fd, context, bev);

  if (g_connection_callback) {
    (*g_connection_callback)(ip.c_str(), mac.c_str(), manager->get_name().c_str(), fd, true);
  }
}

void message_read_cb(struct bufferevent *bev, void *arg)
{
  general_manager *manager = (general_manager*)arg;
  evutil_socket_t fd = bufferevent_getfd(bev);
  if (manager->get(fd)) {
    manager->read_event_callback(bev, arg);
  }
}

void message_error_cb(struct bufferevent *bev, short event, void *arg)
{
  general_manager *manager = (general_manager*)arg;
  evutil_socket_t fd = bufferevent_getfd(bev);
  general_context *context = snapshot_manager::get_instance()->get(fd);
  if (!context) return;
  string mac = context->m_camera_mac;
  string ip = context->m_conn_ip;
  uint32_t port = context->m_conn_port;
  string type = manager->get_name();

  if (event & BEV_EVENT_TIMEOUT) {
    LOG(ERROR)<<"["<<ip<<":"<<port<<" - "<<mac<<" --> localhost.fd="<<fd<<"] "<<type<<" service read/write time out"<<endl;
  }
  else if (event & BEV_EVENT_EOF) {
    LOG(ERROR)<<"["<<ip<<":"<<port<<" - "<<mac<<" --> localhost.fd="<<fd<<"] "<<type<<" service connection closed"<<endl;
  }
  else if (event & BEV_EVENT_ERROR) {
    LOG(ERROR)<<"["<<ip<<":"<<port<<" - "<<mac<<" --> localhost.fd="<<fd<<"] "<<type<<" service some other error"<<endl;
  }

  manager->remove(fd);
}

void timer_reached_cb(evutil_socket_t fd, short event, void *arg)
{
  string mac;
  general_manager *manager = (general_manager*)arg;
  do {
    mac = "";
    string ip;
    evutil_socket_t fd = -1;
    general_context_table_type::iterator itor = manager->m_context_table.begin();
    for (;itor != manager->m_context_table.end(); itor++) {
      //LOG(INFO)<<manager->get_name()<<" "<<itor->first<<" "<<ctime(&(itor->second.m_update_time));
      if (time(NULL) - itor->second.m_update_time > 30) {
        mac = itor->second.m_camera_mac;
        ip = itor->second.m_conn_ip;
        fd = itor->first;
        break;
      }
    }
    if (fd > 0) {
      if (g_connection_callback) {
        (*g_connection_callback)(ip.c_str(), mac.c_str(), manager->get_name().c_str(), fd, false);
      }
      manager->remove(fd);
    }
  } while (mac.length());
}

void handle_keep_alive_command(void *_manager, struct bufferevent *bev)
{
  evutil_socket_t fd = bufferevent_getfd(bev);
  general_context *context = ((general_manager*)_manager)->get(fd);
  if (!context) return;

  const char *data = context->m_buffer_queue->top();
  int current_size = context->m_buffer_queue->size();

  IoctlMsg *recv_message = (IoctlMsg*)data;
  int message_full_size = 4 + recv_message->size;
  if (current_size < message_full_size) {
    return;
  }
  context->m_buffer_queue->pop(NULL, message_full_size);
  ((general_manager*)_manager)->keep_alive(fd);
}

void handle_unregcognized_command(void *_manager, struct bufferevent *bev)
{
  string type = ((general_manager*)_manager)->get_name();
  evutil_socket_t fd = bufferevent_getfd(bev);
  general_context *context = ((general_manager*)_manager)->get(fd);
  if (!context) return;

  string mac = context->m_camera_mac;
  string ip = context->m_conn_ip;
  uint32_t port = context->m_conn_port;

  const char *data = context->m_buffer_queue->top();
  int current_size = context->m_buffer_queue->size();
  IoctlMsg *recv_message = (IoctlMsg*)data;
  //LOG(WARNING)<<"["<<ip<<":"<<port<<" - "<<mac<<" --> localhost.fd="<<fd<<"] "<<type<<" service get unregcognized command 0X"<<hex<<recv_message->ioctlCmd<<" ["<<oct<<current_size<<" bytes], clear buffer queue now"<<endl;

  context->m_buffer_queue->clear();
}
