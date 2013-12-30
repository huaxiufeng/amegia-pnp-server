#ifndef AMEGIA_PNP_SERVER_ACCOUNT_SERVER_CALLBACK_H
#define AMEGIA_PNP_SERVER_ACCOUNT_SERVER_CALLBACK_H

#include <event2/event.h>
#include <event2/bufferevent.h>

extern void account_accept_cb(evutil_socket_t listener, short event, void *arg);
extern void account_read_cb(struct bufferevent *bev, void *arg);
extern void account_write_cb(struct bufferevent *bev, void *arg);
extern void account_error_cb(struct bufferevent *bev, short event, void *arg);

/** the following functions are defined static
*** can only be used by account server callback
*
static void handle_keep_alive_command(struct bufferevent *bev);
static void handle_get_controlserver_command(struct bufferevent *bev, const char *_buf, int _len);
static void handle_unregcognized_command(struct bufferevent *bev);
*/

#endif // AMEGIA_PNP_SERVER_ACCOUNT_SERVER_CALLBACK_H
