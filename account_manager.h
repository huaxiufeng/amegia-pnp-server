#ifndef AMEGIA_PNP_SERVER_ACCOUNT_MANAGER_H
#define AMEGIA_PNP_SERVER_ACCOUNT_MANAGER_H

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <sys/time.h>
#include <pthread.h>
#include <string>
#include <map>

class account_context {
public:
  account_context(const char *_ip = "", uint32_t _port = 0):m_ip(_ip),m_port(_port){gettimeofday(&m_time, NULL);}
  std::string m_ip;
  uint32_t m_port;
  struct timeval m_time;
};

typedef std::map<evutil_socket_t, account_context> account_table_type;

class account_manager {
public:
  static account_manager* get_instance() {
    static account_manager instance;
    return &instance;
  }
  bool add_account(evutil_socket_t _fd, const char *_ip, uint32_t _port);
  bool get_account(evutil_socket_t _fd, account_context& context);
  bool update_account(evutil_socket_t _fd);
  bool check_account(evutil_socket_t _fd);
  bool remove_account(evutil_socket_t _fd);
  bool remove_account(const char *_ip, uint32_t _port);
private:
  account_manager();
  ~account_manager();
  void lock() {pthread_mutex_lock(&m_lock);}
  void unlock() {pthread_mutex_unlock(&m_lock);}

  pthread_mutex_t m_lock;
  account_table_type m_account_table;
};

#endif // AMEGIA_PNP_SERVER_ACCOUNT_MANAGER_H
