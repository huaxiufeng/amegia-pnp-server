#ifndef AMEGIA_PNP_SERVER_GENERAL_MANAGER_H
#define AMEGIA_PNP_SERVER_GENERAL_MANAGER_H

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <sys/time.h>
#include <pthread.h>
#include <map>

class general_context {
public:
  general_context():m_conn_port(0){
    gettimeofday(&m_create_time, NULL);
    gettimeofday(&m_update_time, NULL);
  }
  general_context(const general_context &ctx) {
    m_conn_ip = ctx.m_conn_ip;
    m_conn_port = ctx.m_conn_port;
    m_camera_ip = ctx.m_camera_ip;
    m_camera_mac = ctx.m_camera_mac;
    m_create_time = ctx.m_create_time;
    m_update_time = ctx.m_update_time;
  }
  general_context& operator= (const general_context &ctx) {
    if (this != &ctx) {
      m_conn_ip = ctx.m_conn_ip;
      m_conn_port = ctx.m_conn_port;
      m_camera_ip = ctx.m_camera_ip;
      m_camera_mac = ctx.m_camera_mac;
      m_create_time = ctx.m_create_time;
      m_update_time = ctx.m_update_time;
    }
    return *this;
  }
  std::string m_conn_ip;
  uint32_t m_conn_port;
  std::string m_camera_ip;
  std::string m_camera_mac;
  struct timeval m_create_time;
  struct timeval m_update_time;
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
