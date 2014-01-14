// Copyright (c) 2012-2013 Hua Xiufeng <huaxiufeng@kaisquare.com.cn>
// Copyright (c) 2013 KAI Square Software Limited
// All rights reserved
//
// Author: Hua Xiufeng

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "global_config.h"
#include "gloghelper.h"
#include "general_manager.h"
#include "simple_echo_server.h"

static pthread_t echo_server_thread;
static bool echo_server_alive = true;

static void* start_echo_server0(void *arg)
{
  int listener = socket(AF_INET, SOCK_STREAM, 0);
  if (listener <= 0) {
    LOG(ERROR)<<"[echo server]"<<" create socket error: "<<strerror(errno)<<", listener = "<<listener<<endl;
    return NULL;
  }

  int flags = fcntl(listener, F_GETFL, 0);
  fcntl(listener, F_SETFL, flags | O_NONBLOCK);
  int reuse = 1;
  setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

  struct sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = htonl(INADDR_ANY);
  sin.sin_port = htons(g_http_server_port);
  bzero(&(sin.sin_zero), 8);
  socklen_t slen = sizeof(sin);
  if (bind(listener, (struct sockaddr *)&sin, slen) < 0) {
    LOG(ERROR)<<"[echo server]"<<" bind "<<g_http_server_port<<" error: "<<strerror(errno)<<endl;
    return NULL;
  }

  if (listen(listener, 64) < 0) {
    LOG(ERROR)<<"[echo server]"<<" listen error: "<<strerror(errno)<<endl;
    return NULL;
  }

  while (echo_server_alive) {
    int fd = accept(listener, (struct sockaddr *)&sin, &slen);
    if (fd < 0) {
      sleep(1);
      continue;
    }
    string ip = inet_ntoa(sin.sin_addr);
    uint32_t port = ntohs(sin.sin_port);

    char recv_buffer[MAX_BUFF_SIZE];
    memset(recv_buffer, 0, sizeof(recv_buffer));
    int recv_len = read(fd, recv_buffer, sizeof(recv_buffer));

    string report = context_manager::get_instance()->report();
    if (strstr(recv_buffer, "GET") || strstr(recv_buffer, "get")) {
      // this is a http request
      char *resp_buffer = new char[report.length()+128];
      sprintf(resp_buffer,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %d\r\n"
        "\r\n"
        "%s",
        report.length(), report.c_str());
      write(fd, resp_buffer, strlen(resp_buffer));
      delete []resp_buffer;
    } else {
      // this is a telnet request
      write(fd, report.c_str(), report.length()+1);
    }
    close(fd);
  }

  close(listener);
  return NULL;
}

void start_echo_server()
{
  pthread_create(&echo_server_thread, NULL, start_echo_server0, NULL);
}

void stop_echo_server()
{
  echo_server_alive = false;
  pthread_join(echo_server_thread, NULL);
}
