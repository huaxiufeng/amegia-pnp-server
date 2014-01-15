// Copyright (c) 2012-2013 Hua Xiufeng <huaxiufeng@kaisquare.com.cn>
// Copyright (c) 2013 KAI Square Software Limited
// All rights reserved
//
// Author: Hua Xiufeng

#ifndef AMEGIA_PNP_SERVER_GLOBAL_CONFIG_H
#define AMEGIA_PNP_SERVER_GLOBAL_CONFIG_H

#include "amegia_pnp_sdk.h"

#define MAX_BUFF_SIZE          1024
#define MAX_VIDEO_FRAME_SIZE   65536
#define MAX_SNAPSHOT_SIZE      1048576

extern char g_local_address[16];
extern int g_account_server_port;
extern int g_http_server_port;

extern int g_snapshot_interval;
extern char g_snapshot_directory[512];
extern int g_snapshot_keep_days;
extern int g_snapshot_begin_hour;
extern int g_snapshot_begin_minute;
extern int g_snapshot_end_hour;
extern int g_snapshot_end_minute;

extern fstream_callback g_stream_callback;
extern fsnapshot_callback g_snapshot_callback;
extern fconnection_callback g_connection_callback;

#endif // AMEGIA_PNP_SERVER_GLOBAL_CONFIG_H
