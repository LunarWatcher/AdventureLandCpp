#include "AdvLand.hpp"
#include "meta/Exceptions.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

namespace advland {

AdvLandClient::AdvLandClient(std::string email, std::string password)
        : session("adventure.land", 443,
                  new Context(Context::CLIENT_USE, "", "", "", Context::VERIFY_NONE, 9, false,
                              "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH")) {
    login(email, password);
    collectGameData();
    collectCharactersAndServers();
}

void AdvLandClient::login(std::string& email, std::string& password) {

    std::stringstream indexStream;
    HTTPRequest connect(HTTPRequest::HTTP_GET, "/", HTTPMessage::HTTP_1_1);
    session.sendRequest(connect);
    HTTPResponse indexResponse;
    session.receiveResponse(indexResponse);

    int result = indexResponse.getStatus();

    if (result != 200) {
        throw LoginException("Failed to load index. Received code: " + std::to_string(result));
    }
    mLogger->debug("Connection established. Continuing with login");

    // Prepares the request
    HTTPRequest req(HTTPRequest::HTTP_POST, "/api/signup_or_login", HTTPMessage::HTTP_1_1);
    HTMLForm form;
    form.setEncoding(HTMLForm::ENCODING_MULTIPART);
    form.set("method", "signup_or_login");
    form.set("arguments", "{\"email\":\"" + email + "\", \"password\": \"" + password + "\", \"only_login\": true}");
    form.prepareSubmit(req);

    form.write(session.sendRequest(req));

    HTTPResponse loginResponse;
    std::istream& loginStream = session.receiveResponse(loginResponse);
    std::stringstream jsonStream;
    Poco::StreamCopier::copyStream(loginStream, jsonStream);

    // Check for base login failure
    std::string rawJson = jsonStream.str();
    if (rawJson.find("\"type\": \"ui_error\"") != std::string::npos) {
        throw LoginException(rawJson);
    }

    // Get the auth token
    std::vector<HTTPCookie> cookies;

    try {
        loginResponse.getCookies(cookies);
    } catch (...) {
        // These types of catch statements aren't encouraged, but the only documented reason
        // this would throw is if set-cookies is malformed. This is a bug I can't fix anyway (if it happens,
        // it's on the AL developers)
        throw LoginException("Caught an exception from getCookies() - the set-cookies header is probably malformed");
    }
    for (auto cookie : cookies) {
        if (cookie.getName() == "auth") {
            mLogger->info("Login succeeded");
            this->authToken = cookie.getValue();
            return;
        }
    }

    throw LoginException("Failed to find authentication key");
}

void AdvLandClient::collectGameData() {
    mLogger->info("Fetching game data...");
    std::stringstream result;

    HTTPRequest request(HTTPRequest::HTTP_GET, "/data.js", HTTPMessage::HTTP_1_1);
    HTTPResponse response;

    session.sendRequest(request);
    std::istream& res = session.receiveResponse(response);

    if (response.getStatus() != 200) throw EndpointException("Failed to GET data.js");
    Poco::StreamCopier::copyStream(res, result);

    std::string rawJson(result.str());
    rawJson = rawJson.substr(6, rawJson.length() - 8);
    this->data = GameData(rawJson);
    mLogger->info("Game data collected.");
}

void AdvLandClient::collectCharactersAndServers() {
    mLogger->info("Collecting characters and servers...");
    std::stringstream result;
    HTTPRequest request(HTTPRequest::HTTP_POST, "/api/servers_and_characters", HTTPMessage::HTTP_1_1);
    NameValueCollection collection;
    collection.add("auth", this->authToken);
    request.setCookies(collection);
    HTTPResponse response;
    HTMLForm form;
    form.setEncoding(HTMLForm::ENCODING_MULTIPART);
    form.set("method", "servers_and_characters");
    form.set("arguments", "{}");
    form.prepareSubmit(request);

    form.write(session.sendRequest(request));

    std::istream& rawStream = session.receiveResponse(response);
    std::stringstream resultStream;
    Poco::StreamCopier::copyStream(rawStream, resultStream);
    std::string rawData = resultStream.str();
    mLogger->info(rawData); 
    if (response.getStatus() != 200 || rawData.find("\"args\": [\"Not logged in.\"]") != std::string::npos) {
        throw EndpointException("Failed to retrieve characters and servers: " + std::to_string(response.getStatus()) + "\n" + resultStream.str());
    }
        
    // TODO parse and store the info
    mLogger->info("Successfully collected characters and servers.");
    
}

} // namespace advland
