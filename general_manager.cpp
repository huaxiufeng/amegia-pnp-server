#include "gloghelper.h"
#include "general_manager.h"

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

bool general_manager::remove(const char *_ip)
{
  bool res = false;
  lock();
  unlock();
  return res;
}
