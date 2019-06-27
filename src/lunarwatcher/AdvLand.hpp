#ifndef LUNARWATCHER_ADVLAND
#define LUNARWATCHER_ADVLAND

#include "Poco/Exception.h"
#include "Poco/Net/Context.h"
#include "Poco/Net/HTMLForm.h"
#include "Poco/Net/HTTPCookie.h"
#include "Poco/Net/HTTPMessage.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/HTTPSClientSession.h"
#include "Poco/Net/NameValueCollection.h"
#include "Poco/Net/Socket.h"
#include "Poco/Net/WebSocket.h"
#include "Poco/StreamCopier.h"
#include "nlohmann/json.hpp"

#include "meta/Typedefs.hpp"
#include "objects/GameData.hpp"
#include "objects/Server.hpp"

#include <algorithm>
#include <iostream>
#include <istream>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <string>
#include <utility>

namespace advland {

using namespace Poco;
using namespace Poco::Net;

class AdvLandClient {
private:
    static auto inline const mLogger = spdlog::stdout_color_mt("AdvLandClient");
    HTTPSClientSession session;
    WebSocket* websocket;

    // This is the session cookie. It's used with some calls, and is essential for base
    // post-login auth operations.
    std::string authToken;

    // Mirror of the in-game G variable
    GameData data;

    // vector<pair<id, username>>
    std::vector<std::pair<std::string, std::string>> characters;
    std::vector<ServerCluster> serverClusters;

    void login(std::string& email, std::string& password);
    void collectGameData();
    void collectCharacters();
    void collectServers();

    void parseCharacters(nlohmann::json& data);
    void connectWebsocket();

    void postRequest(std::stringstream& out, HTTPResponse& response, std::string apiEndpoint, std::string arguments,
                     bool auth, const std::vector<CookiePair>& formData = {});

public:
    AdvLandClient(std::string email, std::string password);

    virtual ~AdvLandClient();
   
    ServerCluster* getServerCluster(std::string identifier) {
        for (ServerCluster& cluster : serverClusters) {
            if (cluster.getRegion() == identifier) return &cluster;
        }
        return nullptr;
    }

    Server* getServerInCluster(std::string clusterIdentifier, std::string serverIdentifier) {
        ServerCluster* cluster = getServerCluster(clusterIdentifier);
        if (!cluster) return nullptr;
        return cluster->getServerByName(serverIdentifier);
    }


};

} // namespace advland

#endif
