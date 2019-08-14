#ifndef LUNARWATCHER_ADVLAND_EXCEPTIONS
#define LUNARWATCHER_ADVLAND_EXCEPTIONS

#include <exception>
#include <string>

namespace advland {

// Generic exception class. 
#define createException(name) \
    class name : std::exception { \
    private: \
        std::string message; \
    public: \
        name(std::string message) : message(message) {} \
        virtual const char* what() const throw() { return message.c_str(); } \
    };

createException(LoginException)
createException(EndpointException)

createException(IllegalArgumentException)
createException(EmptyInputException)
createException(IllegalStateException)


#undef createException
}

#endif 
