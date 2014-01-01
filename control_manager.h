// Copyright (c) 2012-2013 Hua Xiufeng <huaxiufeng@kaisquare.com.cn>
// Copyright (c) 2013 KAI Square Software Limited
// All rights reserved
//
// Author: Hua Xiufeng

#ifndef AMEGIA_PNP_SERVER_CONTROL_MANAGER_H
#define AMEGIA_PNP_SERVER_CONTROL_MANAGER_H

#include "general_manager.h"

class control_manager: public general_manager {
public:
  static control_manager* get_instance() {
    static control_manager instance;
    return &instance;
  }
  std::string get_name() {return "control";}
  void read_event_callback(struct bufferevent *bev, void *arg);
};

#endif // AMEGIA_PNP_SERVER_CONTROL_MANAGER_H
