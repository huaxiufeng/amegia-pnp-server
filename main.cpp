// Copyright (c) 2012-2013 Hua Xiufeng <huaxiufeng@kaisquare.com.cn>
// Copyright (c) 2013 KAI Square Software Limited
// All rights reserved
//
// Author: Hua Xiufeng

#include <stdio.h>
#include <string.h>
#include "gloghelper.h"
#include "amegia_pnp_sdk.h"

void stream_callback(const char *_ip, const char *_mac, const unsigned char *_frame_buffer, int _frame_buffer_size)
{
  //LOG(INFO)<<"["<<_mac<<"-"<<_ip<<"] get rtsp frame "<<_frame_buffer_size<<" bytes"<<endl;
  //FILE *fp = fopen("video.h264", "ab");
  //fwrite(_frame_buffer, _frame_buffer_size, 1, fp);
  //fclose(fp);
}

void snapshot_callback(const char *_ip, const char *_mac, const unsigned char *_snapshot_buffer, int _snapshot_buffer_size)
{
  LOG(INFO)<<"["<<_mac<<"-"<<_ip<<"] get snapshot "<<_snapshot_buffer_size<<" bytes"<<endl;
  /*
  string filename = string(_mac) + ".jpg";
  FILE *fp = fopen(filename.c_str(), "wb");
  fwrite(_snapshot_buffer, _snapshot_buffer_size, 1, fp);
  fclose(fp);
  */
}

void connection_callback(const char *_ip, const char *_mac, int _fd, bool _connected)
{
  LOG(INFO)<<"["<<_mac<<"-"<<_ip<<" -- localhost.fd="<<_fd<<"] connection status changed to "<<_connected<<endl;
}

int main()
{
  gloghelper::get_instance();

  amegia_pnp_context config;
  strcpy(config.local_ip_address, "10.101.10.189");
  config.account_service_port = 10000;
  config.http_service_port = 9080;
  config.snapshot_interval = 10;
  config.snapshot_keep_days = 1;
  config.snapshot_begin_hour = 0;
  config.snapshot_begin_minute = 0;
  config.snapshot_end_hour = 16;
  config.snapshot_end_minute = 0;
  strcpy(config.snapshot_directory, "snapshot_file");

  start_service(&config, stream_callback, snapshot_callback, connection_callback);

  sleep(90000);
  LOG(INFO)<<"exit main now"<<endl;

  return 0;
}
