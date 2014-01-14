// Copyright (c) 2012-2013 Hua Xiufeng <huaxiufeng@kaisquare.com.cn>
// Copyright (c) 2013 KAI Square Software Limited
// All rights reserved
//
// Author: Hua Xiufeng

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "global_config.h"
#include "amegia_pnp_sdk.h"
#include "account_manager.h"

amegia_pnp_context::amegia_pnp_context()
{
  account_service_port = 10000;
  http_service_port = 9080;
  snapshot_interval = 300;
  memset(local_ip_address, 0, sizeof(local_ip_address));
  memset(snapshot_directory, 0, sizeof(snapshot_directory));
}

static void set_configuration(const amegia_pnp_context *config)
{
  memset(g_local_address, 0, sizeof(g_local_address));
  strncpy(g_local_address, config->local_ip_address, sizeof(g_local_address)-1);
  g_account_server_port = config->account_service_port;
  g_snapshot_interval = config->snapshot_interval;
  g_http_server_port = config->http_service_port;

  char current_path[512];
  getcwd(current_path, sizeof(current_path)-1);
  memset(g_snapshot_directory, 0, sizeof(g_snapshot_directory));
  if (strlen(config->snapshot_directory) > 0) {
    if (config->snapshot_directory[0] == '/') {
      strncpy(g_snapshot_directory, config->snapshot_directory, sizeof(g_snapshot_directory)-2);
    }
    else {
      snprintf(g_snapshot_directory, sizeof(g_snapshot_directory)-2, "%s/%s", current_path, config->snapshot_directory);
    }
    if (g_snapshot_directory[strlen(g_snapshot_directory)-1] != '/') {
      strcat(g_snapshot_directory, "/");
    }
  }
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

  account_manager::get_instance()->run();

  return 0;
}

void stop_service()
{
  account_manager::get_instance()->kill();
}
