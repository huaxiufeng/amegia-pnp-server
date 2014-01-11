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
#include "rtsp_manager.h"
#include "rtsp_net_func.h"
#include "rtp_net_func.h"
using namespace std;

static void handle_connect_rtsp_command(camera_context *context)
{
  const char *data = context->m_rtsp_buffer_queue->top();
  int current_size = context->m_rtsp_buffer_queue->size();
  int header_size = sizeof(IoctlMsg) + sizeof(IoctlMsgNormal);
  int rtsp_cseq = 0;
  for (int i = current_size-1; i >= 0; i--) {
    const char *pointer = strstr(data+i, "CSeq: ");
    if (pointer) {
      sscanf(pointer, "CSeq: %d", &rtsp_cseq);
      break;
    }
  }
  int fd = context->m_rtsp_fd;

  if (0 == rtsp_cseq) {
    rtsp_write(fd, "OPTIONS", "rtsp://10.101.10.189/v05", NULL, rtsp_cseq+1);
  }
  if (1 == rtsp_cseq) {
    rtsp_write(fd, "DESCRIBE", "rtsp://10.101.10.189/v05", "Accept: application/sdp\r\n", rtsp_cseq+1);
  }
  if (2 == rtsp_cseq && strstr(data+header_size, "a=control")) {
    char result[256] = {0};
    memset(result, 0, sizeof(result));
    if (find_value("range:", data+header_size, result)) {
      context->m_stream_range = result;
    }
    memset(result, 0, sizeof(result));
    const char *pointer = strstr(data+header_size, "m=video");
    if (find_value("a=control:", pointer, result)) {
      context->m_video_track = result;
      if (pointer = strstr(result, "trackID=")) {
        sscanf(pointer, "trackID=%d", &(context->m_video_track_id));
      }
    }
    memset(result, 0, sizeof(result));
    pointer = strstr(data+header_size, "m=audio");
    if (find_value("a=control:", pointer, result)) {
      context->m_audio_track = result;
      if (pointer = strstr(result, "trackID=")) {
        sscanf(pointer, "trackID=%d", &(context->m_audio_track_id));
      }
    }
    if (context->m_video_track.length() > 0) {
      rtsp_write(fd, "SETUP", context->m_video_track.c_str(), "Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n", rtsp_cseq+1);
    }
  }
  if (3 == rtsp_cseq && strstr(data+header_size, "Session:")) {
    char result[256];
    memset(result, 0, sizeof(result));
    if (find_value("Session: ", data+header_size, result)) {
      context->m_stream_session = result;
    }
    if (context->m_stream_session.length() > 0 && context->m_stream_range.length() > 0) {
      snprintf(result, sizeof(result), "Session:%s\r\nRange:%s\r\n", context->m_stream_session.c_str(), context->m_stream_range.c_str());
      rtsp_write(fd, "PLAY", "rtsp://10.101.10.189/v05", result, rtsp_cseq+1);
    }
  }
  if (4 == rtsp_cseq) {
    context->m_rtsp_buffer_queue->clear();
  }
}

static void handle_rtp_stream_response(camera_context *context)
{
  // if haven't get mac from camera, just disconnect it and let it connect again
  if (context->m_camera_mac.length() == 0) {
    context->m_connected = false;
    return;
  }
  do {
    const unsigned char *data = (const unsigned char *)context->m_rtsp_buffer_queue->top();
    int current_size = context->m_rtsp_buffer_queue->size();
    if (current_size < 4) {
      break;
    }
    char magic = data[0];
    int channel = data[1];
    int rtp_length = (data[2]<<8) | data[3];
    //LOG(INFO)<<"magic "<<magic<<", channel "<<channel<<", rtp packet size: "<<rtp_length<<", current queue size: "<<current_size<<endl;
    if (current_size < rtp_length + 4) {
      break;
    }

    int recv_len = rtp_length + 4;
    char recv_buffer[MAX_VIDEO_FRAME_SIZE];
    char frame_buffer[MAX_VIDEO_FRAME_SIZE];
    memset(recv_buffer, 0, recv_len);
    //LOG(INFO)<<" rtsp_buf_size:"<<context->m_rtsp_buffer_queue->size()<<"/"<<context->m_rtsp_buffer_queue->capacity()<<endl;
    //LOG(INFO)<<"rtsp pop:"<<recv_len<<endl;
    context->m_rtsp_buffer_queue->pop(recv_buffer, recv_len);
    //LOG(INFO)<<" rtsp_buf_size:"<<context->m_rtsp_buffer_queue->size()<<"/"<<context->m_rtsp_buffer_queue->capacity()<<endl;
    //LOG(INFO)<<"get one frame! "<<recv_len<<", current queue size: "<<context->m_rtsp_buffer_queue->size()<<endl;
    rtsp_manager::get_instance()->handle_keep_alive_command(NULL, &(context->m_rtsp_update_time));
    int frame_size = rtp_unpackage(recv_buffer+4, recv_len-4, frame_buffer, sizeof(frame_buffer), context->m_fragmentation_units);
    //LOG(INFO)<<"rtp_unpackage get "<<frame_size<<" bytes"<<endl;
    context->m_rtsp_frame_count++;
    if (g_stream_callback && frame_size > 0) {
      (*g_stream_callback)(context->m_conn_ip.c_str(), context->m_camera_mac.c_str(), (const unsigned char*)frame_buffer, frame_size);
    }
  } while (true);
}

int rtsp_manager::handle_accept_connection(void *arg, int listener, const char *_type)
{
  camera_context *ctx = (camera_context*)arg;
  if (!ctx || ctx->m_rtsp_listen_fd <= 0) return -1;
  if (ctx->m_rtsp_fd > 0) return ctx->m_rtsp_fd;

  int fd = general_manager::handle_accept_connection(ctx, ctx->m_rtsp_listen_fd, _type);
  if (fd <= 0) return fd;

  ctx->m_rtsp_fd = fd;
  return fd;
}

int rtsp_manager::handle_read_buffer(void *arg)
{
  camera_context *ctx = (camera_context*)arg;
  if (!ctx || ctx->m_rtsp_fd <= 0) return -1;

  int fd = ctx->m_rtsp_fd;

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

  ctx->m_rtsp_buffer_queue->push(recv_buffer, recv_len);
  const char *data = ctx->m_rtsp_buffer_queue->top();
  IoctlMsg *recv_message = (IoctlMsg*)data;

  switch (recv_message->ioctlCmd) {
  case IOCTL_RTSP_READY:
    //LOG(INFO)<<"IOCTL_RTSP_READY "<<fd<<endl;
    handle_connect_rtsp_command(ctx);
    break;
  default:
    //LOG(INFO)<<"RTSP_STREAM "<<fd<<endl;
    handle_rtp_stream_response(ctx);
    break;
  }
}
