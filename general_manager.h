#ifndef AMEGIA_PNP_SERVER_GENERAL_MANAGER_H
#define AMEGIA_PNP_SERVER_GENERAL_MANAGER_H

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <sys/time.h>
#include <pthread.h>
#include <map>
// Copyright (c) 2012-2013 Hua Xiufeng <huaxiufeng@kaisquare.com.cn>
// Copyright (c) 2013 KAI Square Software Limited
// All rights reserved
//
// Author: Hua Xiufeng

#include "buffer_queue.h"

typedef BufferQueue<char> buffer_queue;

class general_context {
public:
  general_context();
  general_context(const general_context &ctx);
  general_context& operator= (const general_context &ctx);
  ~general_context();

  std::string m_conn_ip;
  uint32_t m_conn_port;
  std::string m_camera_ip;
  std::string m_camera_mac;
  struct timeval m_create_time;
  struct timeval m_update_time;
  buffer_queue* m_buffer_queue;
  // rtsp only
  int m_video_track_id;
  int m_audio_track_id;
  std::string m_video_track;
  std::string m_audio_track;
  std::string m_stream_session;
  std::string m_stream_range;
};

typedef std::map<evutil_socket_t, general_context> general_table_type;
typedef std::map<evutil_socket_t, string> mac_table_type;

class general_manager {
public:
  virtual std::string get_name() {return "general";}
  bool add(evutil_socket_t _fd, const general_context& _info);
  general_context* get(evutil_socket_t _fd);
  const char* get_mac(evutil_socket_t _fd);
  bool keep_alive(evutil_socket_t _fd);
  bool remove(evutil_socket_t _fd);
  virtual void read_event_callback(struct bufferevent *bev, void *arg);
protected:
  general_manager() {pthread_mutex_init(&m_lock, NULL);}
  virtual ~general_manager() {pthread_mutex_destroy(&m_lock);}
  void lock() {pthread_mutex_lock(&m_lock);}
  void unlock() {pthread_mutex_unlock(&m_lock);}

  pthread_mutex_t m_lock;
  general_table_type m_table;
  mac_table_type m_mac_table;
};

#endif // AMEGIA_PNP_SERVER_GENERAL_MANAGER_H
