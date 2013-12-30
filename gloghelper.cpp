#include "gloghelper.h"

gloghelper::gloghelper()
{
  google::InitGoogleLogging("");
  google::LogToStderr();
#ifndef _MSC_VER
  google::SetStderrLogging(google::INFO);
#endif
}

gloghelper::~gloghelper()
{
  google::ShutdownGoogleLogging();
}

