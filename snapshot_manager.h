// Copyright (c) 2012-2013 Hua Xiufeng <huaxiufeng@kaisquare.com.cn>
// Copyright (c) 2013 KAI Square Software Limited
// All rights reserved
//
// Author: Hua Xiufeng

#ifndef AMEGIA_PNP_SERVER_SNAPSHOT_MANAGER_H
#define AMEGIA_PNP_SERVER_SNAPSHOT_MANAGER_H

#include "general_manager.h"

class snapshot_manager: public general_manager {
public:
  static snapshot_manager* get_instance() {
    static snapshot_manager instance;
    return &instance;
  }
  std::string get_name() {return "snapshot";}
  void read_event_callback(struct bufferevent *bev, void *arg);
};

#endif // AMEGIA_PNP_SERVER_SNAPSHOT_MANAGER_H
