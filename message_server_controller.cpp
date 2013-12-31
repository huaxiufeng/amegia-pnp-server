#include <errno.h>
#include "global_config.h"
#include "gloghelper.h"
#include "account_manager.h"
#include "control_manager.h"
#include "rtsp_manager.h"
#include "message_server_controller.h"

message_server_controller::message_server_controller()
{
  m_event_base = event_base_new();

  add_listen_event("account-server", g_account_server_port, account_accept_cb);
  add_listen_event("control-server", g_control_server_port, control_accept_cb);
  add_listen_event("rtsp-server", g_rtsp_server_port, rtsp_accept_cb);
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

void message_server_controller::add_listen_event(const char *_type, uint32_t _port, event_callback_fn _callback)
{
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

  struct event *listen_event = event_new(m_event_base, listener, EV_READ|EV_PERSIST, _callback, (void*)m_event_base);
  event_add(listen_event, NULL);
}
