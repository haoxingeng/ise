/****************************************************************************\
*                                                                            *
*  ISE (Iris Server Engine) Project                                          *
*  http://github.com/haoxingeng/ise                                          *
*                                                                            *
*  Copyright 2013 HaoXinGeng (haoxingeng@gmail.com)                          *
*  All rights reserved.                                                      *
*                                                                            *
*  Licensed under the Apache License, Version 2.0 (the "License");           *
*  you may not use this file except in compliance with the License.          *
*  You may obtain a copy of the License at                                   *
*                                                                            *
*      http://www.apache.org/licenses/LICENSE-2.0                            *
*                                                                            *
*  Unless required by applicable law or agreed to in writing, software       *
*  distributed under the License is distributed on an "AS IS" BASIS,         *
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
*  See the License for the specific language governing permissions and       *
*  limitations under the License.                                            *
*                                                                            *
\****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// ise_socket.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_SOCKET_H_
#define _ISE_SOCKET_H_

#include "ise_options.h"

#ifdef ISE_WIN32
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#endif

#ifdef ISE_LINUX
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <string>
#endif

#include "ise_classes.h"
#include "ise_thread.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// 提前声明

class CSocket;
class CUdpSocket;
class CUdpClient;
class CUdpServer;
class CTcpSocket;
class CDtpConnection;
class CBaseTcpConnection;
class CTcpClient;
class CTcpServer;
class CListenerThread;
class CUdpListenerThread;
class CUdpListenerThreadPool;
class CTcpListenerThread;

///////////////////////////////////////////////////////////////////////////////
// 常量定义

#ifdef ISE_WIN32
const int SS_SD_RECV            = 0;
const int SS_SD_SEND            = 1;
const int SS_SD_BOTH            = 2;

const int SS_EINTR              = WSAEINTR;
const int SS_EBADF              = WSAEBADF;
const int SS_EACCES             = WSAEACCES;
const int SS_EFAULT             = WSAEFAULT;
const int SS_EINVAL             = WSAEINVAL;
const int SS_EMFILE             = WSAEMFILE;
const int SS_EWOULDBLOCK        = WSAEWOULDBLOCK;
const int SS_EINPROGRESS        = WSAEINPROGRESS;
const int SS_EALREADY           = WSAEALREADY;
const int SS_ENOTSOCK           = WSAENOTSOCK;
const int SS_EDESTADDRREQ       = WSAEDESTADDRREQ;
const int SS_EMSGSIZE           = WSAEMSGSIZE;
const int SS_EPROTOTYPE         = WSAEPROTOTYPE;
const int SS_ENOPROTOOPT        = WSAENOPROTOOPT;
const int SS_EPROTONOSUPPORT    = WSAEPROTONOSUPPORT;
const int SS_ESOCKTNOSUPPORT    = WSAESOCKTNOSUPPORT;
const int SS_EOPNOTSUPP         = WSAEOPNOTSUPP;
const int SS_EPFNOSUPPORT       = WSAEPFNOSUPPORT;
const int SS_EAFNOSUPPORT       = WSAEAFNOSUPPORT;
const int SS_EADDRINUSE         = WSAEADDRINUSE;
const int SS_EADDRNOTAVAIL      = WSAEADDRNOTAVAIL;
const int SS_ENETDOWN           = WSAENETDOWN;
const int SS_ENETUNREACH        = WSAENETUNREACH;
const int SS_ENETRESET          = WSAENETRESET;
const int SS_ECONNABORTED       = WSAECONNABORTED;
const int SS_ECONNRESET         = WSAECONNRESET;
const int SS_ENOBUFS            = WSAENOBUFS;
const int SS_EISCONN            = WSAEISCONN;
const int SS_ENOTCONN           = WSAENOTCONN;
const int SS_ESHUTDOWN          = WSAESHUTDOWN;
const int SS_ETOOMANYREFS       = WSAETOOMANYREFS;
const int SS_ETIMEDOUT          = WSAETIMEDOUT;
const int SS_ECONNREFUSED       = WSAECONNREFUSED;
const int SS_ELOOP              = WSAELOOP;
const int SS_ENAMETOOLONG       = WSAENAMETOOLONG;
const int SS_EHOSTDOWN          = WSAEHOSTDOWN;
const int SS_EHOSTUNREACH       = WSAEHOSTUNREACH;
const int SS_ENOTEMPTY          = WSAENOTEMPTY;
#endif

#ifdef ISE_LINUX
const int SS_SD_RECV            = SHUT_RD;
const int SS_SD_SEND            = SHUT_WR;
const int SS_SD_BOTH            = SHUT_RDWR;

const int SS_EINTR              = EINTR;
const int SS_EBADF              = EBADF;
const int SS_EACCES             = EACCES;
const int SS_EFAULT             = EFAULT;
const int SS_EINVAL             = EINVAL;
const int SS_EMFILE             = EMFILE;
const int SS_EWOULDBLOCK        = EWOULDBLOCK;
const int SS_EINPROGRESS        = EINPROGRESS;
const int SS_EALREADY           = EALREADY;
const int SS_ENOTSOCK           = ENOTSOCK;
const int SS_EDESTADDRREQ       = EDESTADDRREQ;
const int SS_EMSGSIZE           = EMSGSIZE;
const int SS_EPROTOTYPE         = EPROTOTYPE;
const int SS_ENOPROTOOPT        = ENOPROTOOPT;
const int SS_EPROTONOSUPPORT    = EPROTONOSUPPORT;
const int SS_ESOCKTNOSUPPORT    = ESOCKTNOSUPPORT;

const int SS_EOPNOTSUPP         = EOPNOTSUPP;
const int SS_EPFNOSUPPORT       = EPFNOSUPPORT;
const int SS_EAFNOSUPPORT       = EAFNOSUPPORT;
const int SS_EADDRINUSE         = EADDRINUSE;
const int SS_EADDRNOTAVAIL      = EADDRNOTAVAIL;
const int SS_ENETDOWN           = ENETDOWN;
const int SS_ENETUNREACH        = ENETUNREACH;
const int SS_ENETRESET          = ENETRESET;
const int SS_ECONNABORTED       = ECONNABORTED;
const int SS_ECONNRESET         = ECONNRESET;
const int SS_ENOBUFS            = ENOBUFS;
const int SS_EISCONN            = EISCONN;
const int SS_ENOTCONN           = ENOTCONN;
const int SS_ESHUTDOWN          = ESHUTDOWN;
const int SS_ETOOMANYREFS       = ETOOMANYREFS;
const int SS_ETIMEDOUT          = ETIMEDOUT;
const int SS_ECONNREFUSED       = ECONNREFUSED;
const int SS_ELOOP              = ELOOP;
const int SS_ENAMETOOLONG       = ENAMETOOLONG;
const int SS_EHOSTDOWN          = EHOSTDOWN;
const int SS_EHOSTUNREACH       = EHOSTUNREACH;
const int SS_ENOTEMPTY          = ENOTEMPTY;
#endif

///////////////////////////////////////////////////////////////////////////////
// 错误信息 (ISE Socket Error Message)

const char* const SSEM_ERROR             = "Socket Error #%d: %s";
const char* const SSEM_SOCKETERROR       = "Socket error";
const char* const SSEM_TCPSENDTIMEOUT    = "TCP send timeout";
const char* const SSEM_TCPRECVTIMEOUT    = "TCP recv timeout";

const char* const SSEM_EINTR             = "Interrupted system call.";
const char* const SSEM_EBADF             = "Bad file number.";
const char* const SSEM_EACCES            = "Access denied.";
const char* const SSEM_EFAULT            = "Buffer fault.";
const char* const SSEM_EINVAL            = "Invalid argument.";
const char* const SSEM_EMFILE            = "Too many open files.";
const char* const SSEM_EWOULDBLOCK       = "Operation would block.";
const char* const SSEM_EINPROGRESS       = "Operation now in progress.";
const char* const SSEM_EALREADY          = "Operation already in progress.";
const char* const SSEM_ENOTSOCK          = "Socket operation on non-socket.";
const char* const SSEM_EDESTADDRREQ      = "Destination address required.";
const char* const SSEM_EMSGSIZE          = "Message too long.";
const char* const SSEM_EPROTOTYPE        = "Protocol wrong type for socket.";
const char* const SSEM_ENOPROTOOPT       = "Bad protocol option.";
const char* const SSEM_EPROTONOSUPPORT   = "Protocol not supported.";
const char* const SSEM_ESOCKTNOSUPPORT   = "Socket type not supported.";
const char* const SSEM_EOPNOTSUPP        = "Operation not supported on socket.";
const char* const SSEM_EPFNOSUPPORT      = "Protocol family not supported.";
const char* const SSEM_EAFNOSUPPORT      = "Address family not supported by protocol family.";
const char* const SSEM_EADDRINUSE        = "Address already in use.";
const char* const SSEM_EADDRNOTAVAIL     = "Cannot assign requested address.";
const char* const SSEM_ENETDOWN          = "Network is down.";
const char* const SSEM_ENETUNREACH       = "Network is unreachable.";
const char* const SSEM_ENETRESET         = "Net dropped connection or reset.";
const char* const SSEM_ECONNABORTED      = "Software caused connection abort.";
const char* const SSEM_ECONNRESET        = "Connection reset by peer.";
const char* const SSEM_ENOBUFS           = "No buffer space available.";
const char* const SSEM_EISCONN           = "Socket is already connected.";
const char* const SSEM_ENOTCONN          = "Socket is not connected.";
const char* const SSEM_ESHUTDOWN         = "Cannot send or receive after socket is closed.";
const char* const SSEM_ETOOMANYREFS      = "Too many references, cannot splice.";
const char* const SSEM_ETIMEDOUT         = "Connection timed out.";
const char* const SSEM_ECONNREFUSED      = "Connection refused.";
const char* const SSEM_ELOOP             = "Too many levels of symbolic links.";
const char* const SSEM_ENAMETOOLONG      = "File name too long.";
const char* const SSEM_EHOSTDOWN         = "Host is down.";
const char* const SSEM_EHOSTUNREACH      = "No route to host.";
const char* const SSEM_ENOTEMPTY         = "Directory not empty";

///////////////////////////////////////////////////////////////////////////////
// 类型定义

#ifdef ISE_WIN32
typedef int socklen_t;
#endif
#ifdef ISE_LINUX
typedef int SOCKET;
#define INVALID_SOCKET ((SOCKET)(-1))
#endif

typedef struct sockaddr_in SockAddr;

// 网络协议类型(UDP|TCP)
enum NET_PROTO_TYPE
{
	NPT_UDP,        // UDP
	NPT_TCP         // TCP
};

// DTP 协议类型(TCP|UTP)
enum DTP_PROTO_TYPE
{
	DPT_TCP,        // TCP
	DPT_UTP         // UTP (UDP Transfer Protocol)
};

// 异步连接的状态
enum ASYNC_CONNECT_STATE
{
	ACS_NONE,       // 尚未发起连接
	ACS_CONNECTING, // 尚未连接完毕，且尚未发生错误
	ACS_CONNECTED,  // 连接已建立成功
	ACS_FAILED      // 连接过程中发生了错误，导致连接失败
};

#pragma pack(1)     // 1字节对齐

// Peer地址信息
struct CPeerAddress
{
	UINT nIp;       // IP (主机字节顺序)
	int nPort;      // 端口

	CPeerAddress() : nIp(0), nPort(0) {}
	CPeerAddress(UINT _nIp, int _nPort)
		{ nIp = _nIp;  nPort = _nPort; }
	bool operator == (const CPeerAddress& rhs) const
		{ return (nIp == rhs.nIp && nPort == rhs.nPort); }
	bool operator != (const CPeerAddress& rhs) const
		{ return !((*this) == rhs); }
};

#pragma pack()

// 回调函数定义
typedef void (*UDPSVR_ON_RECV_DATA_PROC)(void *pParam, void *pPacketBuffer,
	int nPacketSize, const CPeerAddress& PeerAddr);
typedef void (*TCPSVR_ON_CREATE_CONN_PROC)(void *pParam, CTcpServer *pTcpServer,
	SOCKET nSocketHandle, const CPeerAddress& PeerAddr, CBaseTcpConnection*& pConnection);
typedef void (*TCPSVR_ON_ACCEPT_CONN_PROC)(void *pParam, CTcpServer *pTcpServer,
	CBaseTcpConnection *pConnection);

///////////////////////////////////////////////////////////////////////////////
// 杂项函数

// 网络初始化/结束化
void NetworkInitialize();
void NetworkFinalize();
bool IsNetworkInited();
void EnsureNetworkInited();

// 解决跨平台问题的函数
int IseSocketGetLastError();
string IseSocketGetErrorMsg(int nError);
string IseSocketGetLastErrMsg();
void IseCloseSocket(SOCKET nHandle);

// 杂项函数
string IpToString(UINT nIp);
UINT StringToIp(const string& strString);
void GetSocketAddr(SockAddr& SockAddr, UINT nIpHostValue, int nPort);
int GetFreePort(NET_PROTO_TYPE nProto, int nStartPort, int nCheckTimes);
void GetLocalIpList(StringArray& IpList);
string GetLocalIp();
string LookupHostAddr(const string& strHost);
void IseThrowSocketLastError();

///////////////////////////////////////////////////////////////////////////////
// class CSocket - 套接字类

class CSocket
{
public:
	friend class CTcpServer;

protected:
	bool m_bActive;     // 套接字是否准备就绪
	SOCKET m_nHandle;   // 套接字句柄
	int m_nDomain;      // 套接字的协议家族 (PF_UNIX, PF_INET, PF_INET6, PF_IPX, ...)
	int m_nType;        // 套接字类型，必须指定 (SOCK_STREAM, SOCK_DGRAM, SOCK_RAW, SOCK_RDM, SOCK_SEQPACKET)
	int m_nProtocol;    // 套接字所用协议，可为0 (IPPROTO_IP, IPPROTO_UDP, IPPROTO_TCP, ...)
	bool m_bBlockMode;  // 是否为阻塞模式 (缺省为阻塞模式)

private:
	void DoSetBlockMode(SOCKET nHandle, bool bValue);
	void DoClose();

protected:
	void SetActive(bool bValue);
	void SetDomain(int nValue);
	void SetType(int nValue);
	void SetProtocol(int nValue);

	void Bind(int nPort);
public:
	CSocket();
	virtual ~CSocket();

	virtual void Open();
	virtual void Close();

	bool GetActive() const { return m_bActive; }
	SOCKET GetHandle() const { return m_nHandle; }
	bool GetBlockMode() const { return m_bBlockMode; }
	void SetBlockMode(bool bValue);
	void SetHandle(SOCKET nValue);
};

///////////////////////////////////////////////////////////////////////////////
// class CUdpSocket - UDP 套接字类

class CUdpSocket : public CSocket
{
public:
	CUdpSocket()
	{
		m_nType = SOCK_DGRAM;
		m_nProtocol = IPPROTO_UDP;
		m_bBlockMode = true;
	}

	int RecvBuffer(void *pBuffer, int nSize);
	int RecvBuffer(void *pBuffer, int nSize, CPeerAddress& PeerAddr);
	int SendBuffer(void *pBuffer, int nSize, const CPeerAddress& PeerAddr, int nSendTimes = 1);

	virtual void Open();
};

///////////////////////////////////////////////////////////////////////////////
// class CUdpClient - UDP Client 类

class CUdpClient : public CUdpSocket
{
public:
	CUdpClient() { Open(); }
};

///////////////////////////////////////////////////////////////////////////////
// class CUdpServer - UDP Server 类

class CUdpServer : public CUdpSocket
{
public:
	friend class CUdpListenerThread;
private:
	int m_nLocalPort;
	CUdpListenerThreadPool *m_pListenerThreadPool;
	CCallBackDef<UDPSVR_ON_RECV_DATA_PROC> m_OnRecvData;
	PVOID m_pCustomData;
private:
	void DataReceived(void *pPacketBuffer, int nPacketSize, const CPeerAddress& PeerAddr);
protected:
	virtual void StartListenerThreads();
	virtual void StopListenerThreads();
public:
	CUdpServer();
	virtual ~CUdpServer();

	virtual void Open();
	virtual void Close();

	int GetLocalPort() const { return m_nLocalPort; }
	void SetLocalPort(int nValue);

	int GetListenerThreadCount() const;
	void SetListenerThreadCount(int nValue);

	PVOID& CustomData() { return m_pCustomData; }

	void SetOnRecvDataCallBack(UDPSVR_ON_RECV_DATA_PROC pProc, void *pParam = NULL);
};

///////////////////////////////////////////////////////////////////////////////
// class CTcpSocket - TCP 套接字类

class CTcpSocket : public CSocket
{
public:
	CTcpSocket()
	{
		m_nType = SOCK_STREAM;
		m_nProtocol = IPPROTO_TCP;
		m_bBlockMode = false;
	}
};

///////////////////////////////////////////////////////////////////////////////
// class CBaseTcpConnection - TCP Connection 基类

class CBaseTcpConnection
{
protected:
	CTcpSocket m_Socket;
	CPeerAddress m_PeerAddr;
	PVOID m_pCustomData;
private:
	int DoSyncSendBuffer(void *pBuffer, int nSize, int nTimeOutMSecs = -1);
	int DoSyncRecvBuffer(void *pBuffer, int nSize, int nTimeOutMSecs = -1);
	int DoAsyncSendBuffer(void *pBuffer, int nSize);
	int DoAsyncRecvBuffer(void *pBuffer, int nSize);
protected:
	int SendBuffer(void *pBuffer, int nSize, bool bSyncMode = false, int nTimeOutMSecs = -1);
	int RecvBuffer(void *pBuffer, int nSize, bool bSyncMode = false, int nTimeOutMSecs = -1);
protected:
	virtual void DoDisconnect();
public:
	CBaseTcpConnection();
	CBaseTcpConnection(SOCKET nSocketHandle, const CPeerAddress& PeerAddr);
	virtual ~CBaseTcpConnection() {}

	virtual bool IsConnected() const;
	void Disconnect();

	const CTcpSocket& GetSocket() const { return m_Socket; }
	const CPeerAddress& GetPeerAddr() const { return m_PeerAddr; }
	PVOID& CustomData() { return m_pCustomData; }
};

///////////////////////////////////////////////////////////////////////////////
// class CTcpClient - TCP Client 类

class CTcpClient : public CBaseTcpConnection
{
public:
	// 阻塞式连接
	void Connect(const string& strIp, int nPort);
	// 异步(非阻塞式)连接 (返回 enum ASYNC_CONNECT_STATE)
	int AsyncConnect(const string& strIp, int nPort, int nTimeOutMSecs = -1);
	// 检查异步连接的状态 (返回 enum ASYNC_CONNECT_STATE)
	int CheckAsyncConnectState(int nTimeOutMSecs = -1);

	using CBaseTcpConnection::SendBuffer;
	using CBaseTcpConnection::RecvBuffer;
};

///////////////////////////////////////////////////////////////////////////////
// class CTcpServer - TCP Server 类

class CTcpServer
{
public:
	friend class CTcpListenerThread;
public:
	enum { LISTEN_QUEUE_SIZE = 30 };   // TCP监听队列长度
private:
	CTcpSocket m_Socket;
	int m_nLocalPort;
	CTcpListenerThread *m_pListenerThread;
	CCallBackDef<TCPSVR_ON_CREATE_CONN_PROC> m_OnCreateConn;
	CCallBackDef<TCPSVR_ON_ACCEPT_CONN_PROC> m_OnAcceptConn;
	PVOID m_pCustomData;
private:
	CBaseTcpConnection* CreateConnection(SOCKET nSocketHandle, const CPeerAddress& PeerAddr);
	void AcceptConnection(CBaseTcpConnection *pConnection);
protected:
	virtual void StartListenerThread();
	virtual void StopListenerThread();
public:
	CTcpServer();
	virtual ~CTcpServer();

	virtual void Open();
	virtual void Close();

	bool GetActive() const { return m_Socket.GetActive(); }
	void SetActive(bool bValue);

	int GetLocalPort() const { return m_nLocalPort; }
	void SetLocalPort(int nValue);

	const CTcpSocket& GetSocket() const { return m_Socket; }
	PVOID& CustomData() { return m_pCustomData; }

	void SetOnCreateConnCallBack(TCPSVR_ON_CREATE_CONN_PROC pProc, void *pParam = NULL);
	void SetOnAcceptConnCallBack(TCPSVR_ON_ACCEPT_CONN_PROC pProc, void *pParam = NULL);
};

///////////////////////////////////////////////////////////////////////////////
// class CListenerThread - 监听线程类

class CListenerThread : public CThread
{
protected:
	virtual void Execute() {}
public:
	CListenerThread()
	{
#ifdef ISE_WIN32
		SetPriority(THREAD_PRI_HIGHEST);
#endif
#ifdef ISE_LINUX
		SetPolicy(THREAD_POL_RR);
		SetPriority(THREAD_PRI_HIGH);
#endif
	}
	virtual ~CListenerThread() {}
};

///////////////////////////////////////////////////////////////////////////////
// class CUdpListenerThread - UDP服务器监听线程类

class CUdpListenerThread : public CListenerThread
{
private:
	CUdpListenerThreadPool *m_pThreadPool;  // 所属线程池
	CUdpServer *m_pUdpServer;               // 所属UDP服务器
	int m_nIndex;                           // 线程在池中的索引号(0-based)
protected:
	virtual void Execute();
public:
	explicit CUdpListenerThread(CUdpListenerThreadPool *pThreadPool, int nIndex);
	virtual ~CUdpListenerThread();
};

///////////////////////////////////////////////////////////////////////////////
// class CUdpListenerThreadPool - UDP服务器监听线程池类

class CUdpListenerThreadPool
{
private:
	CUdpServer *m_pUdpServer;               // 所属UDP服务器
	CThreadList m_ThreadList;               // 线程列表
	int m_nMaxThreadCount;                  // 允许最大线程数量
public:
	explicit CUdpListenerThreadPool(CUdpServer *pUdpServer);
	virtual ~CUdpListenerThreadPool();

	void RegisterThread(CUdpListenerThread *pThread);
	void UnregisterThread(CUdpListenerThread *pThread);

	void StartThreads();
	void StopThreads();

	int GetMaxThreadCount() const { return m_nMaxThreadCount; }
	void SetMaxThreadCount(int nValue) { m_nMaxThreadCount = nValue; }

	// 返回所属UDP服务器
	CUdpServer& GetUdpServer() { return *m_pUdpServer; }
};

///////////////////////////////////////////////////////////////////////////////
// class CTcpListenerThread - TCP服务器监听线程类

class CTcpListenerThread : public CListenerThread
{
private:
	CTcpServer *m_pTcpServer;
protected:
	virtual void Execute();
public:
	explicit CTcpListenerThread(CTcpServer *pTcpServer);
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_SOCKET_H_
