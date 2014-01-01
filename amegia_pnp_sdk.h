// Copyright (c) 2012-2013 Hua Xiufeng <huaxiufeng@kaisquare.com.cn>
// Copyright (c) 2013 KAI Square Software Limited
// All rights reserved
//
// Author: Hua Xiufeng

#ifndef AMEGIA_PNP_INTERFACE_H
#define AMEGIA_PNP_INTERFACE_H

class amegia_pnp_context {
public:
  char local_ip_address[16];
  int account_service_port;
  int control_service_port;
  int rtsp_service_port;
  int snapshot_service_port;
};

typedef void (*fvideo_stream_callback)(const char *_mac, const unsigned char *_frame_buffer, int _frame_buffer_size);
typedef void (*fvideo_snapshot_callback)(const char *_mac, const unsigned char *_snapshot_buffer, int _snapshot_buffer_size);

extern int start_service(const amegia_pnp_context *_config,
                         fvideo_stream_callback _cb_stream = 0,
                         fvideo_snapshot_callback _cb_snapshot = 0);

extern void stop_service();

#endif // AMEGIA_PNP_INTERFACE_H
