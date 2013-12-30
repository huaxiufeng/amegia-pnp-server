#include <event2/event.h>
#include <event2/bufferevent.h>
#include <string.h>
#include "gloghelper.h"
#include "rtsp_net_func.h"

char* find_value(char *needle, char *buf, char *retstr)
{
  char  *sp, *ep = NULL;
  int  len = 0;

  sp = strstr( buf, needle );
  if( sp != NULL ){
    sp  += strlen( needle );
    ep  = strchr( sp, 0xd );
    if( ep == NULL ){
      ep  = buf + strlen( buf );
    }
    if( ep == NULL )  printf ("ep==NULL\n");
    len  = ep - sp;
    memcpy( retstr, sp, len );
    retstr[ len ] = '\0';
    return  ep;
  }

  return NULL;
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
  LOG(INFO)<<"rtsp write "<<ret<<" bytes, "<<buf<<endl;
  return ret;
}

int rtsp_read(evutil_socket_t fd, char *buf, int size)
{
  int  ret = 0;
  ret = read(fd, buf, size);
  if(ret > 0) {
    buf[ret] = 0;
  }
  LOG(INFO)<<"rtsp read "<<ret<<" bytes, "<<buf<<endl;
  return ret;
}

/*
int RTSP_Write( RTSP_CLIENT_INFO *rSock, char *method, char *url, char *msg ){
  char     buf[ 256 ];
  int    ret;

  sprintf( buf, "%s %s RTSP/1.0\r\nCSeq: %d\r\n", method, url, rSock->m_cseq++);

  strcat( buf, "User-Agent: Jack/1.0 (gogo)\r\n");

  if( msg != NULL ){
    strcat( buf, msg );
  }

  strcat( buf, "\r\n" );
  ret = write( rSock->m_fd, buf, strlen( buf ) );
  if( ret < 1 ) return -1;
  //printf ("\tSendCmd RET[%d] FD[%d]\n[%s]\n", ret, rSock->m_fd, buf );
  return ret;
}

int RTSP_Read( RTSP_CLIENT_INFO *rSock, char *buf, int size ){
  int  ret;
  ret = read( rSock->m_fd, buf, size);
  if( ret < 1 ) return -1;
  buf[ ret ] = 0;
  //  printf ("\t\t%s) buf=[%s]\n", __FUNCTION__, buf );
  return ret;
}
*/
