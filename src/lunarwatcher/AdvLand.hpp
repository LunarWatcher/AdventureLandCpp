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

#include "objects/GameData.hpp"

#include <iostream>
#include <istream>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <string>

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

    GameData data;

    void login(std::string& email, std::string& password);
    void collectGameData();
    void collectCharactersAndServers();

public:
    AdvLandClient(std::string email, std::string password);

    virtual ~AdvLandClient() {
        /* TODO - requires WS initialization to avoid segfault. Needs sensible
        initialization? if (websocket != nullptr) { websocket->close(); delete
        websocket;
        }
        */
    }
};

} // namespace advland

#endif
