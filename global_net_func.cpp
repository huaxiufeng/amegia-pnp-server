// Copyright (c) 2012-2013 Hua Xiufeng <huaxiufeng@kaisquare.com.cn>
// Copyright (c) 2013 KAI Square Software Limited
// All rights reserved
//
// Author: Hua Xiufeng

#include "global_net_func.h"
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <string.h>

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

std::string get_peer_mac(int sockfd)
{
  std::string mac;
  struct arpreq arpreq;
  struct sockaddr_in dstadd_in;
  socklen_t len = sizeof(struct sockaddr_in);
  memset(&arpreq, 0, sizeof(struct arpreq));
  memset(&dstadd_in, 0, sizeof(struct sockaddr_in));
  if(getpeername( sockfd, (struct sockaddr*)&dstadd_in, &len ) < 0) {
    perror("getpeername()");
  }
  else {
    memcpy(&arpreq.arp_pa, &dstadd_in, sizeof( struct sockaddr_in));
    strcpy(arpreq.arp_dev, "eth0");
    arpreq.arp_pa.sa_family = AF_INET;
    arpreq.arp_ha.sa_family = AF_UNSPEC;
    if( ioctl(sockfd, SIOCGARP, &arpreq ) < 0) {
      perror("ioctl SIOCGARP");
    }
    else {
      const unsigned char* ptr = (const unsigned char*)arpreq.arp_ha.sa_data;
      char buffer[64];
      sprintf(buffer, "%02x:%02x:%02x:%02x:%02x:%02x", *ptr, *(ptr+1), *(ptr+2), *(ptr+3), *(ptr+4), *(ptr+5));
      mac = buffer;
    }
  }
  return mac;
};
