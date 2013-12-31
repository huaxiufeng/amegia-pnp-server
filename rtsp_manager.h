// Copyright (c) 2012-2013 Hua Xiufeng <huaxiufeng@kaisquare.com.cn>
// Copyright (c) 2013 KAI Square Software Limited
// All rights reserved
//
// Author: Hua Xiufeng

#ifndef AMEGIA_PNP_SERVER_RTSP_MANAGER_H
#define AMEGIA_PNP_SERVER_RTSP_MANAGER_H

#include "general_manager.h"

class rtsp_manager: public general_manager {
public:
  static rtsp_manager* get_instance() {
    static rtsp_manager instance;
    return &instance;
  }
};

extern void rtsp_accept_cb(evutil_socket_t listener, short event, void *arg);
extern void rtsp_read_cb(struct bufferevent *bev, void *arg);
extern void rtsp_error_cb(struct bufferevent *bev, short event, void *arg);

#endif // AMEGIA_PNP_SERVER_RTSP_MANAGER_H
