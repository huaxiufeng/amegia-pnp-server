// Copyright (c) 2012-2013 Hua Xiufeng <huaxiufeng@kaisquare.com.cn>
// Copyright (c) 2013 KAI Square Software Limited
// All rights reserved
//
// Author: Hua Xiufeng

#ifndef AMEGIA_PNP_SERVER_ACCOUNT_MANAGER_H
#define AMEGIA_PNP_SERVER_ACCOUNT_MANAGER_H

#include "general_manager.h"

class account_manager: public general_manager {
public:
  static account_manager* get_instance() {
    static account_manager instance;
    return &instance;
  }

  void run();
  void kill();

  int handle_accept_connection(void *arg, int listener = 0, const char *_type = "control");
  int handle_read_buffer(void *arg);

protected:
  static void* start_account_service(void *arg);
  static void* start_camera_service(void *arg);
  account_manager():m_account_listen_fd(0),m_keep_running(true){}
  int m_account_listen_fd;
  bool m_keep_running;
  pthread_t m_account_thread_id;
};

#endif // AMEGIA_PNP_SERVER_ACCOUNT_MANAGER_H
