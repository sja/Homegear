#include "Exception.h"

Exception::Exception(std::string message)
{
    _message = message.c_str();
}
