#include "utils/SocketIOParser.hpp"

void advland::onMessage(std::string& message, advland::SocketWrapper& wrapper) {
    wrapper.sendPing(); 
}

std::string advland::createMessage(int type, std::string& message) {
    return std::to_string(type) + message;
}

