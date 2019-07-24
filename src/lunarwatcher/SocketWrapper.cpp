#include "net/SocketWrapper.hpp"

namespace advland {

SocketWrapper::SocketWrapper(std::string characterId, std::string fullUrl, AdvLandClient& client) : webSocket(new ix::WebSocket()), client(client), characterId(characterId){
    if (fullUrl.find("wss://") == std::string::npos) {
        this->webSocket->setUrl("wss://" + fullUrl);
    } else {
        this->webSocket->setUrl(fullUrl);
    }
    initializeSystem();
}

void SocketWrapper::initializeSystem() {
    this->webSocket->setOnMessageCallback([this](const ix::WebSocketMessagePtr& message) {
                this->messageReceiver(message);  
            }); 
}

void SocketWrapper::messageReceiver(const ix::WebSocketMessagePtr& message) {
    if (message->type == ix::WebSocketMessageType::Message) {
        this->mLogger->info("Received: {}", message->str);
    }else if (message->type == ix::WebSocketMessageType::Error) {
        mLogger->error(message->errorInfo.reason);
    } 
}

void SocketWrapper::registerRawMessageCallback(std::function<void(const ix::WebSocketMessagePtr&)> callback) {

}

void SocketWrapper::registerEventCallback(std::string event, std::function<void(const nlohmann::json&)> callback) {
    
}

SocketConnectStatusCode SocketWrapper::connect() {
    // Begin the connection process
    this->webSocket->start();

    return SocketConnectStatusCode::SUCCESSFUL;
}

} // namespace advland

