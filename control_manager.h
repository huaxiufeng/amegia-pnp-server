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

  int handle_accept_connection(void *arg, int listener = 0, const char *_type = "control");
  int handle_read_buffer(void *arg);
};

#endif // AMEGIA_PNP_SERVER_CONTROL_MANAGER_H
