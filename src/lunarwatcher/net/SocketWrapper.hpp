#ifndef LUNARWATCHER_NET_SOCKETWRAPPER
#define LUNARWATCHER_NET_SOCKETWRAPPER

#include <functional>
#include <ixwebsocket/IXWebSocket.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>
namespace advland {

enum SocketConnectStatusCode {
    SUCCESSFUL,
    ERR_NO_UID,
    ERR_SOCKET_FAILURE,
    ERR_ALREADY_CONNECTED

};
class AdvLandClient;

class SocketWrapper {
private:
    static auto inline const mLogger = spdlog::stdout_color_mt("SocketWrapper");
    std::shared_ptr<ix::WebSocket> webSocket;
    AdvLandClient& client;
    std::string characterId;

    std::vector<std::function<void(const ix::WebSocketMessagePtr&)>> rawCallbacks;
    std::vector<std::pair<std::string, std::function<void(const ix::WebSocketMessagePtr&)>>> eventCallback;

    void messageReceiver(const ix::WebSocketMessagePtr& message);
    void initializeSystem();

public:
    /**
     * Initializes a general, empty SocketWrapper ready to connect.
     * When {@link #connect(std::string)} is called, the socket starts up
     * and runs the connection procedures for the target server, the
     * newly created SocketWrapper will be designated to a specific in-game
     * character.
     */
    SocketWrapper(std::string characterId, std::string fullUrl, AdvLandClient& client);
    
    void registerRawMessageCallback(std::function<void(const ix::WebSocketMessagePtr&)> callback);
    void registerEventCallback(std::string event, std::function<void(const nlohmann::json&)> callback);

    /**
     *  Connects a user. this should only be run from the Player class
     */
    SocketConnectStatusCode connect();
};

} // namespace advland

#endif
