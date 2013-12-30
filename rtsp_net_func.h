#ifndef AMEGIA_PNP_SERVER_RTSP_NET_FUNC_H
#define AMEGIA_PNP_SERVER_RTSP_NET_FUNC_H

extern char* find_value(char *needle, char *buf, char *retstr);
extern int rtsp_write(evutil_socket_t fd, const char *method, const char *url, const char *msg, int seq);
extern int rtsp_read(evutil_socket_t fd, char *buf, int size);

#endif // AMEGIA_PNP_SERVER_RTSP_NET_FUNC_H
