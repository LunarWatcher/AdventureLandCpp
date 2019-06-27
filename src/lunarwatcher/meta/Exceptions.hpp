#ifndef LUNARWATCHER_ADVLAND_EXCEPTIONS
#define LUNARWATCHER_ADVLAND_EXCEPTIONS

#include <exception>
#include <string>

namespace advland {

// Generic exception class. 
#define CREATE_EXCEPTION(name) \
    class name : std::exception { \
    private: \
        std::string message; \
    public: \
        name(std::string message) : message(message) {} \
        virtual const char* what() const throw() { return message.c_str(); } \
    };

CREATE_EXCEPTION(LoginException)
CREATE_EXCEPTION(EndpointException)

CREATE_EXCEPTION(IllegalArgumentException)

}

#endif 
