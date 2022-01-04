#include "http/httplistenerresponse.h"

using namespace System::Net::Http;

HttpListenerResponse::HttpListenerResponse()
    : _statusCode(200), _statusDescription("OK")
{ }

HttpListenerResponse::~HttpListenerResponse() { }

// Gets or sets the MIME type of the content returned.
std::string const &HttpListenerResponse::ContentType() const
{
    return _contentType;
}

void HttpListenerResponse::SetContentType(std::string const &contentType)
{
    _contentType = contentType;
}

// Gets or sets the collection of header name/value pairs returned by the server.
std::map<std::string, std::string> &HttpListenerResponse::Headers()
{
    return _headers;
}

// Gets or sets the HTTP status code to be returned to the client.
int HttpListenerResponse::StatusCode() const
{
    return _statusCode;
}
void HttpListenerResponse::SetStatusCode(int code)
{
    _statusCode = code;
}

// Gets or sets a text description of the HTTP status code returned to the client.
std::string const &HttpListenerResponse::StatusDescription() const
{
    return _statusDescription;
}

void HttpListenerResponse::SetStatusDescription(std::string const &statusDescription)
{
    _statusDescription = statusDescription;
}

// Adds the specified header and value to the HTTP headers for this response.
void HttpListenerResponse::AddHeader(std::string const &key, std::string const &value)
{
    this->_headers.insert(std::make_pair(key, value));
}

// Configures the response to redirect the client to the specified URL.
void HttpListenerResponse::Redirect(std::string const &url)
{
    SetStatusCode(302);
    SetStatusDescription("Found");
    AddHeader("Location", url);

    CloseOutput();
}

void HttpListenerResponse::WriteOutput(std::string const &data)
{
    _output += data;
}
