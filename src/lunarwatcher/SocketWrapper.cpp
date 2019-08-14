#include "net/SocketWrapper.hpp"
#include <algorithm>

namespace advland {

SocketWrapper::SocketWrapper(std::string characterId, std::string fullUrl, AdvLandClient& client)
        : webSocket(), client(client), characterId(characterId) {
    // In order to faciliate for websocket connection, a special URL needs to be used.
    // By adding this, the connection can be established as a websocket connection.
    // A real socket.io client likely uses this or something similar internally
    fullUrl += "/socket.io/?EIO=3&transport=websocket";

    if (fullUrl.find("wss://") == std::string::npos) {
        this->webSocket.setUrl("wss://" + fullUrl);
    } else {
        this->webSocket.setUrl(fullUrl);
    }
    this->pingInterval = 4000;
    lastPing = std::chrono::high_resolution_clock::now();

    initializeSystem();
}

SocketWrapper::~SocketWrapper() { webSocket.close(); }

void SocketWrapper::initializeSystem() {
    this->webSocket.setOnMessageCallback(
        [this](const ix::WebSocketMessagePtr& message) { this->messageReceiver(message); });

    this->registerEventCallback("welcome", [this](const nlohmann::json& event) {
        this->emit("loaded", {{"success", 1}, {"width", 1920}, {"height", 1080}, {"scale", 2}});
    });
}

void SocketWrapper::emit(std::string event, const nlohmann::json& json) {
    if (this->webSocket.getReadyState() == ix::ReadyState::Open) {
        std::string i = "42[\"" + event + "\"," + json.dump() + "]";
        mLogger->info("emitting \"{}\"", i);
        this->webSocket.send(i);
    } else {
        mLogger->error("Attempting to call emit on a socket that hasn't opened yet.");
    }
}

void SocketWrapper::messageReceiver(const ix::WebSocketMessagePtr& message) {
    if (pingInterval != 0 && message->type != ix::WebSocketMessageType::Close) {
        auto now = std::chrono::high_resolution_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastPing).count();
        mLogger->info("Diff: {}", diff);
        // The socket expects a ping every few milliseconds (currently, it's every 4000 milliseconds)
        // This is sent through the websocket on the frame type 0
        if (diff > pingInterval) {
            lastPing = now;
            sendPing();
        }
    }

    // All the Socket.IO events also come through as messages
    if (message->type == ix::WebSocketMessageType::Message) {
        this->mLogger->info("Received: {}", message->str);

        std::string& messageStr = message->str;
        if (messageStr.length() == 0) {
            mLogger->info("Received empty websocket message.");
            return;
        }

        if (messageStr.length() == 1) {
            int type = std::stoi(messageStr);
            if (type == 0)
                mLogger->warn("Received empty connect message! Assuming 4000 milliseconds for the ping interval");
            else if (type == 3) {
                // pong
            } else if (type == 2) {
                // ping
            }

            return;
        }

        std::string args;
        int likelyType = messageStr[0] - '0';      // frame type
        int alternativeType = messageStr[1] - '0'; // message type

        if (likelyType < 0 || likelyType > 9) {
            mLogger->error("Failed to parse message: \"{}\". Failed to find frame type", messageStr);
            return;
        }
        if (messageStr.length() == 2 && alternativeType >= 0 && alternativeType <= 9) {
            mLogger->debug("Skipping code-only input: \"{}\"", messageStr);
            return;
        }

        // To get the message itself, we need to parse the numbers.
        // We know the string is two or more characters at this point.
        // If alternativeType < 0 || alternativeType > 9, that means the character,
        // when '0' is subtracted, isn't a number. For that to be the case, it would
        // have to be in that range.
        // Anyway, first condition hits if we have one digit, the second if there's two
        if (alternativeType < 0 || alternativeType > 9)
            args = messageStr.substr(1, messageStr.length() - 1);
        else
            args = messageStr.substr(2, messageStr.length() - 2);

        switch (likelyType) {
        case 0: {
            dispatchEvent("connect", {});
            auto data = nlohmann::json::parse(args);
            pingInterval = data["pingInterval"].get<int>();
            mLogger->info("Received connection data. Pinging required every {} ms", pingInterval);
        } break;
        case 1:
            dispatchEvent("disconnect", {});
            break;
        case 2:
            dispatchEvent("ping", {});
            break;
        case 3:
            dispatchEvent("pong", {});
            break;
        case 4: {
            if (rawCallbacks.size() > 0) {
                for (auto& callback : rawCallbacks) {
                    callback(message);
                }
            }
            int type = -1;
            if (alternativeType >= 0 && alternativeType <= 9) type = alternativeType;
            nlohmann::json json = nlohmann::json::parse(args);

            if (type == -1) {
                // Dispatch message event.
                // Note about the socket.io standard: sending a plain text message using socket.send
                // should send an event called message with the message as the data. The fallback
                // exists mainly because I have no idea what I'm doing. This might never be used.
            }
            // type = 0 for the connect packet
            // type = 1 for the disconnect packet
            // type = 2 for events
            else if (type == 2) {
                if (json.type() == nlohmann::json::value_t::array && json.size() >= 1) {
                    auto& event = json[0];
                    if (event.type() == nlohmann::json::value_t::string) {
                        std::string eventName = event.get<std::string>();
                        if (json.size() == 1) {
                            // dispatch eventName, {}
                            dispatchEvent(eventName, {});
                        } else {
                            auto data = json[1];
                            dispatchEvent(eventName, data);
                        }

                        triggerInternalEvents(eventName, json.size() == 1 ? nlohmann::json{} : json[1]);
                    }
                }
            }
            // type = 3 for ack (AFAIK, not used)
            // type = 4 for errors (no idea how to handle, gotta revisit that when I have data)
            else if (type == 4) {
                mLogger->error("Error received from server:\n{}", json.dump(4));

            }
            // type = 5 for binary event (AFAIK, not used)
            // type = 6 for binary ack (AFAIK, not used)
            else if (type == 3 || type == 5 || type == 6) {
                mLogger->warn("Received an event of type {}. Please report this issue here: "
                              "https://github.com/LunarWatcher/AdventureLandCpp/issues/1 - Full websocket message: {}",
                              type, messageStr);
            }

        } break;
        case 5:
            dispatchEvent("upgrade", {});
            mLogger->info("Upgrade received: {}", messageStr);
            // upgrade
            break;
        case 6:
            dispatchEvent("noop", {});
            mLogger->info("noop received: {}", messageStr);
            // noop
            break;
        }

    } else if (message->type == ix::WebSocketMessageType::Error) {
        mLogger->error(message->errorInfo.reason);
    } else if (message->type == ix::WebSocketMessageType::Open) {
        mLogger->info("Connected");
    }
}

void SocketWrapper::triggerInternalEvents(std::string eventName, const nlohmann::json& event) {
    // TODO
}

void SocketWrapper::dispatchEvent(std::string eventName, const nlohmann::json& event) {
    if (eventCallbacks.find(eventName) != eventCallbacks.end()) {
        for (auto& callback : eventCallbacks[eventName]) {
            callback(event);
        }
    }
}

void SocketWrapper::registerRawMessageCallback(std::function<void(const ix::WebSocketMessagePtr&)> callback) {
    rawCallbacks.push_back(callback);
}

void SocketWrapper::registerEventCallback(std::string event, std::function<void(const nlohmann::json&)> callback) {
    if (eventCallbacks.find(event) == eventCallbacks.end()) {
        eventCallbacks[event].push_back(callback);
    }
}

SocketConnectStatusCode SocketWrapper::connect() {
    // Begin the connection process
    this->webSocket.start();

    return SocketConnectStatusCode::SUCCESSFUL;
}

void SocketWrapper::sendPing() {
    this->webSocket.send("2::"); // ping is event 2
}

} // namespace advland

