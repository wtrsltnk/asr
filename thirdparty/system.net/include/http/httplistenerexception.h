#ifndef HTTPLISTENEREXCEPTION_H
#define HTTPLISTENEREXCEPTION_H

#include <exception>
#include <string>

namespace System
{

namespace Net
{

namespace Http
{

class HttpListenerException : public std::exception
{
    std::string _message;

public:
    HttpListenerException(std::string const &message);

    std::string const &Message() const;
};

}

}

}

#endif // HTTPLISTENEREXCEPTION_H
