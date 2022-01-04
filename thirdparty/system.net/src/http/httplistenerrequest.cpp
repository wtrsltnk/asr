#include "http/httplistenerrequest.h"
#include "http/httplistenerresponse.h"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <fstream>
#include <cctype>

using namespace System::Net::Http;

HttpListenerRequest::HttpListenerRequest()
{ }

// Gets the length of the body data included in the request.
long HttpListenerRequest::ContentLength64() const
{
    return _contentLength64;
}

// Gets the MIME type of the body data included in the request.
std::string const &HttpListenerRequest::ContentType() const
{
    return _contentType;
}

// Gets the collection of header name/value pairs sent in the request.
std::map<std::string, std::string> const &HttpListenerRequest::Headers() const
{
    return _headers;
}

// Gets the HTTP method specified by the client.
std::string const &HttpListenerRequest::HttpMethod() const
{
    return _httpMethod;
}

// Gets the query string included in the request.
std::map<std::string, std::string> const &HttpListenerRequest::QueryString() const
{
    return _queryString;
}

// Gets the URL information (without the host and port) requested by the client.
std::string const &HttpListenerRequest::RawUrl() const
{
    return _rawUrl;
}
