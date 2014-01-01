// Copyright (c) 2012-2013 Hua Xiufeng <huaxiufeng@kaisquare.com.cn>
// Copyright (c) 2013 KAI Square Software Limited
// All rights reserved
//
// Author: Hua Xiufeng

#ifndef AMEGIA_PNP_SERVER_ACCOUNT_MANAGER_H
#define AMEGIA_PNP_SERVER_ACCOUNT_MANAGER_H

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <sys/time.h>
#include <pthread.h>
#include <string>
#include <map>

#include "general_manager.h"

class account_manager: public general_manager {
public:
  static account_manager* get_instance() {
    static account_manager instance;
    return &instance;
  }
  std::string get_name() {return "account";}
  void read_event_callback(struct bufferevent *bev, void *arg);
};

#endif // AMEGIA_PNP_SERVER_ACCOUNT_MANAGER_H
