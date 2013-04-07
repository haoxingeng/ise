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
// ise_http.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_HTTP_H_
#define _ISE_HTTP_H_

#include "ise_options.h"
#include "ise_classes.h"
#include "ise_sysutils.h"
#include "ise_socket.h"
#include "ise_exceptions.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// ÌáÇ°ÉùÃ÷

class CUrl;
class CHttpHeaderStrList;
class CHttpEntityHeaderInfo;
class CHttpRequestHeaderInfo;
class CHttpResponseHeaderInfo;
class CHttpRequest;
class CHttpResponse;
class CCustomHttpClient;
class CHttpClient;

///////////////////////////////////////////////////////////////////////////////
// Type Definitions

// HTTP version
enum HTTP_PROTO_VER
{
	HPV_1_0,
	HPV_1_1,
};

// HTTP methods
enum HTTP_METHOD_TYPE
{
	HMT_GET,
	HMT_POST,
};

// The return value of ProcessResponseHeader()
enum HTTP_NEXT_OP
{
	HNO_REDIRECT,
	HNO_RECV_CONTENT,
	HNO_EXIT,
};

///////////////////////////////////////////////////////////////////////////////
// Constant Definitions

// Default user-agent string
const char* const ISE_DEFAULT_USER_AGENT = "Mozilla/4.0 (compatible; ISE)";
// The default http port
const int DEFAULT_HTTP_PORT = 80;

// Default Timeout defines:
const int HTTP_TCP_CONNECT_TIMEOUT         = 1000*30;     // TCP connect timeout (ms).
const int HTTP_SEND_REQ_HEADER_TIMEOUT     = 1000*30;     // Send request header timeout.
const int HTTP_SEND_REQ_CONT_BLOCK_TIMEOUT = 1000*60*2;   // Send request content block timeout.
const int HTTP_RECV_RES_HEADER_TIMEOUT     = 1000*30;     // Receive response header timeout.
const int HTTP_RECV_RES_CONT_BLOCK_TIMEOUT = 1000*60*2;   // Receive response content block timeout.
const int HTTP_SOCKET_OP_TIMEOUT           = 1000*60*10;  // Socket operation (recv/send) timeout.

// Error Codes:
const int EC_HTTP_SUCCESS                  =  0;
const int EC_HTTP_UNKNOWN_ERROR            = -1;
const int EC_HTTP_SOCKET_ERROR             = -2;
const int EC_HTTP_URL_ERROR                = -3;
const int EC_HTTP_CONTENT_LENGTH_ERROR     = -4;
const int EC_HTTP_SEND_TIMEOUT             = -5;
const int EC_HTTP_RECV_TIMEOUT             = -6;
const int EC_HTTP_CANNOT_CREATE_FILE       = -7;
const int EC_HTTP_RESPONSE_TEXT_ERROR      = -8;
const int EC_HTTP_CANNOT_RECV_CONTENT      = -9;
const int EC_HTTP_IOCP_ERROR               = -10;

///////////////////////////////////////////////////////////////////////////////
// class CHttpClientOptions

struct CHttpClientOptions
{
public:
	int nTcpConnectTimeOut;                // TCP connect timeout (ms).
	int nSendReqHeaderTimeOut;             // Send request header timeout.
	int nSendReqContBlockTimeOut;          // Send request content block timeout.
	int nRecvResHeaderTimeOut;             // Receive response header timeout.
	int nRecvResContBlockTimeOut;          // Receive response content block timeout.
	int nSocketOpTimeOut;                  // Socket operation (recv/send) timeout.
public:
	CHttpClientOptions()
	{
		nTcpConnectTimeOut = HTTP_TCP_CONNECT_TIMEOUT;
		nSendReqHeaderTimeOut = HTTP_SEND_REQ_HEADER_TIMEOUT;
		nSendReqContBlockTimeOut = HTTP_SEND_REQ_CONT_BLOCK_TIMEOUT;
		nRecvResHeaderTimeOut = HTTP_RECV_RES_HEADER_TIMEOUT;
		nRecvResContBlockTimeOut = HTTP_RECV_RES_CONT_BLOCK_TIMEOUT;
		nSocketOpTimeOut = HTTP_SOCKET_OP_TIMEOUT;
	}
};

///////////////////////////////////////////////////////////////////////////////
// class CHttpHeaderStrList

class CHttpHeaderStrList
{
private:
	CStrList m_Items;
	string m_strNameValueSep;
private:
	string MakeLine(const string& strName, const string& strValue) const;
public:
	CHttpHeaderStrList();
	virtual ~CHttpHeaderStrList() {}

	int Add(const string& str);
	void Delete(int nIndex);
	void Clear();
	void AddStrings(const CHttpHeaderStrList& Strings);
	void MoveToTop(const CStrList& NameList);
	int IndexOfName(const string& strName) const;

	int GetCount() const { return m_Items.GetCount(); }
	string GetString(int nIndex) const { return m_Items[nIndex]; }
	string GetText() const;
	string GetName(int nIndex) const;
	string GetValue(int nIndex) const;
	string GetValue(const string& strName) const;
	void SetValue(const string& strName, const string& strValue);

	string operator[] (int nIndex) const { return GetString(nIndex); }
};

///////////////////////////////////////////////////////////////////////////////
// class CHttpEntityHeaderInfo

class CHttpEntityHeaderInfo
{
protected:
	CHttpHeaderStrList m_RawHeaders;
	CHttpHeaderStrList m_CustomHeaders;
	string m_strCacheControl;
	string m_strConnection;
	string m_strContentDisposition;
	string m_strContentEncoding;
	string m_strContentLanguage;
	INT64 m_nContentLength;
	INT64 m_nContentRangeStart;
	INT64 m_nContentRangeEnd;
	INT64 m_nContentRangeInstanceLength;
	string m_strContentType;
	string m_strContentVersion;
	string m_strDate;
	string m_strExpires;
	string m_strETag;
	string m_strLastModified;
	string m_strPragma;
	string m_strTransferEncoding;
protected:
	void Init();
public:
	CHttpEntityHeaderInfo() { Init(); }
	virtual ~CHttpEntityHeaderInfo() {}

	virtual void Clear();
	virtual void ParseHeaders();
	virtual void BuildHeaders();

	bool HasContentLength() { return m_nContentLength >= 0; }
	bool HasContentRange() { return m_nContentRangeEnd > 0; }
	bool HasContentRangeInstance() { return m_nContentRangeInstanceLength > 0; }

	CHttpHeaderStrList& GetRawHeaders() { return m_RawHeaders; }
	CHttpHeaderStrList& GetCustomHeaders() { return m_CustomHeaders; }

	const string& GetCacheControl() const { return m_strCacheControl; }
	const string& GetConnection() const { return m_strConnection; }
	const string& GetContentDisposition() const { return m_strContentDisposition; }
	const string& GetContentEncoding() const { return m_strContentEncoding; }
	const string& GetContentLanguage() const { return m_strContentLanguage; }
	const INT64& GetContentLength() const { return m_nContentLength; }
	const INT64& GetContentRangeStart() const { return m_nContentRangeStart; }
	const INT64& GetContentRangeEnd() const { return m_nContentRangeEnd; }
	const INT64& GetContentRangeInstanceLength() const { return m_nContentRangeInstanceLength; }
	const string& GetContentType() const { return m_strContentType; }
	const string& GetContentVersion() const { return m_strContentVersion; }
	const string& GetDate() const { return m_strDate; }
	const string& GetExpires() const { return m_strExpires; }
	const string& GetETag() const { return m_strETag; }
	const string& GetLastModified() const { return m_strLastModified; }
	const string& GetPragma() const { return m_strPragma; }
	const string& GetTransferEncoding() const { return m_strTransferEncoding; }

	void SetCustomHeaders(const CHttpHeaderStrList& val) { m_CustomHeaders = val; }
	void SetCacheControl(const string& strValue) { m_strCacheControl = strValue; }
	void SetConnection(const string& strValue) { m_strConnection = strValue; }
	void SetConnection(bool bKeepAlive) { m_strConnection = (bKeepAlive? "keep-alive" : "close"); }
	void SetContentDisposition(const string& strValue) { m_strContentDisposition = strValue; }
	void SetContentEncoding(const string& strValue) { m_strContentEncoding = strValue; }
	void SetContentLanguage(const string& strValue) { m_strContentLanguage = strValue; }
	void SetContentLength(INT64 nValue) { m_nContentLength = nValue; }
	void SetContentRangeStart(INT64 nValue) { m_nContentRangeStart = nValue; }
	void SetContentRangeEnd(INT64 nValue) { m_nContentRangeEnd = nValue; }
	void SetContentRangeInstanceLength(INT64 nValue) { m_nContentRangeInstanceLength = nValue; }
	void SetContentType(const string& strValue) { m_strContentType = strValue; }
	void SetContentVersion(const string& strValue) { m_strContentVersion = strValue; }
	void SetDate(const string& strValue) { m_strDate = strValue; }
	void SetExpires(const string& strValue) { m_strExpires = strValue; }
	void SetETag(const string& strValue) { m_strETag = strValue; }
	void SetLastModified(const string& strValue) { m_strLastModified = strValue; }
	void SetPragma(const string& strValue) { m_strPragma = strValue; }
	void SetTransferEncoding(const string& strValue) { m_strTransferEncoding = strValue; }
};

///////////////////////////////////////////////////////////////////////////////
// class CHttpRequestHeaderInfo

class CHttpRequestHeaderInfo : public CHttpEntityHeaderInfo
{
protected:
	string m_strAccept;
	string m_strAcceptCharSet;
	string m_strAcceptEncoding;
	string m_strAcceptLanguage;
	string m_strFrom;
	string m_strReferer;
	string m_strUserAgent;
	string m_strHost;
	string m_strRange;
protected:
	void Init();
public:
	CHttpRequestHeaderInfo() { Init(); }

	virtual void Clear();
	virtual void ParseHeaders();
	virtual void BuildHeaders();

	const string& GetAccept() const { return  m_strAccept; }
	const string& GetAcceptCharSet() const { return  m_strAcceptCharSet; }
	const string& GetAcceptEncoding() const { return  m_strAcceptEncoding; }
	const string& GetAcceptLanguage() const { return  m_strAcceptLanguage; }
	const string& GetFrom() const { return  m_strFrom; }
	const string& GetReferer() const { return  m_strReferer; }
	const string& GetUserAgent() const { return  m_strUserAgent; }
	const string& GetHost() const { return  m_strHost; }
	const string& GetRange() const { return  m_strRange; }

	void SetAccept(const string& strValue) { m_strAccept = strValue; }
	void SetAcceptCharSet(const string& strValue) { m_strAcceptCharSet = strValue; }
	void SetAcceptEncoding(const string& strValue) { m_strAcceptEncoding = strValue; }
	void SetAcceptLanguage(const string& strValue) { m_strAcceptLanguage = strValue; }
	void SetFrom(const string& strValue) { m_strFrom = strValue; }
	void SetReferer(const string& strValue) { m_strReferer = strValue; }
	void SetUserAgent(const string& strValue) { m_strUserAgent = strValue; }
	void SetHost(const string& strValue) { m_strHost = strValue; }
	void SetRange(const string& strValue) { m_strRange = strValue; }
	void SetRange(INT64 nRangeStart, INT64 nRangeEnd = -1);
};

///////////////////////////////////////////////////////////////////////////////
// class CHttpResponseHeaderInfo

class CHttpResponseHeaderInfo : public CHttpEntityHeaderInfo
{
protected:
	string m_strAcceptRanges;
	string m_strLocation;
	string m_strServer;
protected:
	void Init();
public:
	CHttpResponseHeaderInfo() { Init(); }

	virtual void Clear();
	virtual void ParseHeaders();
	virtual void BuildHeaders();

	const string& GetAcceptRanges() const { return  m_strAcceptRanges; }
	const string& GetLocation() const { return  m_strLocation; }
	const string& GetServer() const { return  m_strServer; }

	void SetAcceptRanges(const string& strValue) { m_strAcceptRanges = strValue; }
	void SetLocation(const string& strValue) { m_strLocation = strValue; }
	void SetServer(const string& strValue) { m_strServer = strValue; }
};

///////////////////////////////////////////////////////////////////////////////
// class CHttpRequest

class CHttpRequest : public CHttpRequestHeaderInfo
{
protected:
	CCustomHttpClient& m_HttpClient;
	HTTP_PROTO_VER m_nProtocolVersion;
	string m_strUrl;
	string m_strMethod;
	CStream *m_pContentStream;
protected:
	void Init();
public:
	CHttpRequest(CCustomHttpClient& HttpClient);

	virtual void Clear();

	HTTP_PROTO_VER GetProtocolVersion() const { return m_nProtocolVersion; }
	const string& GetUrl() const { return m_strUrl; }
	const string& GetMethod() const { return m_strMethod; }
	CStream* GetContentStream() const { return m_pContentStream; }

	void SetProtocolVersion(HTTP_PROTO_VER nValue) { m_nProtocolVersion = nValue; }
	void SetUrl(const string& strValue) { m_strUrl = strValue; }
	void SetMethod(const string& strValue) { m_strMethod = strValue; }
	void SetContentStream(CStream *pValue) { m_pContentStream = pValue; }
};

///////////////////////////////////////////////////////////////////////////////
// class CHttpResponse

class CHttpResponse : public CHttpResponseHeaderInfo
{
protected:
	CCustomHttpClient& m_HttpClient;
	string m_strResponseText;
	CStream *m_pContentStream;
protected:
	void Init();
public:
	CHttpResponse(CCustomHttpClient& HttpClient);

	virtual void Clear();

	bool GetKeepAlive();
	const string& GetResponseText() const { return m_strResponseText; }
	int GetResponseCode() const;
	HTTP_PROTO_VER GetResponseVersion() const;
	CStream* GetContentStream() const { return m_pContentStream; }

	void SetResponseText(const string& strValue) { m_strResponseText = strValue; }
	void SetContentStream(CStream *pValue) { m_pContentStream = pValue; }
};

///////////////////////////////////////////////////////////////////////////////
// class CCustomHttpClient - HTTP client base class.

class CCustomHttpClient
{
public:
	friend class CHttpRequest;
	friend class CHttpResponse;
protected:
	CTcpClient m_TcpClient;
	CHttpRequest m_Request;
	CHttpResponse m_Response;
	CHttpClientOptions m_Options;
	CUrl m_Url;
	bool m_bHandleRedirects;
	int m_nRedirectCount;
	bool m_bLastKeepAlive;
protected:
	CPeerAddress GetPeerAddrFromUrl(CUrl& Url);
	void MakeRequestBuffer(CBuffer& Buffer);
	int BeforeRequest(HTTP_METHOD_TYPE nHttpMethod, const string& strUrl, CStream *pRequestContent,
		CStream *pResponseContent, INT64 nReqStreamPos, INT64 nResStreamPos);
	void CheckResponseHeader(char *pBuffer, int nSize, bool& bFinished, bool& bError);
	bool ParseResponseHeader(void *pBuffer, int nSize);
	HTTP_NEXT_OP ProcessResponseHeader();
	void TcpDisconnect(bool bForce = false);
public:
	CCustomHttpClient();
	virtual ~CCustomHttpClient();

	/// Force to disconnect the connection.
	void Disconnect() { TcpDisconnect(true); }
	/// Indicates whether the connection is currently connected or not.
	bool IsConnected() { return m_TcpClient.IsConnected(); }

	/// The http request info.
	CHttpRequest& HttpRequest() { return m_Request; }
	/// The http response info.
	CHttpResponse& HttpResponse() { return m_Response; }
	/// The http client options
	CHttpClientOptions& Options() { return m_Options; }
	/// Returns the response text.
	string GetResponseText() { return m_Response.GetResponseText(); }
	/// Returns the response code.
	int GetResponseCode() { return m_Response.GetResponseCode(); }
	/// Indicates if the http client can handle redirections.
	bool GetHandleRedirects() { return m_bHandleRedirects; }
	/// Indicates the number of redirects encountered in the last request for the http client.
	int GetRedirectCount() { return m_nRedirectCount; }
	/// Returns the tcp client object.
	CTcpClient& GetTcpClient() { return m_TcpClient; }

	/// Determines if the http client can handle redirections.
	void SetHandleRedirects(bool bValue) { m_bHandleRedirects = bValue; }
};

///////////////////////////////////////////////////////////////////////////////
// class CHttpClient - HTTP client class.

class CHttpClient : public CCustomHttpClient
{
public:
	friend class CAutoFinalizer;
private:
	int ReadLine(string& strLine, int nTimeOut);
	int ReadChunkSize(UINT& nChunkSize, int nTimeOut);
	int ReadStream(CStream& Stream, int nBytes, int nTimeOut);
protected:
	int ExecuteHttpAction(HTTP_METHOD_TYPE nHttpMethod, const string& strUrl,
		CStream *pRequestContent, CStream *pResponseContent);
	int ExecuteHttpRequest(HTTP_METHOD_TYPE nHttpMethod, const string& strUrl,
		CStream *pRequestContent, CStream *pResponseContent,
		bool bNeedRecvContent, bool& bCanRecvContent);

	int TcpConnect();
	int SendRequestHeader();
	int SendRequestContent();
	int RecvResponseHeader();
	int RecvResponseContent();
public:
	CHttpClient();
	virtual ~CHttpClient();

	/// Sends a "GET" request to http server, and receives the response content. Returns the error code (EC_HTTP_XXX).
	int Get(const string& strUrl, CStream *pResponseContent);
	/// Sends a "POST" request to http server with the specified request content, and receives the response content. Returns the error code (EC_HTTP_XXX).
	int Post(const string& strUrl, CStream *pRequestContent, CStream *pResponseContent);

	/// Downloads the entire file from the specified url. Returns the error code (EC_HTTP_XXX).
	int DownloadFile(const string& strUrl, const string& strLocalFileName);
	/// Sends the "GET" request to http server, and receives the response text and headers. Returns the error code (EC_HTTP_XXX).
	int RequestFile(const string& strUrl);
	/// Try to receive the response content from http server, returns the total number of bytes received actually, -1 if error.
	int ReceiveFile(void *pBuffer, int nSize, int nTimeOutMSecs = -1);
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_HTTP_H_
