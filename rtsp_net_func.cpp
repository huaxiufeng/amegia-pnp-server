// Copyright (c) 2012-2013 Hua Xiufeng <huaxiufeng@kaisquare.com.cn>
// Copyright (c) 2013 KAI Square Software Limited
// All rights reserved
//
// Author: Hua Xiufeng

#include <stdio.h>
#include <string.h>
#include "gloghelper.h"
#include "rtsp_net_func.h"

const char* find_value(const char *needle, const char *buf, char *result)
{
  const char *sp = NULL, *ep = NULL, *res = NULL;
  sp = strstr((char*)buf, (char*)needle);
  if(sp != NULL) {
    sp += strlen(needle);
    ep = strchr(sp, 0xd);
    if(ep == NULL){
      ep = buf + strlen(buf);
    }
    int len = ep - sp;
    memcpy(result, sp, len);
    result[len] = '\0';
    res = ep;
  }
  return res;
}

int rtsp_write(int fd, const char *method, const char *url, const char *msg, int seq)
{
  int ret = 0;
  char buf[1024];

  snprintf(buf, sizeof(buf), "%s %s RTSP/1.0\r\nCSeq: %d\r\n", method, url, seq);
  strcat(buf, "User-Agent: KAI Square China\r\n");

  if( msg != NULL ){
    strcat(buf, msg);
  }
  strcat(buf, "\r\n");

  ret = write(fd, buf, strlen(buf));
  //LOG(INFO)<<"rtsp write "<<ret<<" bytes: "<<endl<<buf<<endl;
  return ret;
}

int rtsp_read(int fd, char *buf, int size)
{
  int  ret = 0;
  ret = read(fd, buf, size);
  if(ret > 0) {
    buf[ret] = 0;
  }
  //LOG(INFO)<<"rtsp read "<<ret<<" bytes: "<<endl<<buf<<endl;
  return ret;
}
