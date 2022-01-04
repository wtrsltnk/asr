#include "http/httplistenerexception.h"

using namespace System::Net::Http;

HttpListenerException::HttpListenerException(std::string const &message)
    : _message(message)
{ }

std::string const &HttpListenerException::Message() const
{
    return _message;
}
