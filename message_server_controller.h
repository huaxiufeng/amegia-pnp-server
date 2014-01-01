// Copyright (c) 2012-2013 Hua Xiufeng <huaxiufeng@kaisquare.com.cn>
// Copyright (c) 2013 KAI Square Software Limited
// All rights reserved
//
// Author: Hua Xiufeng

#ifndef AMEGIA_PNP_SERVER_MESSAGE_SERVER_CONTROLLER_H
#define AMEGIA_PNP_SERVER_MESSAGE_SERVER_CONTROLLER_H

#include <event2/event.h>
#include <event2/bufferevent.h>

class message_server_controller
{
public:
  static message_server_controller* get_instance() {
    static message_server_controller instance;
    return &instance;
  }
  struct event_base* get_event_base() {return m_event_base;}
  void run();
  void kill();

private:
  message_server_controller();
  ~message_server_controller();
  void add_listen_event(void *_manager, uint32_t _port);

private:
  struct event_base *m_event_base;
};

extern void message_accept_cb(evutil_socket_t listener, short event, void *arg);
extern void message_read_cb(struct bufferevent *bev, void *arg);
extern void message_error_cb(struct bufferevent *bev, short event, void *arg);

#endif // AMEGIA_PNP_SERVER_MESSAGE_SERVER_CONTROLLER_H
