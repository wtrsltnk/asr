#ifndef HTTPLISTENER_H
#define HTTPLISTENER_H

#include "httplistenercontext.h"
#include <vector>
#include <string>

namespace System
{

namespace Net
{

namespace Http
{

typedef std::vector<std::string> HttpListenerPrefixCollection;

class HttpListener
{
    class InternalHttpListener *_internal;

public:
    // Initializes a new instance of the HttpListener class.
    HttpListener();

    virtual ~HttpListener();

public:
    // Gets a value that indicates whether HttpListener has been started.
    bool IsListening() const;

    // Gets the Uniform Resource Identifier (URI) prefixes handled by this HttpListener object.
    HttpListenerPrefixCollection &Prefixes();

public:
    // Shuts down the HttpListener object immediately, discarding all currently queued requests.
    void Abort();

    // Shuts down the HttpListener.
    void Close();

    // Waits for an incoming request and returns when one is received.
    HttpListenerContext *GetContext();

    // Allows this instance to receive incoming requests.
    void Start();

    // Causes this instance to stop receiving incoming requests.
    void Stop();
};

}

}

}

#endif // HTTPLISTENER_H
