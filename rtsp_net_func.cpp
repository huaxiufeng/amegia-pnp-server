#include <event2/event.h>
#include <event2/bufferevent.h>
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

int rtsp_write(evutil_socket_t fd, const char *method, const char *url, const char *msg, int seq)
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
  //LOG(INFO)<<"rtsp write "<<ret<<" bytes, "<<buf<<endl;
  return ret;
}

int rtsp_read(evutil_socket_t fd, char *buf, int size)
{
  int  ret = 0;
  ret = read(fd, buf, size);
  if(ret > 0) {
    buf[ret] = 0;
  }
  //LOG(INFO)<<"rtsp read "<<ret<<" bytes, "<<buf<<endl;
  return ret;
}
