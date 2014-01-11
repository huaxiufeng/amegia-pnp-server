// Copyright (c) 2012-2013 Hua Xiufeng <huaxiufeng@kaisquare.com.cn>
// Copyright (c) 2013 KAI Square Software Limited
// All rights reserved
//
// Author: Hua Xiufeng

#include "global_config.h"

char g_local_address[16] = "10.101.10.189";
int g_account_server_port = 10000;
int g_http_server_port = 9080;

int g_snapshot_interval = 10;
char g_snapshot_directory[512] = "";

fstream_callback g_stream_callback = 0;
fsnapshot_callback g_snapshot_callback = 0;
fconnection_callback g_connection_callback = 0;
