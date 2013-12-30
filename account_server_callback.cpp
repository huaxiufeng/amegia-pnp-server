#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "message_types.h"
#include "global_config.h"
#include "global_net_func.h"
#include "gloghelper.h"
#include "account_manager.h"
#include "account_server_callback.h"
using namespace std;

static void handle_keep_alive_command(struct bufferevent *bev)
{
  evutil_socket_t fd = bufferevent_getfd(bev);
  account_context context;
  if (!account_manager::get_instance()->get_account(fd, context)) return;
}

static void handle_get_controlserver_command(struct bufferevent *bev, const char *_buf, int _len)
{
  evutil_socket_t fd = bufferevent_getfd(bev);
  account_context context;
  if (!account_manager::get_instance()->get_account(fd, context)) return;

  char send_buffer[MAX_BUFF_SIZE];
  IoctlMsg *send_message = (IoctlMsg*)send_buffer;
  memset(send_buffer, 0, sizeof(send_buffer));
  int send_len = sizeof(IoctlMsg)+sizeof(IoctlMsgGetControlServerResp)+sizeof(stServerDef);

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
  account_context context;
  if (!account_manager::get_instance()->get_account(fd, context)) return;
}

void account_accept_cb(evutil_socket_t listener, short event, void *arg)
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

  account_manager::get_instance()->add_account(fd, ip.c_str(), port);

  struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
  bufferevent_setcb(bev, account_read_cb, NULL, account_error_cb, arg);
  bufferevent_enable(bev, EV_READ|EV_WRITE|EV_PERSIST);
}

void account_write_cb(struct bufferevent *bev, void *arg) {}

void account_error_cb(struct bufferevent *bev, short event, void *arg)
{
  evutil_socket_t fd = bufferevent_getfd(bev);
  account_context context;
  if (!account_manager::get_instance()->get_account(fd, context)) return;
  string ip = context.m_ip;
  uint32_t port = context.m_port;

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

void account_read_cb(struct bufferevent *bev, void *arg)
{
  char recv_buffer[MAX_BUFF_SIZE];
  IoctlMsg *recv_message = (IoctlMsg*)recv_buffer;
  int recv_len = 0;
  memset(recv_buffer, 0, sizeof(recv_buffer));

  evutil_socket_t fd = bufferevent_getfd(bev);
  account_context context;
  if (!account_manager::get_instance()->get_account(fd, context)) return;
  string ip = context.m_ip;
  uint32_t port = context.m_port;

  recv_len = read_event_buffer(bev, recv_buffer, sizeof(recv_buffer));
  LOG(INFO)<<"["<<ip<<":"<<port<<" --> localhost.fd="<<fd<<"]"<<" read "<<recv_len<<" bytes message 0X"<<hex<<recv_message->ioctlCmd<<endl;

  switch (recv_message->ioctlCmd) {
  case IOCTL_KEEP_ALIVE:
    handle_keep_alive_command(bev);
    break;
  case IOCTL_GET_CONTROLSERVER_REQ:
    handle_get_controlserver_command(bev, recv_buffer, recv_len);
    break;
  default:
    handle_unregcognized_command(bev);
    break;
  }
}
