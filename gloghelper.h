#ifndef UTIL_GLOG_HELPER_H
#define UTIL_GLOG_HELPER_H

#ifdef    _MSC_VER
#define GLOG_NO_ABBREVIATED_SEVERITIES
#endif

/**
  * 0. print all the messages (INFO, WARNING, ERROR, FATAL)
  * 1. print WARNING messages and above
  * 2. print ERROR messages and above
  * 3. print only FATAL messages
  */
#define GOOGLE_STRIP_LOG 0

#include <glog/logging.h>

#include <iostream>
#include <iomanip>
using namespace std;

class gloghelper
{
  public:
    static gloghelper* get_instance() {
      static gloghelper instance;
      return &instance;
    }

  private:
    gloghelper();
    ~gloghelper();
};

#endif
