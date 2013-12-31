// Copyright (c) 2012-2013 Hua Xiufeng <huaxiufeng@kaisquare.com.cn>
// Copyright (c) 2013 KAI Square Software Limited
// All rights reserved
//
// Author: Hua Xiufeng

#include "global_net_func.h"
#include <stdio.h>

int read_event_buffer(struct bufferevent *bev, char *buf, int len)
{
  int recv_len_total = 0;
  int recv_len_once = 0;
  int recv_max_len = len;
  while (recv_len_once = bufferevent_read(bev, buf+recv_len_total, recv_max_len-recv_len_total), recv_len_once > 0) {
    recv_len_total += recv_len_once;
    if (recv_len_once <= 0 || recv_len_total >= recv_max_len) {
      break;
    }
  }
  return recv_len_total;
}

int write_event_buffer(struct bufferevent *bev, const char *buf, int len)
{
  int send_len_total = 0;
  int send_len_once = 0;
  int send_max_len = len;
  while (send_len_once = bufferevent_write(bev, buf+send_len_total, send_max_len-send_len_total), send_len_once > 0) {
    send_len_total += send_len_once;
    if (send_len_once <= 0 || send_len_total >= send_max_len) {
      break;
    }
  }
  return send_len_total;
}
