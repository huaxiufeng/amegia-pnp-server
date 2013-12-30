#ifndef AMEGIA_PNP_SERVER_MESSAGE_TYPES_H
#define AMEGIA_PNP_SERVER_MESSAGE_TYPES_H

// IOCTL Message Type
typedef enum
{
  IOCTL_KEEP_ALIVE			= 0x0000,
  IOCTL_RTSP_READY,
  IOCTL_PC_PNP_READY,
  IOCTL_CAM_HELO,
  IOCTL_GET_CONTROLSERVER_REQ		= 0x1000,
  IOCTL_GET_CONTROLSERVER_RESP,
  IOCTL_SET_CONTROLSERVER_REQ,
  IOCTL_SET_CONTROLSERVER_RESP,
  IOCTL_GET_RTSPSERVER_REQ,
  IOCTL_GET_RTSPSERVER_RESP,
  IOCTL_SET_RTSPSERVER_REQ,
  IOCTL_SET_RTSPSERVER_RESP,
  IOCTL_GET_PARAMETER_REQ,
  IOCTL_GET_PARAMETER_RESP,
  IOCTL_SET_PARAMETER_REQ,
  IOCTL_SET_PARAMETER_RESP,

  // ipcam ask main_server info of snapshot_server
  IOCTL_GET_SNAPSHOTSERVER_REQ,
  IOCTL_GET_SNAPSHOTSERVER_RESP,

  // main_server forces ipcam to change the info of snapshot_server
  IOCTL_SET_SNAPSHOTSERVER_REQ,
  IOCTL_SET_SNAPSHOTSERVER_RESP,

  // main_server ask ipcam to send snapshot to snapshot_server
  IOCTL_GET_SNAPSHOT_REQ,
  IOCTL_GET_SNAPSHOT_RESP,

  // ipcam send snapshot to snapshot_server
  IOCTL_SET_SNAPSHOT_REQ,
  IOCTL_SET_SNAPSHOT_RESP,

  // main_server <--> stream server using
  IOCTL_REG_STREAMSERVER_REQ		= 0x8000,
  IOCTL_REG_STREAMSERVER_RESP,

  // testing using
  IOCTL_REQUEST_TEST_00			= 0xF000,
  IOCTL_REQUEST_TEST_01,

} ENUM_IOCTL_MSGTYPE;

// ---------- Message body define ----------
typedef struct
{
  char magic[2];
  unsigned short size;
  int ioctlCmd;
  char data[];
} IoctlMsg;

typedef struct
{
  char name[32];
  int	port;
  unsigned char type;	// 0:IP, 1:name
  unsigned char err;	// 0:Normal, 1: Error
  unsigned char reserved[2];
} stServerDef;

// -- IOCTL_KEEP_ALIVE --
// -- IOCTL_RTSP_READY --
// -- IOCTL_GET_CONTROLSERVER_REQ --
// -- IOCTL_GET_RTSPSERVER_REQ --
typedef struct
{
  unsigned char reserved[4];
} IoctlMsgNormal;;

// -- IOCTL_GET_CONTROLSERVER_RESP --
// -- IOCTL_SET_CONTROLSERVER_REQ --
typedef struct
{
  int number;
  stServerDef servers[0];
} IoctlMsgGetControlServerResp, IoctlMsgSetControlServerReq;

// -- IOCTL_SET_CONTROLSERVER_RESP --
// -- IOCTL_SET_RTSPSERVER_RESP --
typedef struct
{
  int result;					// 0: success; otherwise: failed.
  unsigned char reserved[4];
} IoctlMsgSetControlServerResp, IoctlMsgSetRtspServerResp;

// -- IOCTL_GET_RTSPSERVER_RESP --
// -- IOCTL_SET_RTSPSERVER_REQ --
typedef struct
{
  stServerDef server;
} IoctlMsgGetRtspServerResp, IoctlMsgSetRtspServerReq;

// -- IOCTL_GET_SNAPSHOTSERVER_RESP --
// -- IOCTL_SET_SNAPSHOTSERVER_REQ --
typedef struct
{
	int interval;
	int number;
	stServerDef servers[0];
} IoctlMsgGetSnapshotServerResp, IoctlMsgSetSnapshotServerReq;

// -- IOCTL_GET_PARAMETER_RESP --	// 50KB
// -- IOCTL_SET_PARAMETER_RESP --
typedef struct
{
  unsigned int  total;		// Total bytes
  unsigned char index;		// package index, 0,1,2...;
  unsigned char endflag;	// end flag; endFlag = 1 means this package is the last one.
  unsigned short count;		// how much bytes in this package
  char result[0];			// The first memory address of the parameter in this package
} IoctlMsgGetParameterReq, IoctlMsgGetParameterResp,
IoctlMsgSetParameterReq, IoctlMsgSetParameterResp,
IoctlMsgSetSnapshotReq, IoctlMsgSetSnapshotResp,
IoctlMsgGetSnapshotReq, IoctlMsgGetSnapshotResp;

#endif // AMEGIA_PNP_SERVER_MESSAGE_TYPES_H
