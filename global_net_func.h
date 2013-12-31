// Copyright (c) 2012-2013 Hua Xiufeng <huaxiufeng@kaisquare.com.cn>
// Copyright (c) 2013 KAI Square Software Limited
// All rights reserved
//
// Author: Hua Xiufeng

#ifndef AMEGIA_PNP_SERVER_GLOBAL_NET_FUNC_H
#define AMEGIA_PNP_SERVER_GLOBAL_NET_FUNC_H

#include <event2/event.h>
#include <event2/bufferevent.h>

extern int read_event_buffer(struct bufferevent *bev, char *buf, int len);

extern int write_event_buffer(struct bufferevent *bev, const char *buf, int len);

#endif // AMEGIA_PNP_SERVER_GLOBAL_NET_FUNC_H
