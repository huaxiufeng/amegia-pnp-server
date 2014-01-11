// Copyright (c) 2012-2013 Hua Xiufeng <huaxiufeng@kaisquare.com.cn>
// Copyright (c) 2013 KAI Square Software Limited
// All rights reserved
//
// Author: Hua Xiufeng

#ifndef AMEGIA_PNP_INTERFACE_H
#define AMEGIA_PNP_INTERFACE_H

class amegia_pnp_context {
public:
  amegia_pnp_context();
  char local_ip_address[16];
  int account_service_port;
  int http_service_port;
  int snapshot_interval;
  char snapshot_directory[512];
};

typedef void (*fstream_callback)(const char *_ip, const char *_mac, const unsigned char *_frame_buffer, int _frame_buffer_size);
typedef void (*fsnapshot_callback)(const char *_ip, const char *_mac, const unsigned char *_snapshot_buffer, int _snapshot_buffer_size);
typedef void (*fconnection_callback)(const char *_ip, const char *_mac, int _fd, bool _connected);

extern int start_service(const amegia_pnp_context *_config,
                         fstream_callback _cb_stream = 0,
                         fsnapshot_callback _cb_snapshot = 0,
                         fconnection_callback _cb_connection = 0);

extern void stop_service();

#endif // AMEGIA_PNP_INTERFACE_H
