// Copyright (c) 2012-2013 Hua Xiufeng <huaxiufeng@kaisquare.com.cn>
// Copyright (c) 2013 KAI Square Software Limited
// All rights reserved
//
// Author: Hua Xiufeng

#ifndef AMEGIA_PNP_SERVER_GENERAL_MANAGER_H
#define AMEGIA_PNP_SERVER_GENERAL_MANAGER_H

#include <sys/time.h>
#include <pthread.h>
#include <string>
#include <map>
#include <vector>
#include "buffer_queue.h"

typedef BufferQueue<char> buffer_queue;

class camera_context {
public:
  camera_context(int _account_fd = 0);
public:
  // public fields
  bool m_connected;
  std::string m_conn_ip;
  std::string m_camera_ip;
  std::string m_camera_mac;
  pthread_t m_thread_id;
  // account
  int m_account_port;
  int m_account_listen_fd;
  int m_account_fd;
  time_t m_account_create_time;
  time_t m_account_update_time;
  buffer_queue* m_account_buffer_queue;
  // control
  int m_control_port;
  int m_control_listen_fd;
  int m_control_fd;
  time_t m_control_create_time;
  time_t m_control_update_time;
  buffer_queue* m_control_buffer_queue;
  // rtsp
  int m_rtsp_port;
  int m_rtsp_listen_fd;
  int m_rtsp_fd;
  time_t m_rtsp_create_time;
  time_t m_rtsp_update_time;
  buffer_queue* m_rtsp_buffer_queue;
  unsigned long m_rtsp_frame_count;
  buffer_queue* m_fragmentation_units;
  int m_video_track_id;
  int m_audio_track_id;
  std::string m_video_track;
  std::string m_audio_track;
  std::string m_stream_session;
  std::string m_stream_range;
  // snapshot
  int m_snapshot_port;
  int m_snapshot_listen_fd;
  int m_snapshot_fd;
  time_t m_snapshot_create_time;
  time_t m_snapshot_update_time;
  buffer_queue* m_snapshot_buffer_queue;
  unsigned long m_snapshot_count;
};

class context_manager {
public:
  static context_manager* get_instance() {
    static context_manager instance;
    return &instance;
  }

  void add(camera_context &context);
  camera_context* get(int _fd);
  size_t get(std::vector<int>& _fd_all);
  void remove(int _fd);
  size_t count();

  std::string report();

  void lock() {pthread_mutex_lock(&m_lock);}
  void unlock() {pthread_mutex_unlock(&m_lock);}

protected:
  context_manager() {pthread_mutex_init(&m_lock, NULL);}
  ~context_manager() {pthread_mutex_destroy(&m_lock);}

protected:
  std::map<int, camera_context> m_context_table;
  pthread_mutex_t m_lock;
};

class general_manager
{
public:
  int handle_accept_connection(void *arg, int _listener = 0, const char *_type = "default");
  int handle_read_buffer(void *arg);
  int check_readable(int _sock_fd, long _timeout_usec);
  int start_listen(int _port, const char *_type = "default");

  void handle_keep_alive_command(buffer_queue* _queue, time_t *_time);
  void handle_unrecognized_command(buffer_queue* _queue);
};

#endif // AMEGIA_PNP_SERVER_GENERAL_MANAGER_H
