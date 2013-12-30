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
  void run();
  void kill();

private:
  message_server_controller();
  ~message_server_controller();
  void add_listen_event(const char *_type, uint32_t _port, event_callback_fn _callback);

private:
  struct event_base *m_event_base;
};

#endif // AMEGIA_PNP_SERVER_MESSAGE_SERVER_CONTROLLER_H
