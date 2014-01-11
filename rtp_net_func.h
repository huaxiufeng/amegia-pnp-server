// Copyright (c) 2012-2013 Hua Xiufeng <huaxiufeng@kaisquare.com.cn>
// Copyright (c) 2013 KAI Square Software Limited
// All rights reserved
//
// Author: Hua Xiufeng

#ifndef AMEGIA_PNP_SERVER_RTP_NET_FUNC_H
#define AMEGIA_PNP_SERVER_RTP_NET_FUNC_H

/** extract rtp packet into h264 frame
 ** including start code "00000001"
 *
 * buf_in - rtp buffer (input)
 * len_in - length of buf_in (input)
 * buf_out - unpackaged video frame (output)
 * len_out - size of buf_out allocated (input)
 *
 * return the actual frame size
 */

extern int rtp_unpackage(const char *buf_in, int len_in, char *buf_out, int len_out, void *arg = NULL);

#endif // AMEGIA_PNP_SERVER_RTP_NET_FUNC_H
