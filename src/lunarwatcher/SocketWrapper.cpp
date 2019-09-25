#include "net/SocketWrapper.hpp"
#include "AdvLand.hpp"
#include "math/Logic.hpp"
#include "utils/ParsingUtils.hpp"
#include <algorithm>

namespace advland {

#if USE_STATIC_ENTITIES
std::map<string, nlohmann::json> SocketWrapper::entities = {};
#endif

#define WIDTH_HEIGHT_SCALE                                                                                             \
    {"width", 1920}, {"height", 1080}, { "scale", 2 }

SocketWrapper::SocketWrapper(std::string characterId, std::string fullUrl, AdvLandClient& client, Player& player)
        : webSocket(), client(client), player(player), characterId(characterId), hasReceivedFirstEntities(false) {
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

void SocketWrapper::sanitizeInput(nlohmann::json& entity) {
    if (entity.find("rip") != entity.end()) {
        if (entity["rip"].is_number()) {
            entity["rip"] = (int(entity["rip"]) == 1);
        }
    }
}

void SocketWrapper::initializeSystem() {
    this->webSocket.setOnMessageCallback(
        [this](const ix::WebSocketMessagePtr& message) { this->messageReceiver(message); });

    // Loading

    this->registerEventCallback("welcome", [this](const nlohmann::json&) {
        this->emit("loaded", {{"success", 1}, WIDTH_HEIGHT_SCALE});
    });

    this->registerEventCallback("start", [this](const nlohmann::json& event) {
        // The start event contains necessary data to populate the character
        nlohmann::json mut = event;
        // Used for, among other things, canMove. This contains the character bounding box
        // I have no idea what the letters symbolize.
        mut["base"] = {{"h", 8}, {"v", 7}, {"vn", 2}};
        this->player.updateJson(mut);
        this->player.onConnect();
    });

    // Loading + gameplay
    this->registerEventCallback("entities", [this](const nlohmann::json& event) {
        std::string type = event["type"].get<std::string>();
        // mLogger->info(event.dump());
        if (type == "all" && !hasReceivedFirstEntities) {
            login();
            hasReceivedFirstEntities = true;
        }

        if (event.find("players") != event.end()) {
            nlohmann::json players = event["players"];
            for (auto& player : players) {
                sanitizeInput(player);
                player["type"] = "character";
                player["base"] = {{"h", 8}, {"v", 7}, {"vn", 2}};
                auto id = player["id"].get<std::string>();
                if (id == "") {
                    mLogger->error("Found empty ID? Dumping JSON: {}", event.dump());
                    return;
                }

                if (id == this->player.getUsername()) {
                    this->player.updateJson(player);
                }
                // Avoid data loss by updating the JSON rather than overwriting
                if (this->entities.find(id) == entities.end())
                    this->entities[id] = player;
                else {
                    this->entities[id].update(player);
                }
            }
        }

        if (event.find("monsters") != event.end()) {
            nlohmann::json monsters = event["monsters"];

            for (auto& monster : monsters) {
                auto id = monster["id"].get<std::string>();

                sanitizeInput(monster);
                monster["mtype"] = monster["type"].get<std::string>();
                monster["type"] = "monster";
                if (id == "") {
                    mLogger->error("Empty monster ID? Dumping json: {}", event.dump());
                    return;
                }
                if (monster.find("hp") == monster.end()) {
                    monster["hp"] = player.getClient().getData()["monsters"][std::string(monster["mtype"])]["hp"];
                    if (monster.find("max_hp") == monster.end()) monster["max_hp"] = monster["hp"];
                }
                if (this->entities.find(id) == entities.end())
                    this->entities[id] = monster;
                else {
                    this->entities[id].update(monster);
                }
            }
        }
    });

    // Gameplay

    // Methods recording the disappearance of entities.
    // Death is also registered locally using this socket event
    this->registerEventCallback("death", [this](const nlohmann::json& event) {
        nlohmann::json copy = event;
        copy["death"] = true;
        onDisappear(copy);
    });
#define onDisappearConsumer(eventName)                                                                                 \
    this->registerEventCallback(eventName, [this](const nlohmann::json& event) { onDisappear(event); });

    onDisappearConsumer("disappear");
    onDisappearConsumer("notthere");
#undef onDisappearConsumer

    // Chests are recorded with the drop event
    this->registerEventCallback("drop", [this](const nlohmann::json& event) {
        std::lock_guard<std::mutex> guard(chestGuard);
        chests[event["id"].get<std::string>()] = event;
    });
    this->registerEventCallback("chest_opened", [this](const nlohmann::json& event) {
        std::lock_guard<std::mutex> guard(chestGuard);
        chests.erase(event["id"].get<std::string>()); 
    });
    // This contains updates to the player entity. Unlike other entities (AFAIK), these aren't
    // sent using the entities event.
    this->registerEventCallback("player", [this](const nlohmann::json& event) {
        int cachedSpeed = player.getSpeed();
        player.updateJson(event);
        if (player.getSpeed() != cachedSpeed) {
            if (player.isMoving()) {
                auto vxy = MovementMath::calculateVelocity(player.getRawJson());
                player.getRawJson()["vx"] = vxy.first;
                player.getRawJson()["vy"] = vxy.second;
            }
        }
    });
    this->registerEventCallback("new_map", [this](const nlohmann::json& event) {
        player.updateJson({{"map", event["name"].get<std::string>()},
                {"x", event["x"].get<int>()},
                {"y", event["y"].get<int>()},
                {"m", event["m"].get<int>()},
                {"moving", false}});
    });
    this->registerEventCallback("cm", [this](const nlohmann::json& event) {
        player.getSkeleton().onCm(event["name"].get<std::string>(), event["message"]); 
    });
    this->registerEventCallback("pm", [this](const nlohmann::json& event){
        std::string owner = event["owner"].get<std::string>();
        player.getSkeleton().onPm(owner, event["message"].get<std::string>());
    });
    this->registerEventCallback("chat_log", [this](const nlohmann::json& event) {
        player.getSkeleton().onChat(event["owner"].get<std::string>(), event["message"].get<std::string>());
    });
    this->registerEventCallback("invite", [this](const nlohmann::json& event) {
        player.getSkeleton().onPartyInvite(event["name"].get<std::string>());
    });
    this->registerEventCallback("request", [this](const nlohmann::json& event) {
        player.getSkeleton().onPartyRequest(event["name"].get<std::string>());
    });
    /**
     * Position correction. 
     */
    this->registerEventCallback("correction", [this](const nlohmann::json& event) {
        mLogger->warn("Location corrected!");
        // TODO 
    });
    this->registerEventCallback("party_update", [this](const nlohmann::json& event) {
        player.setParty(event["party"]); 
    });
    
    this->registerEventCallback("game_error", [this](const nlohmann::json& event) {
        
    });
}

void SocketWrapper::login() {
    std::string& auth = client.getAuthToken();
    std::string& userId = client.getUserId();

    emit("auth", {{"user", userId},
                  {"character", characterId},
                  {"auth", auth},
                  WIDTH_HEIGHT_SCALE,
                  {"no_html", true},
                  {"no_graphics", true} /*, {"passphrase", ""}*/});
}

void SocketWrapper::emit(std::string event, const nlohmann::json& json) {
    if (this->webSocket.getReadyState() == ix::ReadyState::Open) {
        std::string i = "42[\"" + event + "\"," + json.dump() + "]";
        // mLogger->info("emitting \"{}\"", i);
        this->webSocket.send(i);
    } else {
        mLogger->error("Attempting to call emit on a socket that hasn't opened yet.");
    }
}

void SocketWrapper::messageReceiver(const ix::WebSocketMessagePtr& message) {
    if (pingInterval != 0 && message->type != ix::WebSocketMessageType::Close) {
        auto now = std::chrono::high_resolution_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastPing).count();

        // The socket expects a ping every few milliseconds (currently, it's every 4000 milliseconds)
        // This is sent through the websocket on the frame type 0
        if (diff > pingInterval) {
            lastPing = now;
            sendPing();
        }
    }

    // All the Socket.IO events also come through as messages
    if (message->type == ix::WebSocketMessageType::Message) {
        // Uncomment for data samples
        // this->mLogger->info("Received: {}", message->str);

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
            mLogger->info("Disconnected: {}", message->str);
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
                dispatchEvent("message", json);
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
                            if (eventName == "error")
                                mLogger->info("Error received as a message! Dumping JSON:\n{}", json.dump(4));
                            dispatchEvent(eventName, data);
                        }
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
            } else {
                mLogger->warn("Unknown event received: {}", messageStr);
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
    } else if (message->type == ix::WebSocketMessageType::Close) {
        mLogger->info("Socket disconnected: {}", message->closeInfo.reason);
        hasReceivedFirstEntities = false;
        if (message->str != "")
            mLogger->info("Also received a message: {}", message->str);
    }
}

void SocketWrapper::receiveLocalCm(std::string from, const nlohmann::json& message) {
    player.getSkeleton().onCm(from, message);
}

void SocketWrapper::dispatchEvent(std::string eventName, const nlohmann::json& event) {
    if (eventCallbacks.find(eventName) != eventCallbacks.end()) {
        for (auto& callback : eventCallbacks[eventName]) {
            callback(event);
        }
    }
}

void SocketWrapper::deleteEntities() {
    std::lock_guard<std::mutex> guard(deletionGuard);

    for (auto it = entities.begin(); it != entities.end();) {
        if (getOrElse((*it).second, "dead", false) || getOrElse((*it).second, "rip", false)) {
            it = entities.erase(it);
        } else
            ++it;
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

void SocketWrapper::onDisappear(const nlohmann::json& event) {
    if (entities.find(event["id"]) != entities.end()) {
        auto& entity = entities[event["id"].get<std::string>()];
        entity["dead"] = true;
        if (getOrElse(event, "teleport", false)) {
            entity["tpd"] = true;
        }
        if (getOrElse(event, "invis", false)) {
            entity["invis"] = true;
        }
    }
}

SocketConnectStatusCode SocketWrapper::connect() {
    // Begin the connection process
    this->webSocket.start();

    return SocketConnectStatusCode::SUCCESSFUL;
}

void SocketWrapper::close() {
    if (this->webSocket.getReadyState() == ix::ReadyState::Open) {
        this->webSocket.stop();
    }
}

void SocketWrapper::sendPing() {
    this->webSocket.send("2::"); // ping is event 2
}

std::map<std::string, nlohmann::json>& SocketWrapper::getEntities() { return entities; }
std::map<std::string, nlohmann::json>& SocketWrapper::getChests() { return chests; }
#undef WIDTH_HEIGHT_SCALE
} // namespace advland

