// Copyright (c) 2012-2013 Hua Xiufeng <huaxiufeng@kaisquare.com.cn>
// Copyright (c) 2013 KAI Square Software Limited
// All rights reserved
//
// Author: Hua Xiufeng

#include "global_config.h"
#include "amegia_pnp_sdk.h"

char g_local_address[128] = "10.101.10.189";
int g_account_server_port = 10000;
int g_control_server_port = 10001;
int g_rtsp_server_port = 10002;
int g_snapshot_server_port = 10003;

fvideo_stream_callback g_stream_callback = 0;
fvideo_snapshot_callback g_snapshot_callback = 0;
