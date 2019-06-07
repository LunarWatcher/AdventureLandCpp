#ifndef LUNARWATCHER_ADVLAND
#define LUNARWATCHER_ADVLAND

#include "Poco/Exception.h"
#include "Poco/Net/Context.h"
#include "Poco/Net/HTTPCredentials.h"
#include "Poco/Net/HTTPMessage.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/HTTPSClientSession.h"
#include "Poco/Net/Socket.h"
#include "Poco/Net/WebSocket.h"
#include "Poco/StreamCopier.h"
#include "nlohmann/json.hpp"

#include <iostream>
#include <istream>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string>
#include <sstream>

namespace advland {

using namespace Poco;
using namespace Poco::Net;

class AdvLandClient {
private:
    static auto inline const mLogger = spdlog::stdout_color_mt("AdvLandClient");
    HTTPSClientSession session;
    WebSocket* websocket;

    void login(std::string& email, std::string& password);

public:
    AdvLandClient(std::string email, std::string password);

    virtual ~AdvLandClient() {
        /* TODO - requires WS initialization to avoid segfault. Needs sensible
        initialization? if (websocket != nullptr) { websocket->close(); delete
        websocket;
        }
        */
    }

    /**
     * Executes a request. This doesn't emit websocket events.
     *
     * @param endpoint   The path to send the request to
     * @param method     The method to use - i.e. POST, GET, etc.
     *                   See Poco::Net::HTTPRequest
     * @param out        The stream to output the response into. 
     */
    void request(std::string endpoint, const std::string& method, std::stringstream& out);
};

} // namespace advland

#endif
