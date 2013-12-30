#ifndef AMEGIA_PNP_SERVER_CONTROL_SERVER_CALLBACK_H
#define AMEGIA_PNP_SERVER_CONTROL_SERVER_H

#include <event2/event.h>
#include <event2/bufferevent.h>

extern void control_accept_cb(evutil_socket_t listener, short event, void *arg);
extern void control_read_cb(struct bufferevent *bev, void *arg);
extern void control_write_cb(struct bufferevent *bev, void *arg);
extern void control_error_cb(struct bufferevent *bev, short event, void *arg);

#endif // AMEGIA_PNP_SERVER_CONTROL_SERVER_CALLBACK_H
