// Copyright (c) 2012-2013 Hua Xiufeng <huaxiufeng@kaisquare.com.cn>
// Copyright (c) 2013 KAI Square Software Limited
// All rights reserved
//
// Author: Hua Xiufeng

#ifndef AMEGIA_PNP_SERVER_GLOBAL_CONFIG_H
#define AMEGIA_PNP_SERVER_GLOBAL_CONFIG_H

#include "amegia_pnp_sdk.h"

#define MAX_BUFF_SIZE          8192
#define MAX_VIDEO_FRAME_SIZE   65536
#define MAX_SNAPSHOT_SIZE      1048576

extern char g_local_address[128];
extern int g_account_server_port;
extern int g_control_server_port;
extern int g_rtsp_server_port;
extern int g_snapshot_server_port;

extern fvideo_stream_callback g_stream_callback;
extern fvideo_snapshot_callback g_snapshot_callback;

#endif // AMEGIA_PNP_SERVER_GLOBAL_CONFIG_H
