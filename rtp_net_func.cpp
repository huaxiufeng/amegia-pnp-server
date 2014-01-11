// Copyright (c) 2012-2013 Hua Xiufeng <huaxiufeng@kaisquare.com.cn>
// Copyright (c) 2013 KAI Square Software Limited
// All rights reserved
//
// Author: Hua Xiufeng

#include <string.h>
#include "gloghelper.h"
#include "rtp_net_func.h"
#include "buffer_queue.h"

typedef BufferQueue<char> buffer_queue;

typedef struct
{
/*
0                   1                   2                   3
0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|V=2|P|X|  CC   |M|     PT      |       sequence number         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           timestamp                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|           synchronization source (SSRC) identifier            |
+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
|            contributing source (CSRC) identifiers             |
|                             ....                              |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
V - Version. Identifies the RTP version.
P - Padding. When set, the packet contains one or more additional padding octets at the end which are not part of the payload.
X - Extension bit. When set, the fixed header is followed by exactly one header extension, with a defined format.
CC - CSRC count. Contains the number of CSRC identifiers that follow the fixed header.
M - Marker. The interpretation of the marker is defined by a profile. It is intended to allow significant events such as frame boundaries to be marked in the packet stream.
PT - Payload type. Identifies the format of the RTP payload and determines its interpretation by the application.
     A profile specifies a default static mapping of payload type codes to payload formats. Additional payload type codes may be defined dynamically through non-RTP means.
sequence number - Increments by one for each RTP data packet sent, and may be used by the receiver to detect packet loss and to restore packet sequence.
timestamp - Reflects the sampling instant of the first octet in the RTP data packet.
            The sampling instant must be derived from a clock that increments monotonically and linearly in time to allow synchronization and jitter calculations.
SSRC - Synchronization source. This identifier is chosen randomly, with the intent that no two synchronization sources within the same RTP session will have the same SSRC identifier.
CSRC - Contributing source identifiers list. Identifies the contributing sources for the payload contained in this packet.
*/
/*
the byte order of network stream is big-endian
while in local memory is little-edian
to take the first byte for example:
network stream -- big-endian order : version:2->padding:1->extension:1->csrc_len:4
local memory -- little-endian order: csrc_len:4->extension:1->padding:1->version:2
*/
  /* byte 0 */
  unsigned char csrc_len:4;        /* expect 0 */
  unsigned char extension:1;       /* expect 1, see RTP_OP below */
  unsigned char padding:1;         /* expect 0 */
  unsigned char version:2;         /* expect 2 */
  /* byte 1 */
  unsigned char payloadtype:7;     /* RTP_PAYLOAD_RTSP */
  unsigned char marker:1;          /* expect 1 */
  /* bytes 2,3 */
  unsigned int seq_no;
  /* bytes 4-7 */
  unsigned int timestamp;
  /* bytes 8-11 */
  unsigned int ssrc;              /* stream number is used here. */
} RTP_FIXED_HEADER;

/*
   +---------------+
   |0|1|2|3|4|5|6|7|
   +-+-+-+-+-+-+-+-+
   |F|NRI|  Type   |
   +---------------+
   */
typedef struct
{
  //byte 0
  unsigned char TYPE:5;
  unsigned char NRI:2;
  unsigned char F:1;
} NALU_HEADER; // 1 BYTE

/*
   +---------------+
   |0|1|2|3|4|5|6|7|
   +-+-+-+-+-+-+-+-+
   |F|NRI|  Type   |
   +---------------+
   */
typedef struct
{
  //byte 0
  unsigned char TYPE:5;
  unsigned char NRI:2;
  unsigned char F:1;
} FU_INDICATOR; // 1 BYTE

/*
   +---------------+
   |0|1|2|3|4|5|6|7|
   +-+-+-+-+-+-+-+-+
   |S|E|R|  Type   |
   +---------------+
   */
typedef struct
{
  //byte 0
  unsigned char TYPE:5;
  unsigned char R:1;
  unsigned char E:1;
  unsigned char S:1;
} FU_HEADER;   // 1 BYTES

int rtp_unpackage(const char *buf_in, int len_in, char *buf_out, int len_out, void *arg)
{
  buffer_queue *fu = (buffer_queue*)arg;

  int buf_out_index = 0;
  static char start_code[] = {0x00, 0x00, 0x00, 0x01};
  RTP_FIXED_HEADER *rtp_hdr = (RTP_FIXED_HEADER*)buf_in;
  NALU_HEADER *nalu_hdr = (NALU_HEADER*)(buf_in+12);
  FU_INDICATOR *fu_ind = NULL;
	FU_HEADER	*fu_hdr= NULL;

  if (len_in <= 14) {
    LOG(ERROR)<<"buf_in is not valid, only have "<<len_in<<endl;
    buf_out_index = -1;
    return buf_out_index;
  }
  int required_output_size = 4+1+len_in-13;
  if (len_out < required_output_size) {
    LOG(ERROR)<<"buf_out can not hold the unpackaged frame, need at least "<<required_output_size<<", only have "<<len_out<<endl;
    buf_out_index = -1;
    return buf_out_index;
  }

  memset(buf_out, 0, len_out);

  //LOG(INFO)<<"version:        "<<(int)rtp_hdr->version<<endl;
  //LOG(INFO)<<"marker:         "<<(int)rtp_hdr->marker<<endl;
  //LOG(INFO)<<"payload type:   "<<(int)rtp_hdr->payloadtype<<endl;
  //LOG(INFO)<<"squence number: "<<rtp_hdr->seq_no<<endl;
  //LOG(INFO)<<"timestamp:      "<<rtp_hdr->timestamp<<endl;
  //LOG(INFO)<<"SSRC:           "<<rtp_hdr->ssrc<<endl;
  //LOG(INFO)<<"nalu type:      "<<(int)nalu_hdr->TYPE<<endl;

  if (nalu_hdr->TYPE == 0) {
    LOG(ERROR)<<"NALU type error, type 0 is not defined"<<endl;
    buf_out_index = -2;
  } else
  if (nalu_hdr->TYPE >0 &&  nalu_hdr->TYPE < 24) {
    //LOG(INFO)<<"this package is single NALU"<<endl;
    memcpy(buf_out + buf_out_index, start_code, 4);
    buf_out_index += 4;
    memcpy(buf_out + buf_out_index, nalu_hdr, 1);
    buf_out_index += 1;
    memcpy(buf_out + buf_out_index, buf_in+13, len_in-13);
    buf_out_index += (len_in-13);
  } else
  if ( nalu_hdr->TYPE == 24) {
    LOG(ERROR)<<"this package is STAP-A, need to be implemented"<<endl;
  } else
  if ( nalu_hdr->TYPE == 25) {
    LOG(ERROR)<<"this package is STAP-B, need to be implemented"<<endl;
  } else
  if (nalu_hdr->TYPE == 26) {
    LOG(ERROR)<<"this package is MTAP16, need to be implemented"<<endl;
  } else
  if ( nalu_hdr->TYPE == 27) {
    LOG(ERROR)<<"this package is MTAP24, need to be implemented"<<endl;
  } else
  if ( nalu_hdr->TYPE == 28 && fu) {
    //LOG(INFO)<<"this package is FU-A"<<endl;
    fu_ind = (FU_INDICATOR*)(buf_in+12);
    fu_hdr = (FU_HEADER*)(buf_in+13);
    if (rtp_hdr->marker == 1) { // this is the last segment of the fu-a
      fu->push(buf_in + 14, len_in - 14);
      required_output_size = fu->size();
      if (len_out >= required_output_size) {
        memcpy(buf_out, fu->top(), required_output_size);
        buf_out_index = required_output_size;
      } else {
        LOG(ERROR)<<"buf_out can not hold the unpackaged frame, need at least "<<required_output_size<<", only have "<<len_out<<endl;
        buf_out_index = -1;
      }
      fu->clear();
    } else
    if (rtp_hdr->marker == 0) { // this is not the last segment
      if (fu_hdr->S == 1) {     // this is the first segment
        fu->clear();
        fu->push(start_code, 4);
        char nal_hdr = (fu_ind->F << 7) | (fu_ind->NRI << 5) | (fu_hdr->TYPE);
        fu->push(&nal_hdr, 1);
      } else {                  // this is not the first segment
      }
      fu->push(buf_in + 14, len_in - 14);
    }
  } else
  if ( nalu_hdr->TYPE == 29) {
    LOG(ERROR)<<"this package is FU-B, need to be implemented"<<endl;
  } else {
    LOG(ERROR)<<"NALU type error, type 30-31 is not defined"<<endl;
    buf_out_index = -2;
  }
  return buf_out_index;
}
