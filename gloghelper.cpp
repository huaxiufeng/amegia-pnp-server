// Copyright (c) 2012-2013 Hua Xiufeng <huaxiufeng@kaisquare.com.cn>
// Copyright (c) 2013 KAI Square Software Limited
// All rights reserved
//
// Author: Hua Xiufeng

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

