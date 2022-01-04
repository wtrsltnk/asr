#include "http/httplistenercontext.h"

using namespace System::Net::Http;

HttpListenerContext::HttpListenerContext()
{ }

HttpListenerContext::~HttpListenerContext()
{ }

// Gets the HttpListenerRequest that represents a client's request for a resource.
HttpListenerRequest *HttpListenerContext::Request()
{
    return _request;
}

// Gets the HttpListenerResponse object that will be sent to the client in response to the client's request.
HttpListenerResponse *HttpListenerContext::Response()
{
    return _response;
}
