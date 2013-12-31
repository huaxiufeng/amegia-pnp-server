// Copyright (c) 2012-2013 Hua Xiufeng <huaxiufeng@kaisquare.com.cn>
// Copyright (c) 2013 KAI Square Software Limited
// All rights reserved
//
// Author: Hua Xiufeng

#include "gloghelper.h"
#include "message_server_controller.h"

int main()
{
  gloghelper::get_instance();
  message_server_controller::get_instance()->run();

  return 0;
}
