// Copyright (c) 2012-2013 Hua Xiufeng <huaxiufeng@kaisquare.com.cn>
// Copyright (c) 2013 KAI Square Software Limited
// All rights reserved
//
// Author: Hua Xiufeng

#include <string.h>
#include <pthread.h>
#include <signal.h>
#include "global_config.h"
#include "amegia_pnp_sdk.h"
#include "message_server_controller.h"

static void set_configuration(const amegia_pnp_context *config)
{
  strncpy(g_local_address, config->local_ip_address, sizeof(g_local_address));
  g_account_server_port = config->account_service_port;
  g_control_server_port = config->control_service_port;
  g_rtsp_server_port = config->rtsp_service_port;
  g_snapshot_server_port = config->snapshot_service_port;
  g_snapshot_interval = config->snapshot_interval;
}

int start_service(const amegia_pnp_context *_config,
    fstream_callback _cb_stream,
    fsnapshot_callback _cb_snapshot,
    fconnection_callback _cb_connection)
{
  set_configuration(_config);
  g_stream_callback = _cb_stream;
  g_snapshot_callback = _cb_snapshot;
  g_connection_callback = _cb_connection;
  message_server_controller::get_instance()->run();
  return 0;
}
