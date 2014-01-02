// Copyright (c) 2012-2013 Hua Xiufeng <huaxiufeng@kaisquare.com.cn>
// Copyright (c) 2013 KAI Square Software Limited
// All rights reserved
//
// Author: Hua Xiufeng

#include <stdio.h>
#include "gloghelper.h"
#include "amegia_pnp_sdk.h"

void stream_callback(const char *_mac, const unsigned char *_frame_buffer, int _frame_buffer_size)
{
  //LOG(INFO)<<_mac<<" hi, I got a "<<_frame_buffer_size<<" bytes frame"<<endl;
  //FILE *fp = fopen("video.h264", "ab");
  //fwrite(_frame_buffer, _frame_buffer_size, 1, fp);
  //fclose(fp);
}

void snapshot_callback(const char *_mac, const unsigned char *_snapshot_buffer, int _snapshot_buffer_size)
{
  LOG(INFO)<<_mac<<" hi, I got a "<<_snapshot_buffer_size<<" bytes snapshot"<<endl;
//  FILE *fp = fopen("snapshot.jpg", "wb");
//  fwrite(_snapshot_buffer, _snapshot_buffer_size, 1, fp);
//  fclose(fp);
}

int main()
{
  gloghelper::get_instance();

  amegia_pnp_context config;
  strcpy(config.local_ip_address, "10.101.10.189");
  config.account_service_port = 10000;
  config.control_service_port = 10001;
  config.rtsp_service_port = 10002;
  config.snapshot_service_port = 10003;

  start_service(&config, stream_callback, snapshot_callback);

  return 0;
}
