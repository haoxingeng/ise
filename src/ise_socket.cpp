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
// 文件名称: ise_socket.cpp
// 功能描述: 网络基础类库
///////////////////////////////////////////////////////////////////////////////

#include "ise_socket.h"
#include "ise_sysutils.h"
#include "ise_exceptions.h"

#ifdef ISE_COMPILER_VC
#pragma comment(lib, "ws2_32.lib")
#endif

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// 杂项函数

static int s_networkInitCount = 0;

//-----------------------------------------------------------------------------
// 描述: 网络初始化 (若失败则抛出异常)
//-----------------------------------------------------------------------------
void networkInitialize()
{
	s_networkInitCount++;
	if (s_networkInitCount > 1) return;

#ifdef ISE_WIN32
	WSAData wsd;
	if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0)
	{
		s_networkInitCount--;
		iseThrowSocketLastError();
	}
#endif
}

//-----------------------------------------------------------------------------
// 描述: 网络结束化
//-----------------------------------------------------------------------------
void networkFinalize()
{
	if (s_networkInitCount > 0)
		s_networkInitCount--;
	if (s_networkInitCount != 0) return;

#ifdef ISE_WIN32
	WSACleanup();
#endif
}

//-----------------------------------------------------------------------------

bool isNetworkInited()
{
	return (s_networkInitCount > 0);
}

//-----------------------------------------------------------------------------

void ensureNetworkInited()
{
	if (!isNetworkInited())
		networkInitialize();
}

//-----------------------------------------------------------------------------
// 描述: 取得最后的错误代码
//-----------------------------------------------------------------------------
int iseSocketGetLastError()
{
#ifdef ISE_WIN32
	return WSAGetLastError();
#endif
#ifdef ISE_LINUX
	return errno;
#endif
}

//-----------------------------------------------------------------------------
// 描述: 返回错误信息
//-----------------------------------------------------------------------------
string iseSocketGetErrorMsg(int errorCode)
{
	string result;
	const char *p = "";

	switch (errorCode)
	{
	case SS_EINTR:              p = SSEM_EINTR;                break;
	case SS_EBADF:              p = SSEM_EBADF;                break;
	case SS_EACCES:             p = SSEM_EACCES;               break;
	case SS_EFAULT:             p = SSEM_EFAULT;               break;
	case SS_EINVAL:             p = SSEM_EINVAL;               break;
	case SS_EMFILE:             p = SSEM_EMFILE;               break;

	case SS_EWOULDBLOCK:        p = SSEM_EWOULDBLOCK;          break;
	case SS_EINPROGRESS:        p = SSEM_EINPROGRESS;          break;
	case SS_EALREADY:           p = SSEM_EALREADY;             break;
	case SS_ENOTSOCK:           p = SSEM_ENOTSOCK;             break;
	case SS_EDESTADDRREQ:       p = SSEM_EDESTADDRREQ;         break;
	case SS_EMSGSIZE:           p = SSEM_EMSGSIZE;             break;
	case SS_EPROTOTYPE:         p = SSEM_EPROTOTYPE;           break;
	case SS_ENOPROTOOPT:        p = SSEM_ENOPROTOOPT;          break;
	case SS_EPROTONOSUPPORT:    p = SSEM_EPROTONOSUPPORT;      break;
	case SS_ESOCKTNOSUPPORT:    p = SSEM_ESOCKTNOSUPPORT;      break;
	case SS_EOPNOTSUPP:         p = SSEM_EOPNOTSUPP;           break;
	case SS_EPFNOSUPPORT:       p = SSEM_EPFNOSUPPORT;         break;
	case SS_EAFNOSUPPORT:       p = SSEM_EAFNOSUPPORT;         break;
	case SS_EADDRINUSE:         p = SSEM_EADDRINUSE;           break;
	case SS_EADDRNOTAVAIL:      p = SSEM_EADDRNOTAVAIL;        break;
	case SS_ENETDOWN:           p = SSEM_ENETDOWN;             break;
	case SS_ENETUNREACH:        p = SSEM_ENETUNREACH;          break;
	case SS_ENETRESET:          p = SSEM_ENETRESET;            break;
	case SS_ECONNABORTED:       p = SSEM_ECONNABORTED;         break;
	case SS_ECONNRESET:         p = SSEM_ECONNRESET;           break;
	case SS_ENOBUFS:            p = SSEM_ENOBUFS;              break;
	case SS_EISCONN:            p = SSEM_EISCONN;              break;
	case SS_ENOTCONN:           p = SSEM_ENOTCONN;             break;
	case SS_ESHUTDOWN:          p = SSEM_ESHUTDOWN;            break;
	case SS_ETOOMANYREFS:       p = SSEM_ETOOMANYREFS;         break;
	case SS_ETIMEDOUT:          p = SSEM_ETIMEDOUT;            break;
	case SS_ECONNREFUSED:       p = SSEM_ECONNREFUSED;         break;
	case SS_ELOOP:              p = SSEM_ELOOP;                break;
	case SS_ENAMETOOLONG:       p = SSEM_ENAMETOOLONG;         break;
	case SS_EHOSTDOWN:          p = SSEM_EHOSTDOWN;            break;
	case SS_EHOSTUNREACH:       p = SSEM_EHOSTUNREACH;         break;
	case SS_ENOTEMPTY:          p = SSEM_ENOTEMPTY;            break;
	}

	result = formatString(SSEM_ERROR, errorCode, p);
	return result;
}

//-----------------------------------------------------------------------------
// 描述: 取得最后错误的对应信息
//-----------------------------------------------------------------------------
string iseSocketGetLastErrMsg()
{
	return iseSocketGetErrorMsg(iseSocketGetLastError());
}

//-----------------------------------------------------------------------------
// 描述: 关闭套接字
//-----------------------------------------------------------------------------
void iseCloseSocket(SOCKET handle)
{
#ifdef ISE_WIN32
	closesocket(handle);
#endif
#ifdef ISE_LINUX
	close(handle);
#endif
}

//-----------------------------------------------------------------------------
// 描述: 整形IP(主机字节顺序) -> 串型IP
//-----------------------------------------------------------------------------
string ipToString(UINT ip)
{
#pragma pack(1)
	union CIpUnion
	{
		UINT value;
		struct
		{
			unsigned char ch1;  // value的最低字节
			unsigned char ch2;
			unsigned char ch3;
			unsigned char ch4;
		} Bytes;
	} IpUnion;
#pragma pack()
	char str[64];

	IpUnion.value = ip;
	sprintf(str, "%u.%u.%u.%u", IpUnion.Bytes.ch4, IpUnion.Bytes.ch3,
		IpUnion.Bytes.ch2, IpUnion.Bytes.ch1);
	return &str[0];
}

//-----------------------------------------------------------------------------
// 描述: 串型IP -> 整形IP(主机字节顺序)
//-----------------------------------------------------------------------------
UINT stringToIp(const string& str)
{
#pragma pack(1)
	union CIpUnion
	{
		UINT value;
		struct
		{
			unsigned char ch1;
			unsigned char ch2;
			unsigned char ch3;
			unsigned char ch4;
		} Bytes;
	} ipUnion;
#pragma pack()
	IntegerArray intList;

	splitStringToInt(str, '.', intList);
	if (intList.size() == 4)
	{
		ipUnion.Bytes.ch1 = intList[3];
		ipUnion.Bytes.ch2 = intList[2];
		ipUnion.Bytes.ch3 = intList[1];
		ipUnion.Bytes.ch4 = intList[0];
		return ipUnion.value;
	}
	else
		return 0;
}

//-----------------------------------------------------------------------------
// 描述: 填充 SockAddr 结构
//-----------------------------------------------------------------------------
void getSocketAddr(SockAddr& sockAddr, UINT ipHostValue, int port)
{
	memset(&sockAddr, 0, sizeof(sockAddr));
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.s_addr = htonl(ipHostValue);
	sockAddr.sin_port = htons(port);
}

//-----------------------------------------------------------------------------
// 描述: 取得空闲端口号
// 参数:
//   proto      - 网络协议(UDP,TCP)
//   startPort  - 起始端口号
//   checkTimes - 检测次数
// 返回:
//   空闲端口号 (若失败则返回 0)
//-----------------------------------------------------------------------------
int getFreePort(NET_PROTO_TYPE proto, int startPort, int checkTimes)
{
	int i, result = 0;
	bool success;
	SockAddr addr;

	ise::networkInitialize();
	struct AutoFinalizer {
		~AutoFinalizer() { ise::networkFinalize(); }
	} autoFinalizer;

	SOCKET s = socket(PF_INET, (proto == NPT_UDP? SOCK_DGRAM : SOCK_STREAM), IPPROTO_IP);
	if (s == INVALID_SOCKET) return 0;

	success = false;
	for (i = 0; i < checkTimes; i++)
	{
		result = startPort + i;
		getSocketAddr(addr, ntohl(INADDR_ANY), result);
		if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) != -1)
		{
			success = true;
			break;
		}
	}

	iseCloseSocket(s);
	if (!success) result = 0;
	return result;
}

//-----------------------------------------------------------------------------
// 描述: 取得本机IP列表
//-----------------------------------------------------------------------------
void getLocalIpList(StringArray& ipList)
{
#ifdef ISE_WIN32
	char hostName[250];
	hostent *hostEnt;
	in_addr **addrPtr;

	ipList.clear();
	gethostname(hostName, sizeof(hostName));
	hostEnt = gethostbyname(hostName);
	if (hostEnt)
	{
		addrPtr = (in_addr**)(hostEnt->h_addr_list);
		int i = 0;
		while (addrPtr[i])
		{
			UINT ip = ntohl( *(UINT*)(addrPtr[i]) );
			ipList.push_back(ipToString(ip));
			i++;
		}
	}
#endif
#ifdef ISE_LINUX
	const int MAX_INTERFACES = 16;
	int fd, intfCount;
	struct ifreq buf[MAX_INTERFACES];
	struct ifconf ifc;

	ipList.clear();
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)
	{
		ifc.ifc_len = sizeof(buf);
		ifc.ifc_buf = (caddr_t) buf;
		if (!ioctl(fd, SIOCGIFCONF, (char*)&ifc))
		{
			intfCount = ifc.ifc_len / sizeof(struct ifreq);
			for (int i = 0; i < intfCount; i++)
			{
				ioctl(fd, SIOCGIFADDR, (char*)&buf[i]);
				UINT nIp = ((struct sockaddr_in*)(&buf[i].ifr_addr))->sin_addr.s_addr;
				ipList.push_back(ipToString(ntohl(nIp)));
			}
		}
		close(fd);
	}
#endif
}

//-----------------------------------------------------------------------------
// 描述: 取得本机IP
//-----------------------------------------------------------------------------
string getLocalIp()
{
	StringArray ipList;
	string result;

	getLocalIpList(ipList);
	if (!ipList.empty())
	{
		if (ipList.size() == 1)
			result = ipList[0];
		else
		{
			for (UINT i = 0; i < ipList.size(); i++)
				if (ipList[i] != "127.0.0.1")
				{
					result = ipList[i];
					break;
				}

			if (result.length() == 0)
				result = ipList[0];
		}
	}

	return result;
}

//-----------------------------------------------------------------------------
// 描述: 域名地址 -> IP地址
// 备注: 若失败，则返回空字符串。
//-----------------------------------------------------------------------------
string lookupHostAddr(const string& host)
{
	string result = "";

	struct hostent* hostentPtr = gethostbyname(host.c_str());
	if (hostentPtr != NULL)
		result = ipToString(ntohl(((struct in_addr *)hostentPtr->h_addr)->s_addr));

	return result;
}

//-----------------------------------------------------------------------------
// 描述: 取最后的错误码并抛出异常
//-----------------------------------------------------------------------------
void iseThrowSocketLastError()
{
	iseThrowSocketException(iseSocketGetLastErrMsg().c_str());
}

///////////////////////////////////////////////////////////////////////////////
// class Socket

Socket::Socket() :
	isActive_(false),
	handle_(INVALID_SOCKET),
	domain_(PF_INET),
	type_(SOCK_STREAM),
	protocol_(IPPROTO_IP),
	isBlockMode_(true)
{
	// nothing
}

//-----------------------------------------------------------------------------

Socket::~Socket()
{
	close();
}

//-----------------------------------------------------------------------------

void Socket::doSetBlockMode(SOCKET handle, bool value)
{
#ifdef ISE_WIN32
	UINT notBlock = (value? 0 : 1);
	if (ioctlsocket(handle, FIONBIO, (u_long*)&notBlock) < 0)
		iseThrowSocketLastError();
#endif
#ifdef ISE_LINUX
	int flag = fcntl(handle, F_GETFL);

	if (value)
		flag &= ~O_NONBLOCK;
	else
		flag |= O_NONBLOCK;

	if (fcntl(handle, F_SETFL, flag) < 0)
		iseThrowSocketLastError();
#endif
}

//-----------------------------------------------------------------------------

void Socket::doClose()
{
	shutdown(handle_, SS_SD_BOTH);
	iseCloseSocket(handle_);
	handle_ = INVALID_SOCKET;
	isActive_ = false;
}

//-----------------------------------------------------------------------------

void Socket::setActive(bool value)
{
	if (value != isActive_)
	{
		if (value) open();
		else close();
	}
}

//-----------------------------------------------------------------------------

void Socket::setDomain(int value)
{
	if (value != domain_)
	{
		if (isActive()) close();
		domain_ = value;
	}
}

//-----------------------------------------------------------------------------

void Socket::setType(int value)
{
	if (value != type_)
	{
		if (isActive()) close();
		type_ = value;
	}
}

//-----------------------------------------------------------------------------

void Socket::setProtocol(int value)
{
	if (value != protocol_)
	{
		if (isActive()) close();
		protocol_ = value;
	}
}

//-----------------------------------------------------------------------------

void Socket::setBlockMode(bool value)
{
	// 此处不应作 value != isBlockMode_ 的判断，因为在不同的平台下，
	// 套接字阻塞方式的缺省值不一样。
	if (isActive_)
		doSetBlockMode(handle_, value);
	isBlockMode_ = value;
}

//-----------------------------------------------------------------------------

void Socket::setHandle(SOCKET value)
{
	if (value != handle_)
	{
		if (isActive()) close();
		handle_ = value;
		if (handle_ != INVALID_SOCKET)
			isActive_ = true;
	}
}

//-----------------------------------------------------------------------------
// 描述: 绑定套接字
//-----------------------------------------------------------------------------
void Socket::bind(int port)
{
	SockAddr addr;
	int value = 1;

	getSocketAddr(addr, ntohl(INADDR_ANY), port);

	// 强制重新绑定，而不受其它因素的影响
	setsockopt(handle_, SOL_SOCKET, SO_REUSEADDR, (char*)&value, sizeof(int));
	// 绑定套接字
	if (::bind(handle_, (struct sockaddr*)&addr, sizeof(addr)) < 0)
		iseThrowSocketLastError();
}

//-----------------------------------------------------------------------------
// 描述: 打开套接字
//-----------------------------------------------------------------------------
void Socket::open()
{
	if (!isActive_)
	{
		try
		{
			SOCKET handle;
			handle = socket(domain_, type_, protocol_);
			if (handle == INVALID_SOCKET)
				iseThrowSocketLastError();
			isActive_ = (handle != INVALID_SOCKET);
			if (isActive_)
			{
				handle_ = handle;
				setBlockMode(isBlockMode_);
			}
		}
		catch (SocketException&)
		{
			doClose();
			throw;
		}
	}
}

//-----------------------------------------------------------------------------
// 描述: 关闭套接字
//-----------------------------------------------------------------------------
void Socket::close()
{
	if (isActive_) doClose();
}

///////////////////////////////////////////////////////////////////////////////
// class UdpSocket

//-----------------------------------------------------------------------------
// 描述: 接收数据
//-----------------------------------------------------------------------------
int UdpSocket::recvBuffer(void *buffer, int size)
{
	InetAddress peerAddr;
	return recvBuffer(buffer, size, peerAddr);
}

//-----------------------------------------------------------------------------
// 描述: 接收数据
//-----------------------------------------------------------------------------
int UdpSocket::recvBuffer(void *buffer, int size, InetAddress& peerAddr)
{
	SockAddr addr;
	int bytes;
	socklen_t sockLen = sizeof(addr);

	memset(&addr, 0, sizeof(addr));
	bytes = recvfrom(handle_, (char*)buffer, size, 0, (struct sockaddr*)&addr, &sockLen);

	if (bytes > 0)
	{
		peerAddr.nIp = ntohl(addr.sin_addr.s_addr);
		peerAddr.port = ntohs(addr.sin_port);
	}

	return bytes;
}

//-----------------------------------------------------------------------------
// 描述: 发送数据
//-----------------------------------------------------------------------------
int UdpSocket::sendBuffer(void *buffer, int size, const InetAddress& peerAddr, int sendTimes)
{
	int result = 0;
	SockAddr addr;
	socklen_t sockLen = sizeof(addr);

	getSocketAddr(addr, peerAddr.nIp, peerAddr.port);

	for (int i = 0; i < sendTimes; i++)
		result = sendto(handle_, (char*)buffer, size, 0, (struct sockaddr*)&addr, sockLen);

	return result;
}

//-----------------------------------------------------------------------------
// 描述: 打开套接字
//-----------------------------------------------------------------------------
void UdpSocket::open()
{
	Socket::open();

#ifdef ISE_WIN32
	if (isActive_)
	{
		// Windows下，当收到ICMP包("ICMP port unreachable")后，recvfrom将返回-1，
		// 错误为 WSAECONNRESET(10054)。用下面的方法禁用该行为。

		#define IOC_VENDOR        0x18000000
		#define _WSAIOW(x,y)      (IOC_IN|(x)|(y))
		#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR,12)

		DWORD bytesReturned = 0;
		BOOL newBehavior = FALSE;
		::WSAIoctl(getHandle(), SIO_UDP_CONNRESET, &newBehavior, sizeof(newBehavior),
			NULL, 0, &bytesReturned, NULL, NULL);
	}
#endif
}

///////////////////////////////////////////////////////////////////////////////
// class UdpServer

UdpServer::UdpServer() :
	localPort_(0),
	listenerThreadPool_(NULL)
{
	listenerThreadPool_ = new UdpListenerThreadPool(this);
	setListenerThreadCount(1);
}

//-----------------------------------------------------------------------------

UdpServer::~UdpServer()
{
	delete listenerThreadPool_;
}

//-----------------------------------------------------------------------------
// 描述: 收到数据包
//-----------------------------------------------------------------------------
void UdpServer::dataReceived(void *packetBuffer, int packetSize, const InetAddress& peerAddr)
{
	if (onRecvData_.proc)
		onRecvData_.proc(onRecvData_.param, packetBuffer, packetSize, peerAddr);
}

//-----------------------------------------------------------------------------
// 描述: 启动监听线程
//-----------------------------------------------------------------------------
void UdpServer::startListenerThreads()
{
	listenerThreadPool_->startThreads();
}

//-----------------------------------------------------------------------------
// 描述: 停止监听线程
//-----------------------------------------------------------------------------
void UdpServer::stopListenerThreads()
{
	listenerThreadPool_->stopThreads();
}

//-----------------------------------------------------------------------------
// 描述: 设置监听端口
//-----------------------------------------------------------------------------
void UdpServer::setLocalPort(int value)
{
	if (value != localPort_)
	{
		if (isActive()) close();
		localPort_ = value;
	}
}

//-----------------------------------------------------------------------------
// 描述: 开启 UDP 服务器
//-----------------------------------------------------------------------------
void UdpServer::open()
{
	try
	{
		if (!isActive_)
		{
			UdpSocket::open();
			if (isActive_)
			{
				bind(localPort_);
				startListenerThreads();
			}
		}
	}
	catch (SocketException&)
	{
		close();
		throw;
	}
}

//-----------------------------------------------------------------------------
// 描述: 关闭 UDP 服务器
//-----------------------------------------------------------------------------
void UdpServer::close()
{
	if (isActive())
	{
		stopListenerThreads();
		UdpSocket::close();
	}
}

//-----------------------------------------------------------------------------
// 描述: 取得监听线程的数量
//-----------------------------------------------------------------------------
int UdpServer::getListenerThreadCount() const
{
	return listenerThreadPool_->getMaxThreadCount();
}

//-----------------------------------------------------------------------------
// 描述: 设置监听线程的数量
//-----------------------------------------------------------------------------
void UdpServer::setListenerThreadCount(int value)
{
	if (value < 1) value = 1;
	listenerThreadPool_->setMaxThreadCount(value);
}

//-----------------------------------------------------------------------------
// 描述: 设置“收到数据包”的回调
//-----------------------------------------------------------------------------
void UdpServer::setOnRecvDataCallback(UDPSVR_ON_RECV_DATA_PROC proc, void *param)
{
	onRecvData_.proc = proc;
	onRecvData_.param = param;
}

///////////////////////////////////////////////////////////////////////////////
// class BaseTcpConnection

BaseTcpConnection::BaseTcpConnection()
{
	socket_.setBlockMode(false);
}

//-----------------------------------------------------------------------------

BaseTcpConnection::BaseTcpConnection(SOCKET socketHandle, const InetAddress& peerAddr)
{
	socket_.setHandle(socketHandle);
	socket_.setBlockMode(false);
	peerAddr_ = peerAddr;
}

//-----------------------------------------------------------------------------
// 描述: 发送数据
//   timeoutMSecs - 指定超时时间(毫秒)，若超过指定时间仍未发送完全部数据则退出函数。
//                   若 timeoutMSecs 为 -1，则表示不进行超时检测。
// 返回:
//   < 0    - 未发出任何数据，且发送数据过程发生了错误。
//   >= 0   - 实际发出的字节数。
// 备注:
//   1. 不会抛出异常。
//   2. 若仅因为超时而返回，返回值不会小于0。
//   3. 此处采用非阻塞套接字模式，以便能及时退出。
//-----------------------------------------------------------------------------
int BaseTcpConnection::doSyncSendBuffer(void *buffer, int size, int timeoutMSecs)
{
	const int SELECT_WAIT_MSEC = 250;    // 每次等待时间 (毫秒)

	int result = -1;
	bool error = false;
	fd_set fds;
	struct timeval tv;
	SOCKET socketHandle = socket_.getHandle();
	int n, r, remainSize, index;
	UINT startTime, elapsedMSecs;

	if (size <= 0 || !socket_.isActive())
		return result;

	remainSize = size;
	index = 0;
	startTime = getCurTicks();

	while (socket_.isActive() && remainSize > 0)
	try
	{
		tv.tv_sec = 0;
		tv.tv_usec = (timeoutMSecs? SELECT_WAIT_MSEC * 1000 : 0);

		FD_ZERO(&fds);
		FD_SET((UINT)socketHandle, &fds);

		r = select(socketHandle + 1, NULL, &fds, NULL, &tv);
		if (r < 0)
		{
			if (iseSocketGetLastError() != SS_EINTR)
			{
				error = true;    // error
				break;
			}
		}

		if (r > 0 && socket_.isActive() && FD_ISSET(socketHandle, &fds))
		{
			n = send(socketHandle, &((char*)buffer)[index], remainSize, 0);
			if (n <= 0)
			{
				int errorCode = iseSocketGetLastError();
				if ((n == 0) || (errorCode != SS_EWOULDBLOCK && errorCode != SS_EINTR))
				{
					error = true;    // error
					disconnect();
					break;
				}
				else
					n = 0;
			}

			index += n;
			remainSize -= n;
		}

		// 如果需要超时检测
		if (timeoutMSecs >= 0 && remainSize > 0)
		{
			elapsedMSecs = getTickDiff(startTime, getCurTicks());
			if (elapsedMSecs >= (UINT)timeoutMSecs)
				break;
		}
	}
	catch (...)
	{
		error = true;
		break;
	}

	if (index > 0)
		result = index;
	else if (error)
		result = -1;
	else
		result = 0;

	return result;
}

//-----------------------------------------------------------------------------
// 描述: 接收数据
// 参数:
//   timeoutMSecs - 指定超时时间(毫秒)，若超过指定时间仍未接收完全部数据则退出函数。
//                   若 timeoutMSecs 为 -1，则表示不进行超时检测。
// 返回:
//   < 0    - 未接收到任何数据，且接收数据过程发生了错误。
//   >= 0   - 实际接收到的字节数。
// 备注:
//   1. 不会抛出异常。
//   2. 若仅因为超时而返回，返回值不会小于0。
//   3. 此处采用非阻塞套接字模式，以便能及时退出。
//-----------------------------------------------------------------------------
int BaseTcpConnection::doSyncRecvBuffer(void *buffer, int size, int timeoutMSecs)
{
	const int SELECT_WAIT_MSEC = 250;    // 每次等待时间 (毫秒)

	int result = -1;
	bool error = false;
	fd_set fds;
	struct timeval tv;
	SOCKET socketHandle = socket_.getHandle();
	int n, r, remainSize, index;
	UINT startTime, elapsedMSecs;

	if (size <= 0 || !socket_.isActive())
		return result;

	remainSize = size;
	index = 0;
	startTime = getCurTicks();

	while (socket_.isActive() && remainSize > 0)
	try
	{
		tv.tv_sec = 0;
		tv.tv_usec = (timeoutMSecs? SELECT_WAIT_MSEC * 1000 : 0);

		FD_ZERO(&fds);
		FD_SET((UINT)socketHandle, &fds);

		r = select(socketHandle + 1, &fds, NULL, NULL, &tv);
		if (r < 0)
		{
			if (iseSocketGetLastError() != SS_EINTR)
			{
				error = true;    // error
				break;
			}
		}

		if (r > 0 && socket_.isActive() && FD_ISSET(socketHandle, &fds))
		{
			n = recv(socketHandle, &((char*)buffer)[index], remainSize, 0);
			if (n <= 0)
			{
				int errorCode = iseSocketGetLastError();
				if ((n == 0) || (errorCode != SS_EWOULDBLOCK && errorCode != SS_EINTR))
				{
					error = true;    // error
					disconnect();
					break;
				}
				else
					n = 0;
			}

			index += n;
			remainSize -= n;
		}

		// 如果需要超时检测
		if (timeoutMSecs >= 0 && remainSize > 0)
		{
			elapsedMSecs = getTickDiff(startTime, getCurTicks());
			if (elapsedMSecs >= (UINT)timeoutMSecs)
				break;
		}
	}
	catch (...)
	{
		error = true;
		break;
	}

	if (index > 0)
		result = index;
	else if (error)
		result = -1;
	else
		result = 0;

	return result;
}

//-----------------------------------------------------------------------------
// 描述: 发送数据 (非阻塞)
// 返回:
//   < 0    - 未发送任何数据，且发送数据过程发生了错误。
//   >= 0   - 实际发出的字节数。
// 备注:
//   不会抛出异常。
//-----------------------------------------------------------------------------
int BaseTcpConnection::doAsyncSendBuffer(void *buffer, int size)
{
	int result = -1;
	try
	{
		result = send(socket_.getHandle(), (char*)buffer, size, 0);
		if (result <= 0)
		{
			int errorCode = iseSocketGetLastError();
			if ((result == 0) || (errorCode != SS_EWOULDBLOCK && errorCode != SS_EINTR))
			{
				disconnect();    // error
				result = -1;
			}
			else
				result = 0;
		}
	}
	catch (...)
	{}

	return result;
}

//-----------------------------------------------------------------------------
// 描述: 接收数据 (非阻塞)
// 返回:
//   < 0    - 未接收到任何数据，且接收数据过程发生了错误。
//   >= 0   - 实际接收到的字节数。
// 备注:
//   不会抛出异常。
//-----------------------------------------------------------------------------
int BaseTcpConnection::doAsyncRecvBuffer(void *buffer, int size)
{
	int result = -1;
	try
	{
		result = recv(socket_.getHandle(), (char*)buffer, size, 0);
		if (result <= 0)
		{
			int errorCode = iseSocketGetLastError();
			if ((result == 0) || (errorCode != SS_EWOULDBLOCK && errorCode != SS_EINTR))
			{
				disconnect();    // error
				result = -1;
			}
			else
				result = 0;
		}
	}
	catch (...)
	{}

	return result;
}

//-----------------------------------------------------------------------------
// 描述: 发送数据
//   syncMode     - 是否以同步方式发送
//   timeoutMSecs - 指定超时时间(毫秒)，若超过指定时间仍未发送完全部数据则退出函数。
//                   若 timeoutMSecs 为 -1，则表示不进行超时检测。
// 返回:
//   < 0    - 未发出任何数据，且发送数据过程发生了错误。
//   >= 0   - 实际发出的字节数。
// 备注:
//   1. 不会抛出异常。
//   2. 若仅因为超时而返回，返回值不会小于0。
//-----------------------------------------------------------------------------
int BaseTcpConnection::sendBuffer(void *buffer, int size, bool syncMode, int timeoutMSecs)
{
	int result = size;

	if (syncMode)
		result = doSyncSendBuffer(buffer, size, timeoutMSecs);
	else
		result = doAsyncSendBuffer(buffer, size);

	return result;
}

//-----------------------------------------------------------------------------
// 描述: 接收数据
//   syncMode     - 是否以同步方式接收
//   timeoutMSecs - 指定超时时间(毫秒)，若超过指定时间仍未接收完全部数据则退出函数。
//                   若 timeoutMSecs 为 -1，则表示不进行超时检测。
// 返回:
//   < 0    - 未接收到任何数据，且接收数据过程发生了错误。
//   >= 0   - 实际接收到的字节数。
// 备注:
//   1. 不会抛出异常。
//   2. 若仅因为超时而返回，返回值不会小于0。
//-----------------------------------------------------------------------------
int BaseTcpConnection::recvBuffer(void *buffer, int size, bool syncMode, int timeoutMSecs)
{
	int result = size;

	if (syncMode)
		result = doSyncRecvBuffer(buffer, size, timeoutMSecs);
	else
		result = doAsyncRecvBuffer(buffer, size);

	return result;
}

//-----------------------------------------------------------------------------
// 描述: 断开连接
//-----------------------------------------------------------------------------
void BaseTcpConnection::doDisconnect()
{
	socket_.close();
}

//-----------------------------------------------------------------------------
// 描述: 返回当前是否为连接状态
//-----------------------------------------------------------------------------
bool BaseTcpConnection::isConnected() const
{
	return socket_.isActive();
}

//-----------------------------------------------------------------------------
// 描述: 断开连接
//-----------------------------------------------------------------------------
void BaseTcpConnection::disconnect()
{
	if (isConnected())
		doDisconnect();
}

///////////////////////////////////////////////////////////////////////////////
// class TcpClient

//-----------------------------------------------------------------------------
// 描述: 发起TCP连接请求 (阻塞式)
// 备注: 若连接失败，则抛出异常。
//-----------------------------------------------------------------------------
void TcpClient::connect(const string& ip, int port)
{
	if (isConnected()) disconnect();

	try
	{
		socket_.open();
		if (socket_.isActive())
		{
			SockAddr addr;

			getSocketAddr(addr, stringToIp(ip), port);

			bool oldBlockMode = socket_.isBlockMode();
			socket_.setBlockMode(true);

			if (::connect(socket_.getHandle(), (struct sockaddr*)&addr, sizeof(addr)) < 0)
				iseThrowSocketLastError();

			socket_.setBlockMode(oldBlockMode);
			peerAddr_ = InetAddress(ntohl(addr.sin_addr.s_addr), port);
		}
	}
	catch (SocketException&)
	{
		socket_.close();
		throw;
	}
}

//-----------------------------------------------------------------------------
// 描述: 发起TCP连接请求 (非阻塞式)
// 参数:
//   timeoutMSecs - 最多等待的毫秒数，为-1表示不等待
// 返回:
//   ACS_CONNECTING - 尚未连接完毕，且尚未发生错误
//   ACS_CONNECTED  - 连接已建立成功
//   ACS_FAILED     - 连接过程中发生了错误，导致连接失败
// 备注:
//   不抛异常。
//-----------------------------------------------------------------------------
int TcpClient::asyncConnect(const string& ip, int port, int timeoutMSecs)
{
	int result = ACS_CONNECTING;

	if (isConnected()) disconnect();
	try
	{
		socket_.open();
		if (socket_.isActive())
		{
			SockAddr addr;
			int r;

			getSocketAddr(addr, stringToIp(ip), port);
			socket_.setBlockMode(false);
			r = ::connect(socket_.getHandle(), (struct sockaddr*)&addr, sizeof(addr));
			if (r == 0)
				result = ACS_CONNECTED;
#ifdef ISE_WIN32
			else if (iseSocketGetLastError() != SS_EWOULDBLOCK)
#endif
#ifdef ISE_LINUX
			else if (iseSocketGetLastError() != SS_EINPROGRESS)
#endif
				result = ACS_FAILED;

			peerAddr_ = InetAddress(ntohl(addr.sin_addr.s_addr), port);
		}
	}
	catch (...)
	{
		socket_.close();
		result = ACS_FAILED;
	}

	if (result == ACS_CONNECTING)
		result = checkAsyncConnectState(timeoutMSecs);

	return result;
}

//-----------------------------------------------------------------------------
// 描述: 检查异步连接的状态
// 参数:
//   timeoutMSecs - 最多等待的毫秒数，为-1表示不等待
// 返回:
//   ACS_CONNECTING - 尚未连接完毕，且尚未发生错误
//   ACS_CONNECTED  - 连接已建立成功
//   ACS_FAILED     - 连接过程中发生了错误，导致连接失败
// 备注:
//   不抛异常。
//-----------------------------------------------------------------------------
int TcpClient::checkAsyncConnectState(int timeoutMSecs)
{
	if (!socket_.isActive()) return ACS_FAILED;

	const int WAIT_STEP = 100;   // ms
	int result = ACS_CONNECTING;
	SOCKET handle = getSocket().getHandle();
	fd_set rset, wset;
	struct timeval tv;
	int ms = 0;

	timeoutMSecs = max(timeoutMSecs, -1);

	while (true)
	{
		tv.tv_sec = 0;
		tv.tv_usec = (timeoutMSecs? WAIT_STEP * 1000 : 0);
		FD_ZERO(&rset);
		FD_SET((DWORD)handle, &rset);
		wset = rset;

		int r = select(handle + 1, &rset, &wset, NULL, &tv);
		if (r > 0 && (FD_ISSET(handle, &rset) || FD_ISSET(handle, &wset)))
		{
			socklen_t nErrLen = sizeof(int);
			int errorCode = 0;
			// If error occurs
			if (getsockopt(handle, SOL_SOCKET, SO_ERROR, (char*)&errorCode, &nErrLen) < 0 || errorCode)
				result = ACS_FAILED;
			else
				result = ACS_CONNECTED;
		}
		else if (r < 0)
		{
			if (iseSocketGetLastError() != SS_EINTR)
				result = ACS_FAILED;
		}

		if (result != ACS_CONNECTING)
			break;

		// Check timeout
		if (timeoutMSecs != -1)
		{
			ms += WAIT_STEP;
			if (ms >= timeoutMSecs)
				break;
		}
	}

	return result;
}

///////////////////////////////////////////////////////////////////////////////
// class TcpServer

TcpServer::TcpServer() :
	localPort_(0),
	listenerThread_(NULL)
{
	// nothing
}

//-----------------------------------------------------------------------------

TcpServer::~TcpServer()
{
	close();
}

//-----------------------------------------------------------------------------
// 描述: 创建连接对象
//-----------------------------------------------------------------------------
BaseTcpConnection* TcpServer::createConnection(SOCKET socketHandle, const InetAddress& peerAddr)
{
	BaseTcpConnection *result = NULL;

	if (onCreateConn_.proc)
		onCreateConn_.proc(onCreateConn_.param, this, socketHandle, peerAddr, result);

	if (result == NULL)
		result = new BaseTcpConnection(socketHandle, peerAddr);

	return result;
}

//-----------------------------------------------------------------------------
// 描述: 收到连接 (注: connection 是堆对象，需使用者释放)
//-----------------------------------------------------------------------------
void TcpServer::acceptConnection(BaseTcpConnection *connection)
{
	if (onAcceptConn_.proc)
		onAcceptConn_.proc(onAcceptConn_.param, this, connection);
	else
		delete connection;
}

//-----------------------------------------------------------------------------
// 描述: 启动监听线程
//-----------------------------------------------------------------------------
void TcpServer::startListenerThread()
{
	if (!listenerThread_)
	{
		listenerThread_ = new TcpListenerThread(this);
		listenerThread_->run();
	}
}

//-----------------------------------------------------------------------------
// 描述: 停止监听线程
//-----------------------------------------------------------------------------
void TcpServer::stopListenerThread()
{
	if (listenerThread_)
	{
		listenerThread_->terminate();
		listenerThread_->waitFor();
		delete listenerThread_;
		listenerThread_ = NULL;
	}
}

//-----------------------------------------------------------------------------
// 描述: 开启TCP服务器
//-----------------------------------------------------------------------------
void TcpServer::open()
{
	try
	{
		if (!isActive())
		{
			socket_.open();
			socket_.bind(localPort_);
			if (listen(socket_.getHandle(), LISTEN_QUEUE_SIZE) < 0)
				iseThrowSocketLastError();
			startListenerThread();
		}
	}
	catch (SocketException&)
	{
		close();
		throw;
	}
}

//-----------------------------------------------------------------------------
// 描述: 关闭TCP服务器
//-----------------------------------------------------------------------------
void TcpServer::close()
{
	if (isActive())
	{
		stopListenerThread();
		socket_.close();
	}
}

//-----------------------------------------------------------------------------
// 描述: 开启/关闭TCP服务器
//-----------------------------------------------------------------------------
void TcpServer::setActive(bool value)
{
	if (isActive() != value)
	{
		if (value) open();
		else close();
	}
}

//-----------------------------------------------------------------------------
// 描述: 设置TCP服务器监听端口
//-----------------------------------------------------------------------------
void TcpServer::setLocalPort(int value)
{
	if (value != localPort_)
	{
		if (isActive()) close();
		localPort_ = value;
	}
}

//-----------------------------------------------------------------------------
// 描述: 设置“创建新连接”的回调
//-----------------------------------------------------------------------------
void TcpServer::setOnCreateConnCallback(TCPSVR_ON_CREATE_CONN_PROC proc, void *param)
{
	onCreateConn_.proc = proc;
	onCreateConn_.param = param;
}

//-----------------------------------------------------------------------------
// 描述: 设置“收到连接”的回调
//-----------------------------------------------------------------------------
void TcpServer::setOnAcceptConnCallback(TCPSVR_ON_ACCEPT_CONN_PROC proc, void *param)
{
	onAcceptConn_.proc = proc;
	onAcceptConn_.param = param;
}

///////////////////////////////////////////////////////////////////////////////
// class UdpListenerThread

UdpListenerThread::UdpListenerThread(UdpListenerThreadPool *threadPool, int index) :
	threadPool_(threadPool),
	index_(index)
{
	setFreeOnTerminate(true);
	udpServer_ = &(threadPool->getUdpServer());
	threadPool_->registerThread(this);
}

//-----------------------------------------------------------------------------

UdpListenerThread::~UdpListenerThread()
{
	threadPool_->unregisterThread(this);
}

//-----------------------------------------------------------------------------
// 描述: UDP服务器监听工作
//-----------------------------------------------------------------------------
void UdpListenerThread::execute()
{
	const int MAX_UDP_BUFFER_SIZE = 8192;   // UDP数据包最大字节数
	const int SELECT_WAIT_MSEC    = 100;    // 每次等待时间 (毫秒)

	fd_set fds;
	struct timeval tv;
	SOCKET socketHandle = udpServer_->getHandle();
	Buffer packetBuffer(MAX_UDP_BUFFER_SIZE);
	InetAddress peerAddr;
	int r, n;

	while (!isTerminated() && udpServer_->isActive())
	try
	{
		// 设定每次等待时间
		tv.tv_sec = 0;
		tv.tv_usec = SELECT_WAIT_MSEC * 1000;

		FD_ZERO(&fds);
		FD_SET((UINT)socketHandle, &fds);

		r = select(socketHandle + 1, &fds, NULL, NULL, &tv);

		if (r > 0 && udpServer_->isActive() && FD_ISSET(socketHandle, &fds))
		{
			n = udpServer_->recvBuffer(packetBuffer.data(), MAX_UDP_BUFFER_SIZE, peerAddr);
			if (n > 0)
			{
				udpServer_->dataReceived(packetBuffer.data(), n, peerAddr);
			}
		}
		else if (r < 0)
		{
			int errorCode = iseSocketGetLastError();
			if (errorCode != SS_EINTR && errorCode != SS_EINPROGRESS)
				break;  // error
		}
	}
	catch (Exception&)
	{}
}

///////////////////////////////////////////////////////////////////////////////
// class UdpListenerThreadPool

UdpListenerThreadPool::UdpListenerThreadPool(UdpServer *udpServer) :
	udpServer_(udpServer),
	maxThreadCount_(0)
{
	// nothing
}

//-----------------------------------------------------------------------------

UdpListenerThreadPool::~UdpListenerThreadPool()
{
	// nothing
}

//-----------------------------------------------------------------------------
// 描述: 注册线程
//-----------------------------------------------------------------------------
void UdpListenerThreadPool::registerThread(UdpListenerThread *thread)
{
	threadList_.add(thread);
}

//-----------------------------------------------------------------------------
// 描述: 注销线程
//-----------------------------------------------------------------------------
void UdpListenerThreadPool::unregisterThread(UdpListenerThread *thread)
{
	threadList_.remove(thread);
}

//-----------------------------------------------------------------------------
// 描述: 创建并启动线程
//-----------------------------------------------------------------------------
void UdpListenerThreadPool::startThreads()
{
	for (int i = 0; i < maxThreadCount_; i++)
	{
		UdpListenerThread *thread;
		thread = new UdpListenerThread(this, i);
		thread->run();
	}
}

//-----------------------------------------------------------------------------
// 描述: 通知并等待所有线程退出
//-----------------------------------------------------------------------------
void UdpListenerThreadPool::stopThreads()
{
	const int MAX_WAIT_FOR_SECS = 5;
	threadList_.waitForAllThreads(MAX_WAIT_FOR_SECS);
}

///////////////////////////////////////////////////////////////////////////////
// class TcpListenerThread

TcpListenerThread::TcpListenerThread(TcpServer *tcpServer) :
	tcpServer_(tcpServer)
{
	setFreeOnTerminate(false);
}

//-----------------------------------------------------------------------------
// 描述: TCP服务器监听工作
//-----------------------------------------------------------------------------
void TcpListenerThread::execute()
{
	const int SELECT_WAIT_MSEC = 100;    // 每次等待时间 (毫秒)

	fd_set fds;
	struct timeval tv;
	SockAddr Addr;
	socklen_t nSockLen = sizeof(Addr);
	SOCKET socketHandle = tcpServer_->getSocket().getHandle();
	InetAddress peerAddr;
	SOCKET acceptHandle;
	int r;

	while (!isTerminated() && tcpServer_->isActive())
	try
	{
		// 设定每次等待时间
		tv.tv_sec = 0;
		tv.tv_usec = SELECT_WAIT_MSEC * 1000;

		FD_ZERO(&fds);
		FD_SET((UINT)socketHandle, &fds);

		r = select(socketHandle + 1, &fds, NULL, NULL, &tv);

		if (r > 0 && tcpServer_->isActive() && FD_ISSET(socketHandle, &fds))
		{
			acceptHandle = accept(socketHandle, (struct sockaddr*)&Addr, &nSockLen);
			if (acceptHandle != INVALID_SOCKET)
			{
				peerAddr = InetAddress(ntohl(Addr.sin_addr.s_addr), ntohs(Addr.sin_port));
				BaseTcpConnection *connection = tcpServer_->createConnection(acceptHandle, peerAddr);
				tcpServer_->acceptConnection(connection);
			}
		}
		else if (r < 0)
		{
			int errorCode = iseSocketGetLastError();
			if (errorCode != SS_EINTR && errorCode != SS_EINPROGRESS)
				break;  // error
		}
	}
	catch (Exception&)
	{}
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
