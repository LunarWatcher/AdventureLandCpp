#ifndef LUNARWATCHER_NET_SOCKETWRAPPER
#define LUNARWATCHER_NET_SOCKETWRAPPER

#ifndef USE_STATIC_ENTITIES
#define USE_STATIC_ENTITIES false
#endif 

#include <functional>
#include <ixwebsocket/IXWebSocket.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>
#include <map>
#include "lunarwatcher/utils/SocketIOParser.hpp"

namespace advland {

enum SocketConnectStatusCode {
    SUCCESSFUL,
    ERR_NO_UID,
    ERR_SOCKET_FAILURE,
    ERR_ALREADY_CONNECTED

};
typedef std::function<void(const ix::WebSocketMessagePtr&)> RawCallback;
typedef std::function<void(const nlohmann::json&)> EventCallback;

class AdvLandClient;
class Message;
class Player;

class SocketWrapper {
private:
    static auto inline const mLogger = spdlog::stdout_color_mt("SocketWrapper");
    ix::WebSocket webSocket;
    AdvLandClient& client;
    Player& player;

    std::string characterId;
    // ping managing 
    int pingInterval;
    std::chrono::time_point<std::chrono::high_resolution_clock> lastPing;

    // Entity management
    bool hasReceivedFirstEntities;

    // Callbacks 
    std::vector<RawCallback> rawCallbacks;
    std::map<std::string, std::vector<EventCallback>> eventCallbacks;

    // Entities 
    #if USE_STATIC_ENTITIES 
    static 
    #endif 
    std::map<std::string, nlohmann::json> entities;

    // Functions
    void triggerInternalEvents(std::string eventName, const nlohmann::json& event);
    void dispatchEvent(std::string eventName, const nlohmann::json& event);
    void messageReceiver(const ix::WebSocketMessagePtr& message);
    void initializeSystem();
    void login();

    /**
     * Cleans up input to avoid type bugs introduced by the backend. 
     * One notable use for this is with the `rip` attribute. 
     * Some chars have an integer value as the value, but this 
     * client expects a bool, meaning anything else will trigger a 
     * crash.
     */
    void sanitizeInput(nlohmann::json& entity); 
public:
    /**
     * Initializes a general, empty SocketWrapper ready to connect.
     * When {@link #connect(std::string)} is called, the socket starts up
     * and runs the connection procedures for the target server, the
     * newly created SocketWrapper will be designated to a specific in-game
     * character.
     */
    SocketWrapper(std::string characterId, std::string fullUrl, AdvLandClient& client, Player& player);
    ~SocketWrapper();

    /**
     * Registers a listener that receives raw events straight from the websocket. These aren't
     * parsed - at all - and the receiving function is expected to do so. 
     *
     * This only sends messages - other status codes aren't sent.
     */
    void registerRawMessageCallback(std::function<void(const ix::WebSocketMessagePtr&)> callback);
    /**
     * Equivalent of socket.on
     */
    void registerEventCallback(std::string event, std::function<void(const nlohmann::json&)> callback);

    /**
     *  Connects a user. this should only be run from the Player class
     */
    SocketConnectStatusCode connect();
    void close();

    void sendPing();

    void emit(std::string event, const nlohmann::json& json);
#if USE_STATIC_ENTITIES
    static
#endif
    std::map<std::string, nlohmann::json>& getEntities();
    ix::ReadyState getReadyState() { return webSocket.getReadyState(); }
    
};

#if USE_STATIC_ENTITIES
std::map<string, nlohmann::json> SocketWrapper::entities = {};
#endif


} // namespace advland
#endif
