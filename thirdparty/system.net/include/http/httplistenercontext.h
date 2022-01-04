#ifndef HTTPLISTENERCONTEXT_H
#define HTTPLISTENERCONTEXT_H

#include "httplistenerresponse.h"
#include "httplistenerrequest.h"

namespace System
{

namespace Net
{

namespace Http
{

class HttpListenerContext
{
protected:
    HttpListenerRequest *_request;
    HttpListenerResponse *_response;

public:
    HttpListenerContext();
    virtual ~HttpListenerContext();

    // Gets the HttpListenerRequest that represents a client's request for a resource.
    HttpListenerRequest *Request();

    // Gets the HttpListenerResponse object that will be sent to the client in response to the client's request.
    HttpListenerResponse *Response();

    // Gets an object used to obtain identity, authentication information, and security roles for the client whose request is represented by this HttpListenerContext object.
    // IPrincipal &User();
};

}

}

}

#endif // HTTPLISTENERCONTEXT_H
