#include "http/httplistener.h"
#include "http/httplistenerexception.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <algorithm>
#include <sstream>
#include <regex>
#include <iostream>

using namespace System::Net::Http;

// Global Functions
std::string  ToString(int data)
{
    std::stringstream buffer;
    buffer << data;
    return buffer.str();
}

// trim from start
static inline std::string &ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
                                    std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

// trim from both ends
static inline std::string &trim(std::string &s) {
    return ltrim(rtrim(s));
}

namespace System
{

namespace Net
{

namespace Http
{

#define BUFFER_SIZE 1024*5 // 5KB

class InternalHttpListenerRequest : public HttpListenerRequest
{
    SOCKET _socket;
    sockaddr_in _clientInfo;
    char buffer[BUFFER_SIZE+1];
    int bytes;

    std::string readAllData()
    {
        ZeroMemory(&buffer, sizeof(buffer));

        bytes = recv(_socket, buffer, BUFFER_SIZE, 0);
        if (bytes < 0)
        {
            throw new HttpListenerException("Could not recieve any data");
        }
        buffer[bytes] = '\0';
        std::string browserData = buffer;

        while (BUFFER_SIZE == bytes)
        {
            bytes = recv(_socket, buffer, BUFFER_SIZE, 0);
            buffer[bytes] = '\0';
            browserData += buffer;
        }

        return browserData;
    }
public:
    InternalHttpListenerRequest(SOCKET socket, sockaddr_in clientInfo)
        : _socket(socket), _clientInfo(clientInfo)
    {
        std::string allData = readAllData();

        auto pos = allData.find("\r\n\r\n");
        if (pos == std::string::npos)
        {
            return;
        }

        std::stringstream ss;
        ss.str(allData.substr(0, pos));
        std::string line;

        // Determine methode, uri and HTTP version
        if (std::getline(ss, line))
        {
            trim(line);
            auto first = line.find_first_of(' ');
            auto last = line.find_last_of(' ');
            if (first != std::string::npos && last != std::string::npos)
            {
                auto version = line.substr(last);
                if (trim(version) != "HTTP/1.1")
                {
                    throw new HttpListenerException("invalid HTTP version");
                }

                auto method = line.substr(0, first);
                this->_httpMethod = trim(method);

                auto uri = line.substr(first, last-first);
                this->_rawUrl = trim(uri);
            }
        }

        // Determine headers
        while (std::getline(ss, line))
        {
            auto pos = line.find_first_of(':');
            if (pos != std::string::npos)
            {
                auto key = line.substr(0, pos);
                auto value = line.substr(pos + 1);
                this->_headers.insert(std::make_pair(trim(key), trim(value)));
            }
        }

        // And finally determine payload(if any)
        this->_payload = allData.substr(pos + 4);
    }

    std::string ipAddress() const
    {
        return std::string(inet_ntoa(this->_clientInfo.sin_addr));
    }
};

class InternalHttpListenerResponse : public HttpListenerResponse
{
    SOCKET _socket;
    sockaddr_in _clientInfo;
public:
    InternalHttpListenerResponse(SOCKET socket, sockaddr_in clientInfo)
        : _socket(socket), _clientInfo(clientInfo)
    { }

    void CloseOutput()
    {
        std::stringstream headers;

        headers << "HTTP/1.1 " << _statusCode << " " << _statusDescription << "\r\n";

        for (auto pair : _headers)
        {
            headers << pair.first << ": " << pair.second << "\r\n";
        }

        headers << "Content-Length: " << _output.size() << "\r\n"
                << "\r\n";

        send(_socket, headers.str().c_str(), headers.str().size(), 0);
        send(_socket, _output.c_str(), _output.size(), 0);

        shutdown(_socket, SD_BOTH);
        closesocket(_socket);
    }
};

class InternalHttpListenerContext : public HttpListenerContext
{
    InternalHttpListenerRequest _internalRequest;
    InternalHttpListenerResponse _internalResponse;
public:
    InternalHttpListenerContext(SOCKET socket, sockaddr_in clientInfo)
        : HttpListenerContext(), _internalRequest(socket, clientInfo), _internalResponse(socket, clientInfo)
    {
        _request = &_internalRequest;
        _response = &_internalResponse;
    }
    virtual ~InternalHttpListenerContext()
    { }
};

class InternalHttpListener
{
public:
    SOCKET _listeningSocket;
    HttpListenerPrefixCollection _prefixes;
    int _maxConnections;

    InternalHttpListener()
        : _listeningSocket(0), _maxConnections(50)
    { }
};

}

}

}

HttpListener::HttpListener()
    : _internal(new InternalHttpListener())
{
    WORD socketVersion = MAKEWORD(2,2);
    WSADATA wsaData;

    auto resultCode = WSAStartup(socketVersion, &wsaData);
    if (0 != resultCode)
    {
        throw new HttpListenerException("init winsock failed");
    }
}

HttpListener::~HttpListener()
{
    Abort();
    delete _internal;
    WSACleanup();
}

// Gets a value that indicates whether HttpListener has been started.
bool HttpListener::IsListening() const
{
    return (_internal->_listeningSocket != 0);
}

// Gets the Uniform Resource Identifier (URI) prefixes handled by this HttpListener object.
HttpListenerPrefixCollection &HttpListener::Prefixes()
{
    return _internal->_prefixes;
}

// Shuts down the HttpListener object immediately, discarding all currently queued requests.
void HttpListener::Abort()
{ }

// Shuts down the HttpListener.
void HttpListener::Close()
{ }

// Allows this instance to receive incoming requests.
void HttpListener::Start()
{
    if (IsListening())
    {
        throw new HttpListenerException("Already started");
    }

    std::regex rgx("^([^:]*):\\/\\/([^:\\/]*):?([0-9]*)(\\/?[\\w-_\\/]*)$");

    std::string port;
    std::string schema;

    for (auto prefix : Prefixes())
    {
        std::smatch matches;

        if(std::regex_search(prefix, matches, rgx))
        {
            auto prefixSchema = matches[1].str();
            if (prefixSchema != "")
            {
                if (schema != "" && schema != prefixSchema)
                {
                    throw new HttpListenerException("Not all prefixes use the same schema");
                }
                schema = prefixSchema;
            }

            auto prefixHost = matches[2].str();

            auto prefixPort = matches[3].str();
            if (prefixPort != "")
            {
                if (port != "" && port != prefixPort)
                {
                    throw new HttpListenerException("Not all prefixes use the same port");
                }
                port = prefixPort;
            }

            auto prefixPath = matches[4].str();
        }
    }

    if (port == "")
    {
        port = "8888";
    }

    std::cout << "Listening @ " << port << std::endl;

    addrinfo hints, *result;

    // Socket Settings
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve Local Address And Port
    auto resultCode = getaddrinfo(NULL, port.c_str(), &hints, &result);
    if (0 != resultCode)
    {
        throw new HttpListenerException(std::string("Resolving Address And Port Failed \nError Code: ") + ToString(resultCode));
    }

    // Create Socket
    _internal->_listeningSocket = socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);
    if (INVALID_SOCKET == _internal->_listeningSocket)
    {
        throw new HttpListenerException("Could't Create Socket");
    }

    // Bind
    resultCode = bind(_internal->_listeningSocket, result->ai_addr, (int)(result->ai_addrlen));
    if (SOCKET_ERROR == resultCode)
    {
        throw new HttpListenerException("Bind Socket Failed");
    }

    // Listen
    resultCode = listen(_internal->_listeningSocket, _internal->_maxConnections);
    if (SOCKET_ERROR == resultCode)
    {
        throw new HttpListenerException(std::string("Listening On Port ") + port + " Failed");
    }
}

// Waits for an incoming request and returns when one is received.
HttpListenerContext *HttpListener::GetContext()
{
    sockaddr_in clientInfo;
    int clientInfoSize = sizeof(clientInfo);

    auto socket = accept(_internal->_listeningSocket, (sockaddr*)&clientInfo, &clientInfoSize);

    if (INVALID_SOCKET == socket)
    {
        throw HttpListenerException("Invalid socket");
    }

    return new InternalHttpListenerContext(socket, clientInfo);
}

// Causes this instance to stop receiving incoming requests.
void HttpListener::Stop()
{
    if (IsListening())
    {
        closesocket(_internal->_listeningSocket);
    }
}
