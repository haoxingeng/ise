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

#include "ise/main/ise_options.h"
#include "ise/main/ise_classes.h"
#include "ise/main/ise_sys_utils.h"
#include "ise/main/ise_socket.h"
#include "ise/main/ise_exceptions.h"
#include "ise/main/ise_server_tcp.h"
#include "ise/main/ise_application.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// HTTP Protocol Example:
/*

Request:
~~~~~~~~
GET /images/logo.gif HTTP/1.1          <- request line
Host: www.google.com                   <- request header
...
<CR><LF>                               <- empty line
content stream

Response:
~~~~~~~~~
HTTP/1.1 200 OK                        <- status line
Content-Length: 3059                   <- response header
Content-Type: text/html
...
<CR><LF>                               <- empty line
content stream

*/

///////////////////////////////////////////////////////////////////////////////
// classes

class HttpHeaderStrList;
class HttpEntityHeaderInfo;
class HttpRequestHeaderInfo;
class HttpResponseHeaderInfo;
class HttpRequest;
class HttpResponse;
class CustomHttpClient;
class HttpClient;

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
// class HttpClientOptions

struct HttpClientOptions
{
public:
    int tcpConnectTimeout;                // TCP connect timeout (ms).
    int sendReqHeaderTimeout;             // The timeout of sending the request header.
    int sendReqContBlockTimeout;          // The timeout of sending the request content block.
    int recvResHeaderTimeout;             // The timeout of receiving the response header.
    int recvResContBlockTimeout;          // The timeout of receiving the response content block.
    int socketOpTimeout;                  // The timeout of the socket operation (recv/send).
public:
    HttpClientOptions()
    {
        tcpConnectTimeout = HTTP_TCP_CONNECT_TIMEOUT;
        sendReqHeaderTimeout = HTTP_SEND_REQ_HEADER_TIMEOUT;
        sendReqContBlockTimeout = HTTP_SEND_REQ_CONT_BLOCK_TIMEOUT;
        recvResHeaderTimeout = HTTP_RECV_RES_HEADER_TIMEOUT;
        recvResContBlockTimeout = HTTP_RECV_RES_CONT_BLOCK_TIMEOUT;
        socketOpTimeout = HTTP_SOCKET_OP_TIMEOUT;
    }
};

///////////////////////////////////////////////////////////////////////////////
// class HttpServerOptions

struct HttpServerOptions
{
public:
    int recvLineTimeout;                  // The timeout (ms) of receiving a text line.
    int recvContentTimeout;               // The timeout of receiving any data of the request content stream.
    int sendResponseHeaderTimeout;        // The timeout of sending the response header.
    int sendContentBlockTimeout;          // The timeout of sending a response content block.
    int maxConnectionCount;               // The maximum connections, -1 for no limitation.
public:
    HttpServerOptions()
    {
        recvLineTimeout = TIMEOUT_INFINITE;
        recvContentTimeout = TIMEOUT_INFINITE;
        sendResponseHeaderTimeout = TIMEOUT_INFINITE;
        sendContentBlockTimeout = TIMEOUT_INFINITE;
        maxConnectionCount = -1;
    }
};

///////////////////////////////////////////////////////////////////////////////
// class HttpHeaderStrList

class HttpHeaderStrList
{
public:
    HttpHeaderStrList();
    virtual ~HttpHeaderStrList() {}

    int add(const std::string& str);
    void del(int index);
    void clear();
    void addStrings(const HttpHeaderStrList& strings);
    void moveToTop(const StrList& nameList);
    int indexOfName(const std::string& name) const;

    int getCount() const { return items_.getCount(); }
    std::string getString(int index) const { return items_[index]; }
    std::string getText() const;
    std::string getName(int index) const;
    std::string getValue(int index) const;
    std::string getValue(const std::string& name) const;
    void setValue(const std::string& name, const std::string& value);

    std::string operator[] (int index) const { return getString(index); }

private:
    std::string makeLine(const std::string& name, const std::string& value) const;

private:
    StrList items_;
    std::string nameValueSep_;
};

///////////////////////////////////////////////////////////////////////////////
// class HttpEntityHeaderInfo

class HttpEntityHeaderInfo
{
public:
    HttpEntityHeaderInfo() { init(); }
    virtual ~HttpEntityHeaderInfo() {}

    virtual void clear();
    virtual void parseHeaders();
    virtual void buildHeaders();

    bool hasContentLength() { return contentLength_ >= 0; }
    bool hasContentRange() { return contentRangeEnd_ > 0; }
    bool hasContentRangeInstance() { return contentRangeInstanceLength_ > 0; }

    HttpHeaderStrList& getRawHeaders() { return rawHeaders_; }
    HttpHeaderStrList& getCustomHeaders() { return customHeaders_; }

    const std::string& getCacheControl() const { return cacheControl_; }
    const std::string& getConnection() const { return connection_; }
    const std::string& getContentDisposition() const { return contentDisposition_; }
    const std::string& getContentEncoding() const { return contentEncoding_; }
    const std::string& getContentLanguage() const { return contentLanguage_; }
    const INT64& getContentLength() const { return contentLength_; }
    const INT64& getContentRangeStart() const { return contentRangeStart_; }
    const INT64& getContentRangeEnd() const { return contentRangeEnd_; }
    const INT64& getContentRangeInstanceLength() const { return contentRangeInstanceLength_; }
    const std::string& getContentType() const { return contentType_; }
    const std::string& getContentVersion() const { return contentVersion_; }
    const std::string& getDate() const { return date_; }
    const std::string& getExpires() const { return expires_; }
    const std::string& getETag() const { return eTag_; }
    const std::string& getLastModified() const { return lastModified_; }
    const std::string& getPragma() const { return pragma_; }
    const std::string& getTransferEncoding() const { return transferEncoding_; }

    void setCustomHeaders(const HttpHeaderStrList& val) { customHeaders_ = val; }
    void setCacheControl(const std::string& value) { cacheControl_ = value; }
    void setConnection(const std::string& value) { connection_ = value; }
    void setConnection(bool keepAlive) { connection_ = (keepAlive? "keep-alive" : "close"); }
    void setContentDisposition(const std::string& value) { contentDisposition_ = value; }
    void setContentEncoding(const std::string& value) { contentEncoding_ = value; }
    void setContentLanguage(const std::string& value) { contentLanguage_ = value; }
    void setContentLength(INT64 value) { contentLength_ = value; }
    void setContentRangeStart(INT64 value) { contentRangeStart_ = value; }
    void setContentRangeEnd(INT64 value) { contentRangeEnd_ = value; }
    void setContentRangeInstanceLength(INT64 value) { contentRangeInstanceLength_ = value; }
    void setContentType(const std::string& value) { contentType_ = value; }
    void setContentVersion(const std::string& value) { contentVersion_ = value; }
    void setDate(const std::string& value) { date_ = value; }
    void setExpires(const std::string& value) { expires_ = value; }
    void setETag(const std::string& value) { eTag_ = value; }
    void setLastModified(const std::string& value) { lastModified_ = value; }
    void setPragma(const std::string& value) { pragma_ = value; }
    void setTransferEncoding(const std::string& value) { transferEncoding_ = value; }

protected:
    void init();

protected:
    HttpHeaderStrList rawHeaders_;
    HttpHeaderStrList customHeaders_;
    std::string cacheControl_;
    std::string connection_;
    std::string contentDisposition_;
    std::string contentEncoding_;
    std::string contentLanguage_;
    INT64 contentLength_;
    INT64 contentRangeStart_;
    INT64 contentRangeEnd_;
    INT64 contentRangeInstanceLength_;
    std::string contentType_;
    std::string contentVersion_;
    std::string date_;
    std::string expires_;
    std::string eTag_;
    std::string lastModified_;
    std::string pragma_;
    std::string transferEncoding_;
};

///////////////////////////////////////////////////////////////////////////////
// class HttpRequestHeaderInfo

class HttpRequestHeaderInfo : public HttpEntityHeaderInfo
{
public:
    HttpRequestHeaderInfo() { init(); }

    virtual void clear();
    virtual void parseHeaders();
    virtual void buildHeaders();

    const std::string& getAccept() const { return  accept_; }
    const std::string& getAcceptCharSet() const { return  acceptCharSet_; }
    const std::string& getAcceptEncoding() const { return  acceptEncoding_; }
    const std::string& getAcceptLanguage() const { return  acceptLanguage_; }
    const std::string& getFrom() const { return  from_; }
    const std::string& getReferer() const { return  referer_; }
    const std::string& getUserAgent() const { return  userAgent_; }
    const std::string& getHost() const { return  host_; }
    const std::string& getRange() const { return  range_; }

    void setAccept(const std::string& value) { accept_ = value; }
    void setAcceptCharSet(const std::string& value) { acceptCharSet_ = value; }
    void setAcceptEncoding(const std::string& value) { acceptEncoding_ = value; }
    void setAcceptLanguage(const std::string& value) { acceptLanguage_ = value; }
    void setFrom(const std::string& value) { from_ = value; }
    void setReferer(const std::string& value) { referer_ = value; }
    void setUserAgent(const std::string& value) { userAgent_ = value; }
    void setHost(const std::string& value) { host_ = value; }
    void setRange(const std::string& value) { range_ = value; }
    void setRange(INT64 rangeStart, INT64 rangeEnd = -1);

protected:
    void init();

protected:
    std::string accept_;
    std::string acceptCharSet_;
    std::string acceptEncoding_;
    std::string acceptLanguage_;
    std::string from_;
    std::string referer_;
    std::string userAgent_;
    std::string host_;
    std::string range_;
};

///////////////////////////////////////////////////////////////////////////////
// class HttpResponseHeaderInfo

class HttpResponseHeaderInfo : public HttpEntityHeaderInfo
{
public:
    HttpResponseHeaderInfo() { init(); }

    virtual void clear();
    virtual void parseHeaders();
    virtual void buildHeaders();

    const std::string& getAcceptRanges() const { return  acceptRanges_; }
    const std::string& getLocation() const { return  location_; }
    const std::string& getServer() const { return  server_; }

    void setAcceptRanges(const std::string& value) { acceptRanges_ = value; }
    void setLocation(const std::string& value) { location_ = value; }
    void setServer(const std::string& value) { server_ = value; }

protected:
    void init();

protected:
    std::string acceptRanges_;
    std::string location_;
    std::string server_;
};

///////////////////////////////////////////////////////////////////////////////
// class HttpRequest

class HttpRequest : public HttpRequestHeaderInfo
{
public:
    HttpRequest();

    virtual void clear();

    HTTP_PROTO_VER getProtocolVersion() const { return protocolVersion_; }
    const std::string& getUrl() const { return url_; }
    const std::string& getMethod() const { return method_; }
    Stream* getContentStream() const { return contentStream_; }

    void setProtocolVersion(HTTP_PROTO_VER value) { protocolVersion_ = value; }
    void setUrl(const std::string& value) { url_ = value; }
    void setMethod(const std::string& value) { method_ = value; }
    void setContentStream(Stream *value) { contentStream_ = value; }
    bool setRequestLine(const std::string& reqLine);

    void makeRequestHeaderBuffer(Buffer& buffer);

    static bool parseRequestLine(const std::string& reqLine, std::string& method,
        std::string& url, HTTP_PROTO_VER& protoVer);

protected:
    void init();

protected:
    HTTP_PROTO_VER protocolVersion_;
    std::string url_;
    std::string method_;
    Stream *contentStream_;
};

///////////////////////////////////////////////////////////////////////////////
// class HttpResponse

class HttpResponse : public HttpResponseHeaderInfo
{
public:
    HttpResponse();
    virtual ~HttpResponse();

    virtual void clear();

    bool getKeepAlive() const;
    const std::string& getStatusLine() const { return statusLine_; }
    int getStatusCode() const;
    HTTP_PROTO_VER getResponseVersion() const;
    Stream* getContentStream() const { return contentStream_; }

    void setStatusLine(const std::string& value) { statusLine_ = value; }
    void setStatusCode(int statusCode);
    void setContentStream(Stream *stream, bool ownsObject = false);

    void makeResponseHeaderBuffer(Buffer& buffer);

protected:
    void init();

protected:
    std::string statusLine_;
    Stream *contentStream_;
    bool ownsContentStream_;
};

///////////////////////////////////////////////////////////////////////////////
// class HttpTcpClient

class HttpTcpClient : public BaseTcpClient
{
public:
    class HttpTcpConnection : public BaseTcpConnection
    {
    public:
        using BaseTcpConnection::sendBuffer;
        using BaseTcpConnection::recvBuffer;
    };

public:
    HttpTcpConnection& getConnection() { return *static_cast<HttpTcpConnection*>(connection_); }
protected:
    virtual BaseTcpConnection* createConnection() { return new HttpTcpConnection(); }
};

///////////////////////////////////////////////////////////////////////////////
// class CustomHttpClient - HTTP client base class.

class CustomHttpClient : boost::noncopyable
{
public:
    friend class HttpRequest;
    friend class HttpResponse;
public:
    CustomHttpClient();
    virtual ~CustomHttpClient();

    /// Force to disconnect the connection.
    void disconnect() { tcpDisconnect(true); }
    /// Indicates whether the connection is currently connected or not.
    bool isConnected() { return tcpClient_.isConnected(); }

    /// The http request info.
    HttpRequest& httpRequest() { return request_; }
    /// The http response info.
    HttpResponse& httpResponse() { return response_; }
    /// The http client options
    HttpClientOptions& options() { return options_; }
    /// Returns the status line of the response.
    std::string getStatusLine() { return response_.getStatusLine(); }
    /// Returns the status code of the response.
    int getStatusCode() { return response_.getStatusCode(); }
    /// Indicates if the http client can handle redirections.
    bool getHandleRedirects() { return handleRedirects_; }
    /// Indicates the number of redirects encountered in the last request for the http client.
    int getRedirectCount() { return redirectCount_; }
    /// Returns the tcp client object.
    HttpTcpClient& getTcpClient() { return tcpClient_; }

    /// Determines if the http client can handle redirections.
    void setHandleRedirects(bool value) { handleRedirects_ = value; }

protected:
    InetAddress getInetAddrFromUrl(Url& url);
    int beforeRequest(HTTP_METHOD_TYPE httpMethod, const std::string& urlStr, Stream *requestContent,
        Stream *responseContent, INT64 reqStreamPos, INT64 resStreamPos);
    void checkResponseHeader(char *buffer, int size, bool& finished, bool& error);
    bool parseResponseHeader(void *buffer, int size);
    HTTP_NEXT_OP processResponseHeader();
    void tcpDisconnect(bool force = false);

protected:
    HttpTcpClient tcpClient_;
    HttpRequest request_;
    HttpResponse response_;
    HttpClientOptions options_;
    Url url_;
    bool handleRedirects_;
    int redirectCount_;
    bool lastKeepAlive_;
};

///////////////////////////////////////////////////////////////////////////////
// class HttpClient - HTTP client class.

class HttpClient : public CustomHttpClient
{
public:
    friend class AutoFinalizer;
public:
    HttpClient();
    virtual ~HttpClient();

    /// Sends a "GET" request to http server, and receives the response content. Returns the error code (EC_HTTP_XXX).
    int get(const std::string& url, Stream *responseContent);
    /// Sends a "POST" request to http server with the specified request content, and receives the response content. Returns the error code (EC_HTTP_XXX).
    int post(const std::string& url, Stream *requestContent, Stream *responseContent);

    /// Downloads the entire file from the specified url. Returns the error code (EC_HTTP_XXX).
    int downloadFile(const std::string& url, const std::string& localFileName);
    /// Sends the "GET" request to http server, and receives the response text and headers. Returns the error code (EC_HTTP_XXX).
    int requestFile(const std::string& url);
    /// Try to receive the response content from http server, returns the total number of bytes received actually, -1 if error.
    int receiveFile(void *buffer, int size, int timeoutMSecs = -1);

protected:
    int executeHttpAction(HTTP_METHOD_TYPE httpMethod, const std::string& url,
        Stream *requestContent, Stream *responseContent);
    int executeHttpRequest(HTTP_METHOD_TYPE httpMethod, const std::string& url,
        Stream *requestContent, Stream *responseContent,
        bool needRecvContent, bool& canRecvContent);

    int tcpConnect();
    int sendRequestHeader();
    int sendRequestContent();
    int recvResponseHeader();
    int recvResponseContent();

private:
    int readLine(std::string& line, int timeout);
    int readChunkSize(UINT& chunkSize, int timeout);
    int readStream(Stream& stream, int bytes, int timeout);
};

///////////////////////////////////////////////////////////////////////////////
// class HttpServer - HTTP server class.

class HttpServer :
    boost::noncopyable,
    public TcpCallbacks
{
public:
    typedef boost::function<void (
        const HttpRequest& request,
        HttpResponse& response
        )> HttpSessionCallback;

public:
    HttpServer();
    virtual ~HttpServer();

    void setHttpSessionCallback(const HttpSessionCallback& callback) { onHttpSession_ = callback; }
    HttpServerOptions& options() { return options_; }
    int getConnCount() { return static_cast<int>(connCount_.get()); }

public:  /* interface TcpCallbacks */
    virtual void onTcpConnected(const TcpConnectionPtr& connection);
    virtual void onTcpDisconnected(const TcpConnectionPtr& connection);
    virtual void onTcpRecvComplete(const TcpConnectionPtr& connection, void *packetBuffer,
        int packetSize, const Context& context);
    virtual void onTcpSendComplete(const TcpConnectionPtr& connection, const Context& context);

private:
    enum RecvReqState
    {
        RRS_RECVING_REQ_LINE,
        RRS_RECVING_REQ_HEADERS,
        RRS_RECVING_CONTENT,
        RRS_COMPLETE,
    };

    enum SendResState
    {
        SRS_SENDING_RES_HEADERS,
        SRS_SENDING_CONTENT,
        SRS_COMPLETE,
    };

    struct ConnContext
    {
    public:
        RecvReqState recvReqState;
        SendResState sendResState;
        HttpRequest httpRequest;
        MemoryStream reqContentStream;
        HttpResponse httpResponse;
        MemoryStream resContentStream;
    public:
        ConnContext()
        {
            recvReqState = static_cast<RecvReqState>(0); 
            sendResState = static_cast<SendResState>(0); 
            httpRequest.setContentStream(&reqContentStream);
            httpResponse.setContentStream(&resContentStream, false);
        }
    };

    typedef boost::shared_ptr<ConnContext> ConnContextPtr;

private:
    HttpServerOptions options_;
    AtomicInt connCount_;
    HttpSessionCallback onHttpSession_;
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_HTTP_H_
