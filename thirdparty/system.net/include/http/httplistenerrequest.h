#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <winsock2.h>
#include <functional>
#include <string>
#include <map>

#define BUFFER_SIZE 1024*5 // 5KB

namespace System
{

namespace Net
{

namespace Http
{

class HttpListenerRequest
{
protected:
    long _contentLength64;
    std::string _contentType;
    std::map<std::string, std::string> _headers;
    std::string _httpMethod;
    std::map<std::string, std::string> _queryString;
    std::string _rawUrl;

public:
    HttpListenerRequest();

public:
    // Gets the length of the body data included in the request.
    long ContentLength64() const;

    // Gets the MIME type of the body data included in the request.
    std::string const &ContentType() const;

    // Gets the collection of header name/value pairs sent in the request.
    std::map<std::string, std::string> const &Headers() const;

    // Gets the HTTP method specified by the client.
    std::string const &HttpMethod() const;

    // Gets the query string included in the request.
    std::map<std::string, std::string> const &QueryString() const;

    // Gets the URL information (without the host and port) requested by the client.
    std::string const &RawUrl() const;

    std::string _payload;

    std::string ipAddress() const;
};

}

}

}

#endif // HTTPREQUEST_H
