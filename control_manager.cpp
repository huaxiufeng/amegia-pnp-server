// Copyright (c) 2012-2013 Hua Xiufeng <huaxiufeng@kaisquare.com.cn>
// Copyright (c) 2013 KAI Square Software Limited
// All rights reserved
//
// Author: Hua Xiufeng

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "message_types.h"
#include "global_config.h"
#include "gloghelper.h"
#include "control_manager.h"
using namespace std;

static void handle_get_parameter_command(camera_context *context)
{
  const char *data = context->m_control_buffer_queue->top();
  int current_size = context->m_control_buffer_queue->size();

  char recv_buffer[MAX_BUFF_SIZE], send_buffer[MAX_BUFF_SIZE];
  int recv_len = 0, send_len = 0;
  memset(recv_buffer, 0, sizeof(recv_buffer));
  memset(send_buffer, 0, sizeof(send_buffer));

  IoctlMsg *recv_message = (IoctlMsg*)data;
  int message_full_size = 4 + recv_message->size;
  if (current_size < message_full_size) {
    return;
  }
  context->m_control_buffer_queue->pop(recv_buffer, message_full_size);
  recv_len = message_full_size;
  IoctlMsg *send_message = (IoctlMsg*)send_buffer;
  send_message->magic[0] = '$';
  send_message->magic[1] = '\0';
  send_message->ioctlCmd = IOCTL_GET_PARAMETER_REQ;
  IoctlMsgGetParameterReq *resp = (IoctlMsgGetParameterReq*)send_message->data;
  const char* param_request = "group=all";
  strcpy(resp->result, param_request);
	resp->total = resp->count = strlen (param_request);
	resp->index = 0;
	resp->endflag = 1;
	send_len = sizeof(IoctlMsg)+sizeof(IoctlMsgGetParameterReq)+strlen(param_request);
  send_message->size = send_len - 4;

  write(context->m_control_fd, send_buffer, send_len);
}

static void handle_set_rtspserver_command(camera_context *context);
static void handle_get_parameter_response(camera_context *context)
{
  const char *data = context->m_control_buffer_queue->top();
  int current_size = context->m_control_buffer_queue->size();
  IoctlMsg *recv_message = (IoctlMsg*)data;
  int message_full_size = 4 + recv_message->size;

  int current_parameter_size = 0, total_parameter_size = 0, end_flag = 0;
  char parameter[64*1024];
  memset(parameter, 0 ,sizeof(parameter));
  int last_index = -1;
  for (int index = 0; index < current_size-1; index++) {
    if (current_size-index < sizeof(IoctlMsg)+sizeof(IoctlMsgGetParameterResp)) {
      break;
    }
    if (data[index] == '$' && data[index+1] == '\0') {
      IoctlMsgGetParameterResp *response = (IoctlMsgGetParameterResp*)(data+index+sizeof(IoctlMsg));
      if (index+sizeof(IoctlMsg)+sizeof(IoctlMsgGetParameterResp)+response->count > current_size) {
        break;
      }
      if (response->index != last_index+1) {
        end_flag = 1;
        break;
      }
      if (0 == total_parameter_size) total_parameter_size = response->total;
      memcpy(parameter+current_parameter_size, response->result, response->count);
      current_parameter_size += response->count;
      end_flag = response->endflag;
      //LOG(INFO)<<"index:"<<(int)response->index<<" end:"<<(int)response->endflag<<" ["<<current_parameter_size<<"/"<<(int)response->total<<"]"<<endl;
      if (current_parameter_size >= total_parameter_size || end_flag) {
        //LOG(INFO)<<"["<<context->m_conn_ip<<" --> localhost.fd="<<context->m_control_fd<<"]"<<" get parameter "<<current_parameter_size<<" bytes, total "<<total_parameter_size<<endl;
        break;
      }
      index = index + sizeof(IoctlMsg) + sizeof(IoctlMsgGetParameterResp) + response->count - 1;
      last_index = response->index;
    }
  }
  if (current_parameter_size >= total_parameter_size && end_flag) {
    char* pstr = NULL;
    if (pstr = strstr(parameter, "network.hwaddress")) {
      char mac[24];
      sscanf(pstr, "network.hwaddress='%s';", mac);
      mac[17] = 0;
      context->m_camera_mac = mac;
      LOG(INFO)<<"get mac "<<mac<<endl;
    }
    // we only want stream v03, the corresponding profile is profile_2
    if (pstr = strstr(parameter, "profile_2.framerate")) {
      sscanf(pstr, "profile_2.framerate=%lf;", &context->m_rtsp_profile_frame_rate);
      LOG(INFO)<<"get frame rate from profile "<<context->m_rtsp_profile_frame_rate<<endl;
    }
    context->m_control_buffer_queue->clear();

    handle_set_rtspserver_command(context);
  }
}

static void handle_set_rtspserver_command(camera_context *context)
{
  char send_buffer[MAX_BUFF_SIZE];
  int send_len = 0;
  memset(send_buffer, 0, sizeof(send_buffer));

  IoctlMsg *send_message = (IoctlMsg*)send_buffer;
  memset(send_buffer, 0, sizeof(send_buffer));
  send_len = sizeof(IoctlMsg)+sizeof(IoctlMsgSetRtspServerReq);
  send_message->magic[0] = '$';
  send_message->magic[1] = '\0';
  send_message->ioctlCmd = IOCTL_SET_RTSPSERVER_REQ;
  IoctlMsgSetRtspServerReq *resp = (IoctlMsgSetRtspServerReq*)send_message->data;
  strcpy(resp->server.name, g_local_address);
  resp->server.port = context->m_rtsp_port;
  resp->server.type = resp->server.err = 0;
  send_message->size = send_len - 4;

  if (g_stream_callback) {
    write(context->m_control_fd, send_buffer, send_len);
  }
}

static void handle_set_rtspserver_response(camera_context *context)
{
  const char *data = context->m_control_buffer_queue->top();
  int current_size = context->m_control_buffer_queue->size();

  char recv_buffer[MAX_BUFF_SIZE];
  int recv_len = 0;
  memset(recv_buffer, 0, sizeof(recv_buffer));

  IoctlMsg *recv_message = (IoctlMsg*)data;
  int message_full_size = 4 + recv_message->size;
  if (current_size < message_full_size) {
    return;
  }
  context->m_control_buffer_queue->pop(recv_buffer, message_full_size);
  recv_len = message_full_size;
  IoctlMsgSetRtspServerResp *resp = (IoctlMsgSetRtspServerResp*)recv_message->data;
  //LOG(INFO)<<"get set rtspserver response from camera, result: "<<resp->result<<endl;
}

static void handle_set_snapshotserver_command(camera_context *context)
{
  const char *data = context->m_control_buffer_queue->top();
  int current_size = context->m_control_buffer_queue->size();

  char recv_buffer[MAX_BUFF_SIZE], send_buffer[MAX_BUFF_SIZE];
  int recv_len = 0, send_len = 0;
  memset(recv_buffer, 0, sizeof(recv_buffer));
  memset(send_buffer, 0, sizeof(send_buffer));

  IoctlMsg *recv_message = (IoctlMsg*)data;
  int message_full_size = 4 + recv_message->size;
  if (current_size < message_full_size) {
    return;
  }
  context->m_control_buffer_queue->pop(recv_buffer, message_full_size);
  recv_len = message_full_size;

  IoctlMsg *send_message = (IoctlMsg*)send_buffer;
  memset(send_buffer, 0, sizeof(send_buffer));
  send_len = sizeof(IoctlMsg)+sizeof(IoctlMsgGetSnapshotServerResp)+sizeof(stServerDef);;

  send_message->magic[0] = '$';
  send_message->magic[1] = '\0';
  send_message->ioctlCmd = IOCTL_GET_SNAPSHOTSERVER_RESP;
  IoctlMsgGetSnapshotServerResp *resp = (IoctlMsgGetSnapshotServerResp*)send_message->data;
  resp->interval = g_snapshot_interval;
  resp->number = 1;
  stServerDef *server = resp->servers;
  strcpy(server->name, g_local_address);
  server->port = context->m_snapshot_port;
  server->type = server->err = 0;
  send_message->size = send_len - 4;

  if (g_snapshot_callback) {
    write(context->m_control_fd, send_buffer, send_len);
  }
}

int control_manager::handle_accept_connection(void *arg, int listener, const char *_type)
{
  camera_context *ctx = (camera_context*)arg;
  if (!ctx || ctx->m_control_listen_fd <= 0) return -1;
  if (ctx->m_control_fd > 0) return ctx->m_control_fd;

  int fd = general_manager::handle_accept_connection(ctx, ctx->m_control_listen_fd, _type);
  if (fd <= 0) return fd;

  ctx->m_control_fd = fd;
  return fd;
}

int control_manager::handle_read_buffer(void *arg)
{
  camera_context *ctx = (camera_context*)arg;
  if (!ctx || ctx->m_control_fd <= 0) return -1;

  int fd = ctx->m_control_fd;
  int res = general_manager::check_readable(fd, 10*1000);
  if (res <= 0) {
    return res;
  }
  char recv_buffer[MAX_BUFF_SIZE];
  memset(recv_buffer, 0, sizeof(recv_buffer));
  int recv_len = read(fd, recv_buffer, sizeof(recv_buffer));
  if (recv_len <= 0) {
    return recv_len;
  }

  ctx->m_control_buffer_queue->push(recv_buffer, recv_len);
  const char *data = ctx->m_control_buffer_queue->top();
  IoctlMsg *recv_message = (IoctlMsg*)data;

  switch (recv_message->ioctlCmd) {
  case IOCTL_KEEP_ALIVE:
    //LOG(INFO)<<"IOCTL_KEEP_ALIVE "<<fd<<endl;
    handle_keep_alive_command(ctx->m_control_buffer_queue, &(ctx->m_control_update_time));
    break;
  case IOCTL_CAM_HELO:
    //LOG(INFO)<<"IOCTL_CAM_HELO "<<fd<<endl;
    handle_get_parameter_command(ctx);
    break;
  case IOCTL_GET_PARAMETER_RESP:
    //LOG(INFO)<<"IOCTL_GET_PARAMETER_RESP "<<fd<<endl;
    handle_get_parameter_response(ctx);
    break;
  case IOCTL_SET_RTSPSERVER_RESP:
    //LOG(INFO)<<"IOCTL_SET_RTSPSERVER_RESP "<<fd<<endl;
    handle_set_rtspserver_response(ctx);
    break;
  case IOCTL_GET_SNAPSHOTSERVER_REQ:
    //LOG(INFO)<<"IOCTL_GET_SNAPSHOTSERVER_REQ "<<fd<<endl;
    handle_set_snapshotserver_command(ctx);
    break;
  default:
    //LOG(INFO)<<"UNRECOGNIZED COMMAND "<<fd<<hex<<" 0X"<<recv_message->ioctlCmd<<endl;
    handle_unrecognized_command(ctx->m_control_buffer_queue);
    break;
  }
}
