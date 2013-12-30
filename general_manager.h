#ifndef AMEGIA_PNP_SERVER_GENERAL_MANAGER_H
#define AMEGIA_PNP_SERVER_GENERAL_MANAGER_H

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <sys/time.h>
#include <pthread.h>
#include <map>
#include "buffer_queue.h"

typedef BufferQueue<char> buffer_queue;

class general_context {
public:
  general_context():m_conn_port(0),m_video_track_id(0),m_audio_track_id(1){
    gettimeofday(&m_create_time, NULL);
    gettimeofday(&m_update_time, NULL);
    m_buffer_queue = new buffer_queue(4096);
  }
  general_context(const general_context &ctx) {
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
  general_context& operator= (const general_context &ctx) {
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
  ~general_context() {
    if (m_buffer_queue) delete m_buffer_queue;
  }

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

class general_manager {
public:
  bool add(evutil_socket_t _fd, const general_context& _info);
  general_context* get(evutil_socket_t _fd);
  bool keep_alive(evutil_socket_t _fd);
  bool remove(evutil_socket_t _fd);
  bool remove(const char *_ip);
protected:
  general_manager() {pthread_mutex_init(&m_lock, NULL);}
  virtual ~general_manager() {pthread_mutex_destroy(&m_lock);}
  void lock() {pthread_mutex_lock(&m_lock);}
  void unlock() {pthread_mutex_unlock(&m_lock);}

  pthread_mutex_t m_lock;
  general_table_type m_table;
};

#endif // AMEGIA_PNP_SERVER_GENERAL_MANAGER_H
