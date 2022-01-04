#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include <string>
#include <sstream>
#include <map>

#include "httplistenerrequest.h"

namespace System
{

namespace Net
{

namespace Http
{

class HttpListenerResponse
{
protected:
    std::string _contentType;
    std::map<std::string, std::string> _headers;
    int _statusCode;
    std::string _statusDescription;
    std::string _output;

protected:
    HttpListenerResponse();

public:
    HttpListenerResponse(HttpListenerResponse &copy);
    virtual ~HttpListenerResponse();

public:
    // Gets or sets the MIME type of the content returned.
    std::string const &ContentType() const;
    void SetContentType(std::string const &contentType);

    // Gets or sets the collection of header name/value pairs returned by the server.
    std::map<std::string, std::string> &Headers();

    // Gets or sets the HTTP status code to be returned to the client.
    int StatusCode() const;
    void SetStatusCode(int code);

    // Gets or sets a text description of the HTTP status code returned to the client.
    std::string const &StatusDescription() const;
    void SetStatusDescription(std::string const &statusDescription);

public:
    // Adds the specified header and value to the HTTP headers for this response.
    void AddHeader(std::string const &key, std::string const &value);

    // Configures the response to redirect the client to the specified URL.
    void Redirect(std::string const &url);

    void WriteOutput(std::string const &data);

    virtual void CloseOutput() = 0;
};

}

}

}

#endif // HTTPRESPONSE_H
