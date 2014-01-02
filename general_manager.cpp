// Copyright (c) 2012-2013 Hua Xiufeng <huaxiufeng@kaisquare.com.cn>
// Copyright (c) 2013 KAI Square Software Limited
// All rights reserved
//
// Author: Hua Xiufeng

#include "gloghelper.h"
#include "global_net_func.h"
#include "general_manager.h"

general_context::general_context():
m_conn_port(0),m_video_track_id(0),m_audio_track_id(1)
{
  gettimeofday(&m_create_time, NULL);
  gettimeofday(&m_update_time, NULL);
  m_buffer_queue = new buffer_queue(4096);
}

general_context::general_context(const general_context &ctx)
{
  m_conn_ip = ctx.m_conn_ip;
  m_conn_port = ctx.m_conn_port;
  m_camera_ip = ctx.m_camera_ip;
  m_camera_mac = ctx.m_camera_mac;
  m_create_time = ctx.m_create_time;
  m_update_time = ctx.m_update_time;
  m_buffer_queue = NULL;
  if (ctx.m_buffer_queue) {
    m_buffer_queue = new buffer_queue(ctx.m_buffer_queue->capacity());
    *m_buffer_queue = *(ctx.m_buffer_queue);
  }
  m_video_track_id = ctx.m_video_track_id;
  m_audio_track_id = ctx.m_audio_track_id;
  m_video_track = ctx.m_video_track;
  m_audio_track = ctx.m_audio_track;
  m_stream_session = ctx.m_stream_session;
  m_stream_range = ctx.m_stream_range;
}

general_context& general_context::operator= (const general_context &ctx)
{
  if (this != &ctx) {
    m_conn_ip = ctx.m_conn_ip;
    m_conn_port = ctx.m_conn_port;
    m_camera_ip = ctx.m_camera_ip;
    m_camera_mac = ctx.m_camera_mac;
    m_create_time = ctx.m_create_time;
    m_update_time = ctx.m_update_time;
    if (m_buffer_queue) {
      delete m_buffer_queue;
      m_buffer_queue = NULL;
    }
    if (ctx.m_buffer_queue) {
      m_buffer_queue = new buffer_queue(ctx.m_buffer_queue->capacity());
      *m_buffer_queue = *(ctx.m_buffer_queue);
    }
    m_video_track_id = ctx.m_video_track_id;
    m_audio_track_id = ctx.m_audio_track_id;
    m_video_track = ctx.m_video_track;
    m_audio_track = ctx.m_audio_track;
    m_stream_session = ctx.m_stream_session;
    m_stream_range = ctx.m_stream_range;
  }
  return *this;
}

general_context::~general_context()
{
  if (m_buffer_queue) {
    delete m_buffer_queue;
  }
}

bool general_manager::add(evutil_socket_t _fd, const general_context& _info, struct bufferevent *_bev)
{
  bool res = false;
  lock();
  m_context_table.insert(std::pair<evutil_socket_t, general_context>(_fd, _info));
  m_buffevent_table.insert(std::pair<evutil_socket_t, struct bufferevent*>(_fd, _bev));
  res = true;
  unlock();
  return res;
}

general_context* general_manager::get(evutil_socket_t _fd)
{
  general_context* res = NULL;
  lock();
  general_context_table_type::iterator itor = m_context_table.find(_fd);
  if (itor != m_context_table.end()) {
    res = &(itor->second);
  }
  unlock();
  return res;
}

bool general_manager::keep_alive(evutil_socket_t _fd)
{
  bool res = false;
  lock();
  general_context_table_type::iterator itor = m_context_table.find(_fd);
  if (itor != m_context_table.end()) {
    gettimeofday(&(itor->second.m_update_time), NULL);
    res = true;
  }
  unlock();
  return res;
}

bool general_manager::remove(const char *_mac)
{
  bool res = false;
  lock();
  evutil_socket_t fd = 0;
  LOG(INFO)<<get_name()<<" remove "<<_mac<<" ..."<<endl;
  do {
    general_context_table_type::iterator itor = m_context_table.begin();
    for (; itor != m_context_table.end(); itor++) {
      if (itor->second.m_camera_mac == _mac) {
        fd = itor->first;
        LOG(INFO)<<"found fd = "<<fd<<endl;
      }
    }
    if (itor != m_context_table.end()) {
      m_context_table.erase(itor);
      res = true;
    }
  } while (0);
  do {
    buffevent_table_type::iterator itor = m_buffevent_table.find(fd);
    if (itor != m_buffevent_table.end()) {
      bufferevent_free(itor->second);
      LOG(INFO)<<"bufferevent_free for fd = "<<fd<<endl;
      m_buffevent_table.erase(itor);
    }
  } while (0);
  unlock();
  return res;
}

void general_manager::read_event_callback(struct bufferevent *bev, void *arg)
{
  LOG(ERROR)<<"must implement this interface for subclass"<<endl;
}
