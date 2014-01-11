// Copyright (c) 2012-2013 Hua Xiufeng <huaxiufeng@kaisquare.com.cn>
// Copyright (c) 2013 KAI Square Software Limited
// All rights reserved
//
// Author: Hua Xiufeng

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#include "message_types.h"
#include "global_config.h"
#include "gloghelper.h"
#include "snapshot_manager.h"
using namespace std;

static void handle_set_snapshot_command(camera_context *context)
{
  const char *data = context->m_snapshot_buffer_queue->top();
  int current_size = context->m_snapshot_buffer_queue->size();
  if (current_size < sizeof(IoctlMsg)+sizeof(IoctlMsgSetSnapshotReq)) {
    return;
  }
  IoctlMsg *recv_message = (IoctlMsg*)data;
  int message_full_size = 4 + recv_message->size;
  if (current_size < message_full_size) {
    return;
  }

  int current_snapshot_size = 0, total_snapshot_size = 0, end_flag = 0;
  char snapshot[MAX_SNAPSHOT_SIZE];
  memset(snapshot, 0, sizeof(snapshot));
  int index = 0;
  int last_index = -1;
  for (index = 0; index < current_size-1; index++) {
    if (current_size-index < sizeof(IoctlMsg)+sizeof(IoctlMsgSetSnapshotReq)) {
      break;
    }
    if (data[index] == '$' && data[index+1] == '\0') {
      IoctlMsgSetSnapshotReq *response = (IoctlMsgSetSnapshotReq*)(data+index+sizeof(IoctlMsg));
      if (index+sizeof(IoctlMsg)+sizeof(IoctlMsgSetSnapshotReq)+response->count > current_size) {
        break;
      }
      if (response->index == 0 && index != 0 && current_snapshot_size % 258048 != 0) {
        //LOG(INFO)<<"the last snapshot is incomplete, find first segment at index "<<index<<", clear the old one"<<endl;
        if (last_index <= 0 || current_snapshot_size < 1008*10) {
          context->m_snapshot_buffer_queue->pop(NULL, index);
          end_flag = 0;
          break;
        }
        else {
          index = last_index;
          end_flag = 1;
          break;
        }
      }
      if (response->index > last_index + 10) {
        LOG(INFO)<<"index error happens at index "<<index<<", clear the old one"<<endl;
        context->m_snapshot_buffer_queue->pop(NULL, index);
        break;
      }
      if (0 == total_snapshot_size) total_snapshot_size = response->total;
      memcpy(snapshot+current_snapshot_size, response->result, response->count);
      current_snapshot_size += response->count;
      end_flag = response->endflag;
      //LOG(INFO)<<"index:"<<(int)response->index<<" end:"<<(int)response->endflag<<" ["<<current_snapshot_size<<"/"<<(int)response->total<<"]"<<endl;
      if (current_snapshot_size >= total_snapshot_size || end_flag) {
        //LOG(INFO)<<"["<<context->m_conn_ip<<" - "<<context->m_camera_mac<<" --> localhost.fd="<<context->m_snapshot_fd<<"]"<<" get snapshot "<<current_snapshot_size<<" bytes, total "<<total_snapshot_size<<endl;
        break;
      }
      last_index = index;
      index = index + sizeof(IoctlMsg) + sizeof(IoctlMsgSetSnapshotReq) + response->count - 1;
    }
  }
  if (current_snapshot_size >= total_snapshot_size || end_flag) {
    IoctlMsgSetSnapshotReq *response = (IoctlMsgSetSnapshotReq*)(data+index+sizeof(IoctlMsg));

    //LOG(INFO)<<context->m_camera_mac<<" index: "<<index<<endl;
    //LOG(INFO)<<context->m_camera_mac<<" current_size: "<<current_size<<endl;
    //LOG(INFO)<<context->m_camera_mac<<" current_snapshot_size: "<<current_snapshot_size<<endl;
    //LOG(INFO)<<context->m_camera_mac<<" total_snapshot_size: "<<total_snapshot_size<<endl;
    //LOG(INFO)<<context->m_camera_mac<<" end_flag: "<<end_flag<<endl;
    //LOG(INFO)<<context->m_camera_mac<<" last count: "<<response->count<<endl;

    int pop_size = index+response->count+sizeof(IoctlMsg)+sizeof(IoctlMsgSetSnapshotReq);

    if (pop_size <= current_size) {
      // if haven't get mac from camera, just disconnect it and let it connect again
      if (context->m_camera_mac.length() == 0) {
        context->m_connected = false;
        return;
      }
      //LOG(INFO)<<context->m_camera_mac<<" should pop out "<<pop_size<<endl;
      //context->m_snapshot_buffer_queue->pop(NULL, pop_size);
      //LOG(INFO)<<context->m_camera_mac<<" buffer queue left "<<context->m_snapshot_buffer_queue->size()<<endl;
      // in case there are keep alive commands after the snapshot, clear the buffer
      context->m_snapshot_buffer_queue->clear();

      // update snapshot service status
      snapshot_manager::get_instance()->handle_keep_alive_command(NULL, &(context->m_snapshot_update_time));
      context->m_snapshot_count++;

      // save the snapshot
      if (strlen(g_snapshot_directory) > 0 && current_snapshot_size > 10*1024) {
        string file_name = snapshot_manager::get_instance()->generate_name(context->m_camera_mac.c_str());
        if (file_name.length() > 0) {
          FILE *fp = fopen(file_name.c_str(), "wb");
          if (fp) {
            fwrite(snapshot, 1, current_snapshot_size, fp);
            fclose(fp);
          }
        }
      }
      // notify callback
      if (g_snapshot_callback && current_snapshot_size > 10*1024) {
        (*g_snapshot_callback)(context->m_conn_ip.c_str(), context->m_camera_mac.c_str(), (const unsigned char*)snapshot, current_snapshot_size);
      }
    }
    else {
      LOG(INFO)<<context->m_camera_mac<<" the last segment is incomplete!"<<endl;
    }
  }
}

void snapshot_manager::create_directory(const char *_dir)
{
  char path[512];
  snprintf(path, sizeof(path), "%s", _dir);
  int len = strlen(path);
  for (int i = 0; i < len; i++) {
    if(path[i] == '\\') path[i] = '/';
  }
  if (path[len-1] != '\\' && path[len-1] != '/') {
    path[len] = '/';
    path[len+1] = '\0';
    len++;
  }
  for (int i = 0; i < len; i++) {
    if(('\\' == path[i] || '/' == path[i]) && i != 0) {
      path[i] = '\0';
      int ret = access(path, 0);
      if (ret != 0) {
        ret = mkdir(path, 0755);
        if (ret != 0) {
          return;
        }
      }
      path[i] = '/';
    }
  }
}

std::string snapshot_manager::generate_name(const char *_mac)
{
  string subdir;
  for (int i = 0; i < strlen(_mac); i++) {
    if (_mac[i] != ':') subdir += _mac[i];
  }
  char file_name[128];
  time_t now = time(NULL);
  string name;
  do {
    strftime(file_name, 32, "/%Y%m%d_%H%M%S.jpg", localtime(&now));
    name = g_snapshot_directory + subdir;
    if (access(name.c_str(), 0) != 0) {
      create_directory(name.c_str());
    }
    name += file_name;
    if (access(name.c_str(), 0) == 0) {
      now++;
    } else {
      break;
    }
  } while(true);
  return name;
}

int snapshot_manager::handle_accept_connection(void *arg, int listener, const char *_type)
{
  camera_context *ctx = (camera_context*)arg;
  if (!ctx || ctx->m_snapshot_listen_fd <= 0) return -1;
  if (ctx->m_snapshot_fd > 0) return ctx->m_snapshot_fd;

  int fd = general_manager::handle_accept_connection(ctx, ctx->m_snapshot_listen_fd, _type);
  if (fd <= 0) return fd;

  ctx->m_snapshot_fd = fd;
  return fd;
}

int snapshot_manager::handle_read_buffer(void *arg)
{
  camera_context *ctx = (camera_context*)arg;
  if (!ctx || ctx->m_snapshot_fd <= 0) return -1;

  int fd = ctx->m_snapshot_fd;
  int res = general_manager::check_readable(fd, 10*1000);
  if (res <= 0) {
    return res;
  }

  char recv_buffer[MAX_BUFF_SIZE];
  memset(recv_buffer, 0, sizeof(recv_buffer));
  int recv_len = read(fd, recv_buffer, sizeof(recv_buffer));
  if (recv_len <= 0) {
    return recv_len;
  }

  ctx->m_snapshot_buffer_queue->push(recv_buffer, recv_len);
  const char *data = ctx->m_snapshot_buffer_queue->top();
  IoctlMsg *recv_message = (IoctlMsg*)data;

  switch (recv_message->ioctlCmd) {
  case IOCTL_CAM_HELO:
  case IOCTL_KEEP_ALIVE:
    //LOG(INFO)<<"IOCTL_KEEP_ALIVE "<<fd<<endl;
    handle_keep_alive_command(ctx->m_snapshot_buffer_queue, &(ctx->m_snapshot_update_time));
    break;
  case IOCTL_SET_SNAPSHOT_REQ:
    //LOG(INFO)<<"IOCTL_SET_SNAPSHOT_REQ "<<fd<<" len:"<<recv_len<<endl;
    handle_set_snapshot_command(ctx);
    break;
  default:
    //LOG(INFO)<<"UNRECOGNIZED COMMAND "<<fd<<hex<<" 0X"<<recv_message->ioctlCmd<<endl;
    handle_unrecognized_command(ctx->m_snapshot_buffer_queue);
    break;
  }
}
