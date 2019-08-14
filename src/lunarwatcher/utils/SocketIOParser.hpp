#ifndef LUNARWATCHER_ADVLAND_UTILS_SOCKETIOPARSER_H
#define LUNARWATCHER_ADVLAND_UTILS_SOCKETIOPARSER_H

#include <string>
#include <nlohmann/json.hpp>
#include "lunarwatcher/meta/Exceptions.hpp"
#include <chrono>
#include "lunarwatcher/net/SocketWrapper.hpp"

namespace advland {

typedef std::chrono::high_resolution_clock Clock;

#define FLAG_STANDALONE_MESSAGE 0xa104e
class SocketWrapper;
class Message {
private:
    int type;
    std::string message;
    nlohmann::json parsedData;

public:
    Message(int type, std::string message) : type(type), message(message) {
        this->parsedData = nlohmann::json::parse(message);
    }

    int& getType() { return type; }
    std::string& getRawMessage() { return message; }
    nlohmann::json& getParsedMessage() { return parsedData; }
};

extern void onMessage(std::string& message, SocketWrapper& wrapper);
extern std::string createMessage(int type, std::string& message);

}


#endif 
