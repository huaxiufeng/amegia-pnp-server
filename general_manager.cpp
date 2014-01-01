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
  if (m_buffer_queue) delete m_buffer_queue;
}

bool general_manager::add(evutil_socket_t _fd, const general_context& _info)
{
  bool res = false;
  lock();
  m_table.insert(std::pair<evutil_socket_t, general_context>(_fd, _info));
  res = true;
  unlock();
  return res;
}

general_context* general_manager::get(evutil_socket_t _fd)
{
  general_context* res = NULL;
  lock();
  general_table_type::iterator itor = m_table.find(_fd);
  if (itor != m_table.end()) {
    res = &(itor->second);
  }
  unlock();
  return res;
}

const char* general_manager::get_mac(evutil_socket_t _fd)
{
  const char* res = NULL;
  lock();
  mac_table_type::iterator itor = m_mac_table.find(_fd);
  if (itor != m_mac_table.end()) {
    res = itor->second.c_str();
  }
  else {
    std::string mac = get_peer_mac(_fd);
    if (mac.length() > 0) {
      m_mac_table.insert(pair<evutil_socket_t,std::string>(_fd, mac));
      res = mac.c_str();
    }
  }
  unlock();
  return res;
}

bool general_manager::keep_alive(evutil_socket_t _fd)
{
  bool res = false;
  lock();
  general_table_type::iterator itor = m_table.find(_fd);
  if (itor != m_table.end()) {
    gettimeofday(&(itor->second.m_update_time), NULL);
    res = true;
  }
  unlock();
  return res;
}

bool general_manager::remove(evutil_socket_t _fd)
{
  bool res = false;
  lock();
  general_table_type::iterator itor = m_table.find(_fd);
  if (itor != m_table.end()) {
    m_table.erase(itor);
    res = true;
  }
  unlock();
  return res;
}

void general_manager::read_event_callback(struct bufferevent *bev, void *arg)
{
  LOG(ERROR)<<"must implement this interface for subclass"<<endl;
}
