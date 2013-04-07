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
// 文件名称: ise_http.cpp
// 功能描述: HTTP客户端
///////////////////////////////////////////////////////////////////////////////

#include "ise_http.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// Misc Routines

string GetHttpProtoVerStr(HTTP_PROTO_VER nVersion)
{
	switch (nVersion)
	{
	case HPV_1_0:  return "1.0";
	case HPV_1_1:  return "1.1";
	default:       return "";
	}
}

//-----------------------------------------------------------------------------

string GetHttpMethodStr(HTTP_METHOD_TYPE nHttpMethod)
{
	switch (nHttpMethod)
	{
	case HMT_GET:   return "GET";
	case HMT_POST:  return "POST";
	default:        return "";
	}
}

///////////////////////////////////////////////////////////////////////////////
// class CHttpHeaderStrList

CHttpHeaderStrList::CHttpHeaderStrList()
{
	m_strNameValueSep = ":";
}

//-----------------------------------------------------------------------------

string CHttpHeaderStrList::MakeLine(const string& strName, const string& strValue) const
{
	return strName + m_strNameValueSep + " " + strValue;
}

//-----------------------------------------------------------------------------

int CHttpHeaderStrList::Add(const string& str)
{
	return m_Items.Add(str.c_str());
}

//-----------------------------------------------------------------------------

void CHttpHeaderStrList::Delete(int nIndex)
{
	m_Items.Delete(nIndex);
}

//-----------------------------------------------------------------------------

void CHttpHeaderStrList::Clear()
{
	m_Items.Clear();
}

//-----------------------------------------------------------------------------

void CHttpHeaderStrList::AddStrings(const CHttpHeaderStrList& Strings)
{
	for (int i = 0; i < Strings.GetCount(); i++)
		Add(Strings.GetString(i));
}

//-----------------------------------------------------------------------------

void CHttpHeaderStrList::MoveToTop(const CStrList& NameList)
{
	for (int i = NameList.GetCount() - 1; i >= 0; i--)
	{
		int nIndex = IndexOfName(NameList[i]);
		if (nIndex >= 0)
			m_Items.Move(nIndex, 0);
	}
}

//-----------------------------------------------------------------------------

int CHttpHeaderStrList::IndexOfName(const string& strName) const
{
	int nResult = -1;

	for (int i = 0; i < m_Items.GetCount(); i++)
		if (SameText(GetName(i), strName))
		{
			nResult = i;
			break;
		}

	return nResult;
}

//-----------------------------------------------------------------------------

string CHttpHeaderStrList::GetText() const
{
	string strResult;
	for (int i = 0; i < m_Items.GetCount(); i++)
	{
		string strName = GetName(i);
		string strValue = GetValue(i);
		if (!strName.empty() && !strValue.empty())
		{
			strResult += MakeLine(strName, strValue);
			strResult += "\r\n";
		}
	}
	return strResult;
}

//-----------------------------------------------------------------------------

string CHttpHeaderStrList::GetName(int nIndex) const
{
	string strResult;
	string strLine = m_Items[nIndex];
	string::size_type nPos = strLine.find(m_strNameValueSep);
	if (nPos != string::npos && nPos > 0)
		strResult = TrimString(strLine.substr(0, nPos));
	return strResult;
}

//-----------------------------------------------------------------------------

string CHttpHeaderStrList::GetValue(int nIndex) const
{
	string strResult;
	string strLine = m_Items[nIndex];
	string::size_type nPos = strLine.find(m_strNameValueSep);
	if (nPos != string::npos && nPos > 0)
		strResult = TrimString(strLine.substr(nPos + 1));
	return strResult;
}

//-----------------------------------------------------------------------------

string CHttpHeaderStrList::GetValue(const string& strName) const
{
	string strResult;
	int nIndex = IndexOfName(strName);
	if (nIndex >= 0)
		strResult = GetValue(nIndex);
	return strResult;
}

//-----------------------------------------------------------------------------

void CHttpHeaderStrList::SetValue(const string& strName, const string& strValue)
{
	string strNewName(TrimString(strName));
	string strNewValue(TrimString(strValue));

	int nIndex = IndexOfName(strNewName);
	if (strNewValue.empty())
	{
		if (nIndex >= 0) Delete(nIndex);
	}
	else
	{
		if (nIndex < 0)
			nIndex = Add("");
		m_Items.SetString(nIndex, MakeLine(strNewName, strNewValue).c_str());
	}
}

///////////////////////////////////////////////////////////////////////////////
// class CHttpEntityHeaderInfo

void CHttpEntityHeaderInfo::Init()
{
	m_RawHeaders.Clear();
	m_CustomHeaders.Clear();
	m_strCacheControl = "no-cache";
	m_strConnection = "close";
	m_strContentDisposition.clear();
	m_strContentEncoding.clear();
	m_strContentLanguage.clear();
	m_nContentLength = -1;
	m_nContentRangeStart = 0;
	m_nContentRangeEnd = 0;
	m_nContentRangeInstanceLength = 0;
	m_strContentType.clear();
	m_strContentVersion.clear();
	m_strDate.clear();
	m_strExpires.clear();
	m_strETag.clear();
	m_strLastModified.clear();
	m_strPragma = "no-cache";
	m_strTransferEncoding.clear();
}

//-----------------------------------------------------------------------------

void CHttpEntityHeaderInfo::Clear()
{
	Init();
}

//-----------------------------------------------------------------------------

void CHttpEntityHeaderInfo::ParseHeaders()
{
	m_strCacheControl = m_RawHeaders.GetValue("Cache-control");
	m_strConnection = m_RawHeaders.GetValue("Connection");
	m_strContentVersion = m_RawHeaders.GetValue("Content-Version");
	m_strContentDisposition = m_RawHeaders.GetValue("Content-Disposition");
	m_strContentEncoding = m_RawHeaders.GetValue("Content-Encoding");
	m_strContentLanguage = m_RawHeaders.GetValue("Content-Language");
	m_strContentType = m_RawHeaders.GetValue("Content-Type");
	m_nContentLength = StrToInt64(m_RawHeaders.GetValue("Content-Length"), -1);

	m_nContentRangeStart = 0;
	m_nContentRangeEnd = 0;
	m_nContentRangeInstanceLength = 0;

	/* Content-Range Examples: */
	// content-range: bytes 1-65536/102400
	// content-range: bytes */102400
	// content-range: bytes 1-65536/*

	string s = m_RawHeaders.GetValue("Content-Range");
	if (!s.empty())
	{
		FetchStr(s);
		string strRange = FetchStr(s, '/');
		string strLength = FetchStr(s);

		m_nContentRangeStart = StrToInt64(FetchStr(strRange, '-'), 0);
		m_nContentRangeEnd = StrToInt64(strRange, 0);
		m_nContentRangeInstanceLength = StrToInt64(strLength, 0);
	}

	m_strDate = m_RawHeaders.GetValue("Date");
	m_strLastModified = m_RawHeaders.GetValue("Last-Modified");
	m_strExpires = m_RawHeaders.GetValue("Expires");
	m_strETag = m_RawHeaders.GetValue("ETag");
	m_strPragma = m_RawHeaders.GetValue("Pragma");
	m_strTransferEncoding = m_RawHeaders.GetValue("Transfer-Encoding");
}

//-----------------------------------------------------------------------------

void CHttpEntityHeaderInfo::BuildHeaders()
{
	m_RawHeaders.Clear();

	if (!m_strConnection.empty())
		m_RawHeaders.SetValue("Connection", m_strConnection);
	if (!m_strContentVersion.empty())
		m_RawHeaders.SetValue("Content-Version", m_strContentVersion);
	if (!m_strContentDisposition.empty())
		m_RawHeaders.SetValue("Content-Disposition", m_strContentDisposition);
	if (!m_strContentEncoding.empty())
		m_RawHeaders.SetValue("Content-Encoding", m_strContentEncoding);
	if (!m_strContentLanguage.empty())
		m_RawHeaders.SetValue("Content-Language", m_strContentLanguage);
	if (!m_strContentType.empty())
		m_RawHeaders.SetValue("Content-Type", m_strContentType);
	if (m_nContentLength >= 0)
		m_RawHeaders.SetValue("Content-Length", IntToStr(m_nContentLength));
	if (!m_strCacheControl.empty())
		m_RawHeaders.SetValue("Cache-control", m_strCacheControl);
	if (!m_strDate.empty())
		m_RawHeaders.SetValue("Date", m_strDate);
	if (!m_strETag.empty())
		m_RawHeaders.SetValue("ETag", m_strETag);
	if (!m_strExpires.empty())
		m_RawHeaders.SetValue("Expires", m_strExpires);
	if (!m_strPragma.empty())
		m_RawHeaders.SetValue("Pragma", m_strPragma);
	if (!m_strTransferEncoding.empty())
		m_RawHeaders.SetValue("Transfer-Encoding", m_strTransferEncoding);

	if (m_CustomHeaders.GetCount() > 0)
		m_RawHeaders.AddStrings(m_CustomHeaders);
}

///////////////////////////////////////////////////////////////////////////////
// class CHttpRequestHeaderInfo

void CHttpRequestHeaderInfo::Init()
{
	m_strAccept = "text/html, */*";
	m_strAcceptCharSet.clear();
	m_strAcceptEncoding.clear();
	m_strAcceptLanguage.clear();
	m_strFrom.clear();
	m_strReferer.clear();
	m_strUserAgent = ISE_DEFAULT_USER_AGENT;
	m_strHost.clear();
	m_strRange.clear();
}

//-----------------------------------------------------------------------------

void CHttpRequestHeaderInfo::Clear()
{
	CHttpEntityHeaderInfo::Clear();
	Init();
}

//-----------------------------------------------------------------------------

void CHttpRequestHeaderInfo::ParseHeaders()
{
	CHttpEntityHeaderInfo::ParseHeaders();

	m_strAccept = m_RawHeaders.GetValue("Accept");
	m_strAcceptCharSet = m_RawHeaders.GetValue("Accept-Charset");
	m_strAcceptEncoding = m_RawHeaders.GetValue("Accept-Encoding");
	m_strAcceptLanguage = m_RawHeaders.GetValue("Accept-Language");
	m_strHost = m_RawHeaders.GetValue("Host");
	m_strFrom = m_RawHeaders.GetValue("From");
	m_strReferer = m_RawHeaders.GetValue("Referer");
	m_strUserAgent = m_RawHeaders.GetValue("User-Agent");

	// strip off the 'bytes=' portion of the header
	string s = m_RawHeaders.GetValue("Range");
	FetchStr(s, '=');
	m_strRange = s;
}

//-----------------------------------------------------------------------------

void CHttpRequestHeaderInfo::BuildHeaders()
{
	CHttpEntityHeaderInfo::BuildHeaders();

	if (!m_strHost.empty())
		m_RawHeaders.SetValue("Host", m_strHost);
	if (!m_strAccept.empty())
		m_RawHeaders.SetValue("Accept", m_strAccept);
	if (!m_strAcceptCharSet.empty())
		m_RawHeaders.SetValue("Accept-Charset", m_strAcceptCharSet);
	if (!m_strAcceptEncoding.empty())
		m_RawHeaders.SetValue("Accept-Encoding", m_strAcceptEncoding);
	if (!m_strAcceptLanguage.empty())
		m_RawHeaders.SetValue("Accept-Language", m_strAcceptLanguage);
	if (!m_strFrom.empty())
		m_RawHeaders.SetValue("From", m_strFrom);
	if (!m_strReferer.empty())
		m_RawHeaders.SetValue("Referer", m_strReferer);
	if (!m_strUserAgent.empty())
		m_RawHeaders.SetValue("User-Agent", m_strUserAgent);
	if (!m_strRange.empty())
		m_RawHeaders.SetValue("Range", "bytes=" + m_strRange);
	if (!m_strLastModified.empty())
		m_RawHeaders.SetValue("If-Modified-Since", m_strLastModified);

	// Sort the list
	CStrList NameList;
	NameList.Add("Host");
	NameList.Add("Accept");
	NameList.Add("Accept-Charset");
	NameList.Add("Accept-Encoding");
	NameList.Add("Accept-Language");
	NameList.Add("From");
	NameList.Add("Referer");
	NameList.Add("User-Agent");
	NameList.Add("Range");
	NameList.Add("Connection");
	m_RawHeaders.MoveToTop(NameList);
}

//-----------------------------------------------------------------------------

void CHttpRequestHeaderInfo::SetRange(INT64 nRangeStart, INT64 nRangeEnd)
{
	string strRange = FormatString("%I64d-", nRangeStart);
	if (nRangeEnd >= 0)
		strRange += FormatString("%I64d", nRangeEnd);
	SetRange(strRange);
}

///////////////////////////////////////////////////////////////////////////////
// class CHttpResponseHeaderInfo

void CHttpResponseHeaderInfo::Init()
{
	m_strAcceptRanges.clear();
	m_strLocation.clear();
	m_strServer.clear();

	m_strContentType = "text/html";
}

//-----------------------------------------------------------------------------

void CHttpResponseHeaderInfo::Clear()
{
	CHttpEntityHeaderInfo::Clear();
	Init();
}

//-----------------------------------------------------------------------------

void CHttpResponseHeaderInfo::ParseHeaders()
{
	CHttpEntityHeaderInfo::ParseHeaders();

	m_strAcceptRanges = m_RawHeaders.GetValue("Accept-Ranges");
	m_strLocation = m_RawHeaders.GetValue("Location");
	m_strServer = m_RawHeaders.GetValue("Server");
}

//-----------------------------------------------------------------------------

void CHttpResponseHeaderInfo::BuildHeaders()
{
	CHttpEntityHeaderInfo::BuildHeaders();

	if (HasContentRange() || HasContentRangeInstance())
	{
		string strCR = (HasContentRange() ?
			FormatString("%I64d-%I64d", m_nContentRangeStart, m_nContentRangeEnd) : "*");
		string strCI = (HasContentRangeInstance() ?
			IntToStr(m_nContentRangeInstanceLength) : "*");

		m_RawHeaders.SetValue("Content-Range", "bytes " + strCR + "/" + strCI);
	}

	if (!m_strAcceptRanges.empty())
		m_RawHeaders.SetValue("Accept-Ranges", m_strAcceptRanges);
}

///////////////////////////////////////////////////////////////////////////////
// class CHttpRequest

CHttpRequest::CHttpRequest(CCustomHttpClient& HttpClient) :
	m_HttpClient(HttpClient)
{
	Init();
}

//-----------------------------------------------------------------------------

void CHttpRequest::Init()
{
	m_nProtocolVersion = HPV_1_1;
	m_strUrl.clear();
	m_strMethod.clear();
	m_pContentStream = NULL;
}

//-----------------------------------------------------------------------------

void CHttpRequest::Clear()
{
	CHttpRequestHeaderInfo::Clear();
	Init();
}

///////////////////////////////////////////////////////////////////////////////
// class CHttpResponse

CHttpResponse::CHttpResponse(CCustomHttpClient& HttpClient) :
	m_HttpClient(HttpClient)
{
	Init();
}

//-----------------------------------------------------------------------------

void CHttpResponse::Init()
{
	m_strResponseText.clear();
	m_pContentStream = NULL;
}

//-----------------------------------------------------------------------------

void CHttpResponse::Clear()
{
	CHttpResponseHeaderInfo::Clear();
	Init();
}

//-----------------------------------------------------------------------------

bool CHttpResponse::GetKeepAlive()
{
	bool bResult = m_HttpClient.m_TcpClient.IsConnected();

	if (bResult)
	{
		string str = GetConnection();
		bResult = !str.empty() && !SameText(str, "close");
	}

	return bResult;
}

//-----------------------------------------------------------------------------

int CHttpResponse::GetResponseCode() const
{
	string s = m_strResponseText;
	FetchStr(s);
	s = TrimString(s);
	s = FetchStr(s);
	s = FetchStr(s, '.');

	return StrToInt(s, -1);
}

//-----------------------------------------------------------------------------

HTTP_PROTO_VER CHttpResponse::GetResponseVersion() const
{
	// eg: HTTP/1.1 200 OK
	string s = m_strResponseText.substr(5, 3);

	if (SameText(s, GetHttpProtoVerStr(HPV_1_0)))
		return HPV_1_0;
	else if (SameText(s, GetHttpProtoVerStr(HPV_1_1)))
		return HPV_1_1;
	else
		return HPV_1_0;
}

///////////////////////////////////////////////////////////////////////////////
// class CCustomHttpClient

CCustomHttpClient::CCustomHttpClient() :
	m_Request(*this),
	m_Response(*this),
	m_bHandleRedirects(true),
	m_nRedirectCount(0),
	m_bLastKeepAlive(false)
{
	EnsureNetworkInited();
}

CCustomHttpClient::~CCustomHttpClient()
{
	// nothing
}

//-----------------------------------------------------------------------------

CPeerAddress CCustomHttpClient::GetPeerAddrFromUrl(CUrl& Url)
{
	CPeerAddress PeerAddr(0, 0);

	if (!Url.GetHost().empty())
	{
		string strIp = LookupHostAddr(Url.GetHost());
		if (!strIp.empty())
		{
			PeerAddr.nIp = StringToIp(strIp);
			PeerAddr.nPort = StrToInt(Url.GetPort(), DEFAULT_HTTP_PORT);
		}
	}

	return PeerAddr;
}

//-----------------------------------------------------------------------------

void CCustomHttpClient::MakeRequestBuffer(CBuffer& Buffer)
{
	m_Request.BuildHeaders();

	string strText;
	strText = m_Request.GetMethod() + " " + m_Request.GetUrl() +
		" HTTP/" + GetHttpProtoVerStr(m_Request.GetProtocolVersion()) +
		"\r\n";

	for (int i = 0; i < m_Request.GetRawHeaders().GetCount(); i++)
	{
		string s = m_Request.GetRawHeaders()[i];
		if (!s.empty())
			strText = strText + s + "\r\n";
	}

	strText += "\r\n";

	Buffer.Assign(strText.c_str(), (int)strText.length());
}

//-----------------------------------------------------------------------------

int CCustomHttpClient::BeforeRequest(HTTP_METHOD_TYPE nHttpMethod, const string& strUrl,
	CStream *pRequestContent, CStream *pResponseContent,
	INT64 nReqStreamPos, INT64 nResStreamPos)
{
	ISE_ASSERT(nHttpMethod == HMT_GET || nHttpMethod == HMT_POST);

	if (nHttpMethod == HMT_POST)
	{
		m_TcpClient.Disconnect();

		// Currently when issuing a POST, SFC HTTP will automatically set the protocol
		// to version 1.0 independently of the value it had initially. This is because
		// there are some servers that don't respect the RFC to the full extent. In
		// particular, they don't respect sending/not sending the Expect: 100-Continue
		// header. Until we find an optimum solution that does NOT break the RFC, we
		// will restrict POSTS to version 1.0.
		m_Request.SetProtocolVersion(HPV_1_0);

		// Usual posting request have default ContentType is "application/x-www-form-urlencoded"
		if (m_Request.GetContentType().empty() ||
			SameText(m_Request.GetContentType(), "text/html"))
		{
			m_Request.SetContentType("application/x-www-form-urlencoded");
		}
	}

	CUrl Url(strUrl);
	if (m_nRedirectCount == 0)
	{
		if (Url.GetHost().empty())
			return EC_HTTP_URL_ERROR;
		m_Url = Url;
	}
	else
	{
		if (Url.GetHost().empty())
		{
			CUrl OldUrl = m_Url;
			m_Url = Url;
			m_Url.SetProtocol(OldUrl.GetProtocol());
			m_Url.SetHost(OldUrl.GetHost());
			m_Url.SetPort(OldUrl.GetPort());
		}
		else
			m_Url = Url;
	}

	if (pRequestContent)
		pRequestContent->SetPosition(nReqStreamPos);
	if (pResponseContent)
	{
		pResponseContent->SetSize(nResStreamPos);
		pResponseContent->SetPosition(nResStreamPos);
	}

	m_Request.SetMethod(GetHttpMethodStr(nHttpMethod));
	m_Request.SetUrl(m_Url.GetUrl(CUrl::URL_PATH | CUrl::URL_FILENAME | CUrl::URL_PARAMS));

	//if (m_Request.GetReferer().empty())
	//	m_Request.SetReferer(m_Url.GetUrl(CUrl::URL_ALL & ~(CUrl::URL_FILENAME | CUrl::URL_BOOKMARK | CUrl::URL_PARAMS)));

	int nPort = StrToInt(Url.GetPort(), DEFAULT_HTTP_PORT);
	m_Request.SetHost(nPort == DEFAULT_HTTP_PORT ?
		m_Url.GetHost() : m_Url.GetHost() + ":" + IntToStr(nPort));

	m_Request.SetContentLength(pRequestContent?
		pRequestContent->GetSize() - pRequestContent->GetPosition() : -1);
	m_Request.SetContentStream(pRequestContent);

	m_Response.Clear();
	m_Response.SetContentStream(pResponseContent);

	return EC_HTTP_SUCCESS;
}

//-----------------------------------------------------------------------------

void CCustomHttpClient::CheckResponseHeader(char *pBuffer, int nSize, bool& bFinished, bool& bError)
{
	bFinished = bError = false;

	if (nSize >= 4)
	{
		char *p = pBuffer + nSize - 4;
		if (p[0] == '\r' && p[1] == '\n' && p[2] == '\r' && p[3] == '\n')
			bFinished = true;

		if (!SameText(string(pBuffer, 4), "HTTP"))
			bError = true;
	}
}

//-----------------------------------------------------------------------------

bool CCustomHttpClient::ParseResponseHeader(void *pBuffer, int nSize)
{
	bool bResult = true;
	CMemoryStream Stream;
	Stream.Write(pBuffer, nSize);

	CStrList Lines;
	Lines.SetLineBreak("\r\n");
	Stream.SetPosition(0);
	Lines.LoadFromStream(Stream);

	if (Lines.GetCount() > 0)
	{
		string str = Lines[0];
		if (SameText(str.substr(0, 7), "HTTP/1."))
			m_Response.SetResponseText(str);
		else
			bResult = false;
	}
	else
		bResult = false;

	if (bResult)
	{
		m_Response.GetRawHeaders().Clear();
		for (int i = 1; i < Lines.GetCount(); i++)
		{
			if (!Lines[i].empty())
				m_Response.GetRawHeaders().Add(Lines[i]);
		}
		m_Response.ParseHeaders();
	}

	return bResult;
}

//-----------------------------------------------------------------------------

HTTP_NEXT_OP CCustomHttpClient::ProcessResponseHeader()
{
	HTTP_NEXT_OP nResult = HNO_EXIT;

	int nResponseCode = m_Response.GetResponseCode();
	int nResponseDigit = nResponseCode / 100;

	m_bLastKeepAlive = m_Response.GetKeepAlive();

	// Handle Redirects
	if ((nResponseDigit == 3 && nResponseCode != 304) ||
		(!m_Response.GetLocation().empty() && nResponseCode != 201))
	{
		m_nRedirectCount++;
		if (m_bHandleRedirects)
			nResult = HNO_REDIRECT;
	}
	else if (nResponseDigit == 2)
	{
		nResult = HNO_RECV_CONTENT;
	}

	return nResult;
}

//-----------------------------------------------------------------------------

void CCustomHttpClient::TcpDisconnect(bool bForce)
{
	if (!m_bLastKeepAlive || bForce)
		m_TcpClient.Disconnect();
}

///////////////////////////////////////////////////////////////////////////////
// class CHttpClient

CHttpClient::CHttpClient()
{
	// nothing
}

CHttpClient::~CHttpClient()
{
	// nothing
}

//-----------------------------------------------------------------------------

int CHttpClient::ReadLine(string& strLine, int nTimeOut)
{
	int nResult = EC_HTTP_SUCCESS;
	UINT nStartTicks = GetCurTicks();
	CMemoryStream Stream;
	char ch;

	while (true)
	{
		int nRemainTimeOut = Max(0, nTimeOut - (int)GetTickDiff(nStartTicks, GetCurTicks()));

		int r = m_TcpClient.RecvBuffer(&ch, sizeof(ch), true, nRemainTimeOut);
		if (r < 0)
		{
			nResult = EC_HTTP_SOCKET_ERROR;
			break;
		}
		else if (r > 0)
		{
			Stream.Write(&ch, sizeof(ch));

			char *pBuffer = Stream.GetMemory();
			int nSize = (int)Stream.GetSize();
			bool bFinished = false;

			if (nSize >= 2)
			{
				char *p = pBuffer + nSize - 2;
				if (p[0] == '\r' && p[1] == '\n')
					bFinished = true;
			}

			if (bFinished)
				break;
		}

		if (GetTickDiff(nStartTicks, GetCurTicks()) > (UINT)nTimeOut)
		{
			nResult = EC_HTTP_RECV_TIMEOUT;
			break;
		}
	}

	if (nResult == EC_HTTP_SUCCESS)
	{
		if (Stream.GetSize() > 0)
			strLine.assign(Stream.GetMemory(), (int)Stream.GetSize());
		else
			strLine.clear();
	}

	return nResult;
}

//-----------------------------------------------------------------------------

int CHttpClient::ReadChunkSize(UINT& nChunkSize, int nTimeOut)
{
	string strLine;
	int nResult = ReadLine(strLine, nTimeOut);
	if (nResult == EC_HTTP_SUCCESS)
	{
		string::size_type nPos = strLine.find(';');
		if (nPos != string::npos)
			strLine = strLine.substr(0, nPos);
		nChunkSize = (UINT)strtol(strLine.c_str(), NULL, 16);
	}

	return nResult;
}

//-----------------------------------------------------------------------------

int CHttpClient::ReadStream(CStream& Stream, int nBytes, int nTimeOut)
{
	const int BLOCK_SIZE = 1024*64;

	int nResult = EC_HTTP_SUCCESS;
	UINT nStartTicks = GetCurTicks();
	INT64 nRemainSize = nBytes;
	CBuffer Buffer(BLOCK_SIZE);

	while (nRemainSize > 0)
	{
		int nBlockSize = (int)Min(nRemainSize, (INT64)BLOCK_SIZE);
		int nRemainTimeOut = Max(0, nTimeOut - (int)GetTickDiff(nStartTicks, GetCurTicks()));
		int nRecvSize = m_TcpClient.RecvBuffer(Buffer.Data(), nBlockSize, true, nRemainTimeOut);

		if (nRecvSize < 0)
		{
			nResult = EC_HTTP_SOCKET_ERROR;
			break;
		}

		nRemainSize -= nRecvSize;
		Stream.WriteBuffer(Buffer.Data(), nRecvSize);

		if (GetTickDiff(nStartTicks, GetCurTicks()) > (UINT)nTimeOut)
		{
			nResult = EC_HTTP_RECV_TIMEOUT;
			break;
		}
	}

	return nResult;
}

//-----------------------------------------------------------------------------

int CHttpClient::ExecuteHttpAction(HTTP_METHOD_TYPE nHttpMethod, const string& strUrl,
	CStream *pRequestContent, CStream *pResponseContent)
{
	class CAutoFinalizer
	{
	private:
		CHttpClient& m_Owner;
	public:
		CAutoFinalizer(CHttpClient& Owner) : m_Owner(Owner) {}
		~CAutoFinalizer() { m_Owner.TcpDisconnect(); }
	} AutoFinalizer(*this);

	bool bNeedRecvContent = (pResponseContent != NULL);
	bool bCanRecvContent = false;

	int nResult = ExecuteHttpRequest(nHttpMethod, strUrl, pRequestContent,
		pResponseContent, bNeedRecvContent, bCanRecvContent);

	if (nResult == EC_HTTP_SUCCESS && bNeedRecvContent)
		nResult = RecvResponseContent();

	return nResult;
}

//-----------------------------------------------------------------------------

int CHttpClient::ExecuteHttpRequest(HTTP_METHOD_TYPE nHttpMethod, const string& strUrl,
	CStream *pRequestContent, CStream *pResponseContent, bool bNeedRecvContent,
	bool& bCanRecvContent)
{
	int nResult = EC_HTTP_SUCCESS;

	string strNewUrl(strUrl);
	INT64 nReqStreamPos = (pRequestContent? pRequestContent->GetPosition() : 0);
	INT64 nResStreamPos = (pResponseContent? pResponseContent->GetPosition() : 0);

	bCanRecvContent = false;
	m_nRedirectCount = 0;

	while (true)
	{
		nResult = BeforeRequest(nHttpMethod, strNewUrl, pRequestContent, pResponseContent,
			nReqStreamPos, nResStreamPos);

		if (nResult == EC_HTTP_SUCCESS) nResult = TcpConnect();
		if (nResult == EC_HTTP_SUCCESS) nResult = SendRequestHeader();
		if (nResult == EC_HTTP_SUCCESS) nResult = SendRequestContent();
		if (nResult == EC_HTTP_SUCCESS) nResult = RecvResponseHeader();

		if (nResult == EC_HTTP_SUCCESS)
		{
			HTTP_NEXT_OP nNextOp = ProcessResponseHeader();

			if (nNextOp == HNO_REDIRECT)
			{
				strNewUrl = m_Response.GetLocation();
			}
			else if (nNextOp == HNO_RECV_CONTENT)
			{
				bCanRecvContent = true;
				break;
			}
			else
				break;
		}
		else
			break;
	}

	if (nResult == EC_HTTP_SUCCESS && bNeedRecvContent && !bCanRecvContent)
		nResult = EC_HTTP_CANNOT_RECV_CONTENT;

	return nResult;
}

//-----------------------------------------------------------------------------

int CHttpClient::TcpConnect()
{
	if (!m_bLastKeepAlive)
		m_TcpClient.Disconnect();

	if (m_TcpClient.IsConnected())
	{
		CPeerAddress PeerAddr = GetPeerAddrFromUrl(m_Url);
		if (m_TcpClient.GetPeerAddr() != PeerAddr)
			m_TcpClient.Disconnect();
	}

	if (!m_TcpClient.IsConnected())
	{
		CPeerAddress PeerAddr = GetPeerAddrFromUrl(m_Url);
		int nState = m_TcpClient.AsyncConnect(IpToString(PeerAddr.nIp),
			PeerAddr.nPort, m_Options.nTcpConnectTimeOut);

		if (nState != ACS_CONNECTED)
			return EC_HTTP_SOCKET_ERROR;
	}

	if (m_TcpClient.IsConnected())
	{
		SOCKET hHandle = m_TcpClient.GetSocket().GetHandle();
		int nTimeOut = m_Options.nSocketOpTimeOut;
		::setsockopt(hHandle, SOL_SOCKET, SO_SNDTIMEO, (char*)&nTimeOut, sizeof(nTimeOut));
		::setsockopt(hHandle, SOL_SOCKET, SO_RCVTIMEO, (char*)&nTimeOut, sizeof(nTimeOut));
	}

	return EC_HTTP_SUCCESS;
}

//-----------------------------------------------------------------------------

int CHttpClient::SendRequestHeader()
{
	int nResult = EC_HTTP_SUCCESS;
	CBuffer Buffer;
	MakeRequestBuffer(Buffer);

	int r = m_TcpClient.SendBuffer(Buffer.Data(), Buffer.GetSize(), true, m_Options.nSendReqHeaderTimeOut);
	if (r != Buffer.GetSize())
		nResult = EC_HTTP_SOCKET_ERROR;

	return nResult;
}

//-----------------------------------------------------------------------------

int CHttpClient::SendRequestContent()
{
	int nResult = EC_HTTP_SUCCESS;

	CStream *pStream = m_Request.GetContentStream();
	if (!pStream) return nResult;

	const int BLOCK_SIZE = 1024*64;

	CBuffer Buffer(BLOCK_SIZE);

	while (true)
	{
		int nReadSize = pStream->Read(Buffer.Data(), BLOCK_SIZE);
		if (nReadSize > 0)
		{
			int r = m_TcpClient.SendBuffer(Buffer.Data(), nReadSize, true, m_Options.nSendReqContBlockTimeOut);
			if (r < 0)
			{
				nResult = EC_HTTP_SOCKET_ERROR;
				break;
			}
			else if (r != nReadSize)
			{
				nResult = EC_HTTP_SEND_TIMEOUT;
				break;
			}
		}

		if (nReadSize < BLOCK_SIZE) break;
	}

	return nResult;
}

//-----------------------------------------------------------------------------

int CHttpClient::RecvResponseHeader()
{
	const int RECV_TIMEOUT = m_Options.nRecvResHeaderTimeOut;

	int nResult = EC_HTTP_SUCCESS;
	UINT nStartTicks = GetCurTicks();

	do
	{
		CMemoryStream Stream;
		char ch;

		while (true)
		{
			int nRemainTimeOut = Max(0, RECV_TIMEOUT - (int)GetTickDiff(nStartTicks, GetCurTicks()));

			int r = m_TcpClient.RecvBuffer(&ch, sizeof(ch), true, nRemainTimeOut);
			if (r < 0)
			{
				nResult = EC_HTTP_SOCKET_ERROR;
				break;
			}
			else if (r > 0)
			{
				Stream.Write(&ch, sizeof(ch));

				bool bFinished, bError;
				CheckResponseHeader(Stream.GetMemory(), (int)Stream.GetSize(), bFinished, bError);
				if (bError)
				{
					nResult = EC_HTTP_RESPONSE_TEXT_ERROR;
					break;
				}
				if (bFinished)
					break;
			}

			if (GetTickDiff(nStartTicks, GetCurTicks()) > (UINT)RECV_TIMEOUT)
			{
				nResult = EC_HTTP_RECV_TIMEOUT;
				break;
			}
		}

		if (nResult == EC_HTTP_SUCCESS)
		{
			if (!ParseResponseHeader(Stream.GetMemory(), (int)Stream.GetSize()))
				nResult = EC_HTTP_RESPONSE_TEXT_ERROR;
		}
	}
	while (m_Response.GetResponseCode() == 100 && nResult == EC_HTTP_SUCCESS);

	return nResult;
}

//-----------------------------------------------------------------------------

int CHttpClient::RecvResponseContent()
{
	int nResult = EC_HTTP_SUCCESS;
	if (!m_Response.GetContentStream()) return nResult;

	if (LowerCase(m_Response.GetTransferEncoding()).find("chunked") != string::npos)
	{
		while (true)
		{
			int nTimeOut = m_Options.nRecvResContBlockTimeOut;
			UINT nChunkSize = 0;
			nResult = ReadChunkSize(nChunkSize, nTimeOut);
			if (nResult != EC_HTTP_SUCCESS)
				break;

			if (nChunkSize != 0)
			{
				nResult = ReadStream(*m_Response.GetContentStream(), nChunkSize, nTimeOut);
				if (nResult != EC_HTTP_SUCCESS)
					break;

				string strCrLf;
				nResult = ReadLine(strCrLf, nTimeOut);
				if (nResult != EC_HTTP_SUCCESS)
					break;
			}
			else
				break;
		}
	}
	else
	{
		INT64 nContentLength = m_Response.GetContentLength();
		if (nContentLength <= 0)
			nResult = EC_HTTP_CONTENT_LENGTH_ERROR;

		if (nResult == EC_HTTP_SUCCESS)
		{
			const int BLOCK_SIZE = 1024*64;

			INT64 nRemainSize = nContentLength;
			CBuffer Buffer(BLOCK_SIZE);

			while (nRemainSize > 0)
			{
				int nBlockSize = (int)Min(nRemainSize, (INT64)BLOCK_SIZE);
				int nRecvSize = m_TcpClient.RecvBuffer(Buffer.Data(), nBlockSize,
					true, m_Options.nRecvResContBlockTimeOut);

				if (nRecvSize < 0)
				{
					nResult = EC_HTTP_SOCKET_ERROR;
					break;
				}

				nRemainSize -= nRecvSize;
				m_Response.GetContentStream()->WriteBuffer(Buffer.Data(), nRecvSize);
			}
		}
	}

	return nResult;
}

//-----------------------------------------------------------------------------

int CHttpClient::Get(const string& strUrl, CStream *pResponseContent)
{
	return ExecuteHttpAction(HMT_GET, strUrl, NULL, pResponseContent);
}

//-----------------------------------------------------------------------------

int CHttpClient::Post(const string& strUrl, CStream *pRequestContent, CStream *pResponseContent)
{
	ISE_ASSERT(pRequestContent != NULL);
	return ExecuteHttpAction(HMT_POST, strUrl, pRequestContent, pResponseContent);
}

//-----------------------------------------------------------------------------

int CHttpClient::DownloadFile(const string& strUrl, const string& strLocalFileName)
{
	ForceDirectories(ExtractFilePath(strLocalFileName));
	CFileStream fs;
	if (!fs.Open(strLocalFileName, FM_CREATE | FM_SHARE_DENY_WRITE))
		return EC_HTTP_CANNOT_CREATE_FILE;

	int nResult = Get(strUrl, &fs);
	if (nResult != EC_HTTP_SUCCESS)
		DeleteFile(strLocalFileName);
	return nResult;
}

//-----------------------------------------------------------------------------

int CHttpClient::RequestFile(const string& strUrl)
{
	bool bCanRecvContent;
	int nResult = ExecuteHttpRequest(HMT_GET, strUrl, NULL, NULL, true, bCanRecvContent);
	if (nResult != EC_HTTP_SUCCESS || !bCanRecvContent)
		TcpDisconnect(true);
	return nResult;
}

//-----------------------------------------------------------------------------

int CHttpClient::ReceiveFile(void *pBuffer, int nSize, int nTimeOutMSecs)
{
	return m_TcpClient.RecvBuffer(pBuffer, nSize, true, nTimeOutMSecs);
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
