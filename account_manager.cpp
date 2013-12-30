#include "gloghelper.h"
#include "account_manager.h"
using namespace std;

account_manager::account_manager()
{
  pthread_mutex_init(&m_lock, NULL);
}

account_manager::~account_manager()
{
  pthread_mutex_destroy(&m_lock);
}

bool account_manager::add_account(evutil_socket_t _fd, const char *_ip, uint32_t _port)
{
  bool res = false;
  if (!_ip || _port <= 0 || _port > 65536) return res;
  lock();
  account_context context(_ip, _port);
  m_account_table.insert(std::pair<evutil_socket_t, account_context>(_fd, context));
  res = true;
  unlock();
  return res;
}

bool account_manager::get_account(evutil_socket_t _fd, account_context& context)
{
  bool res = false;
  lock();
  account_table_type::iterator itor = m_account_table.find(_fd);
  if (itor != m_account_table.end()) {
    context = itor->second;
    res = true;
  }
  unlock();
  return res;
}

bool account_manager::update_account(evutil_socket_t _fd)
{
  bool res = false;
  lock();
  account_table_type::iterator itor = m_account_table.find(_fd);
  if (itor != m_account_table.end()) {
    gettimeofday(&(itor->second.m_time), NULL);
    res = true;
  }
  unlock();
  return res;
}

bool account_manager::check_account(evutil_socket_t _fd)
{
  bool res = false;
  lock();
  account_table_type::iterator itor = m_account_table.find(_fd);
  if (itor != m_account_table.end()) {
    res = true;
  }
  unlock();
  return res;
}

bool account_manager::remove_account(evutil_socket_t _fd)
{
  bool res = false;
  lock();
  account_table_type::iterator itor = m_account_table.find(_fd);
  if (itor != m_account_table.end()) {
    m_account_table.erase(_fd);
    res = true;
  }
  unlock();
  return res;
}

bool account_manager::remove_account(const char *_ip, uint32_t _port)
{
  bool res = false;
  lock();
  account_table_type::iterator itor = m_account_table.begin();
  do {
    for (itor = m_account_table.begin(); itor != m_account_table.end(); itor++) {
      if (itor->second.m_ip == _ip) {
        m_account_table.erase(itor);
        res = true;
        break;
      }
    }
  } while (itor != m_account_table.end());
  unlock();
  return res;
}
