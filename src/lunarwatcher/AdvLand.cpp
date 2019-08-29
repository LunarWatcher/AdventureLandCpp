#include "AdvLand.hpp"
#include "math/Logic.hpp"
#include "meta/Exceptions.hpp"
#include "meta/Typedefs.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include <regex>
#include "utils/ParsingUtils.hpp"
#include <fstream>

namespace advland {
bool AdvLandClient::running = true;

AdvLandClient::AdvLandClient() : AdvLandClient("credentials.json") {}

AdvLandClient::AdvLandClient(const std::string& credentialFileLocation) : session("adventure.land", 443,
                  new Context(Context::CLIENT_USE, "", "", "", Context::VERIFY_NONE, 9, false,
                              "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH")){
    std::ifstream creds(credentialFileLocation);
    if (!creds) {
        mLogger->error("Failed to find credentials.json. To pass the email and password directly, please use AdvLandClient(std::string, std::string).");
        mLogger->error("If you intended to use this function, make sure the file exists in the current working directory. If it does exist, make sure the permission are correct.");
        throw "IO failure.";
    }
    nlohmann::json tmp;
    creds >> tmp;
    construct(tmp["email"], tmp["password"]); 
}
AdvLandClient::AdvLandClient(const nlohmann::json& email, const nlohmann::json& password)
        : session("adventure.land", 443,
                  new Context(Context::CLIENT_USE, "", "", "", Context::VERIFY_NONE, 9, false,
                              "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH")) {
    construct(email, password); 
}

void AdvLandClient::construct(const nlohmann::json& email, const nlohmann::json& password) {
    ix::initNetSystem();
    login(email.get<std::string>(), password.get<std::string>());
    collectGameData();
    collectCharacters();
    validateSession();
    collectServers();
}

AdvLandClient::~AdvLandClient() { ix::uninitNetSystem(); }

void AdvLandClient::login(const std::string& email, const std::string& password) {

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
    for (auto& cookie : cookies) {
        if (cookie.getName() == "auth") {
            mLogger->info("Login succeeded");
            this->sessionCookie = cookie.getValue();
            this->userId = this->sessionCookie.substr(0, this->sessionCookie.find("-"));
            return;
        }
    }

    throw LoginException("Failed to find authentication key");
}

void AdvLandClient::validateSession() {
    HTTPRequest request(HTTPRequest::HTTP_GET, "/", HTTPMessage::HTTP_1_1);
    HTTPResponse response;

    NameValueCollection collection;
    collection.add("auth", this->sessionCookie);
    request.setCookies(collection);

    session.sendRequest(request);
    std::istream& s = session.receiveResponse(response);
    std::stringstream index;
    Poco::StreamCopier::copyStream(s, index);

    std::string haystack = index.str();

    std::regex regex("user_auth=\"([a-zA-Z0-9]+)\"");
    std::smatch match;

    std::regex_search(haystack, match, regex);

    this->userAuth = match[1];

    if (userAuth == "") throw LoginException("Failed to retrieve user authentication token");
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

void AdvLandClient::collectCharacters() {
    mLogger->info("Collecting characters...");
    std::stringstream result;
    HTTPRequest request(HTTPRequest::HTTP_POST, "/api/servers_and_characters", HTTPMessage::HTTP_1_1);
    NameValueCollection collection;
    collection.add("auth", this->sessionCookie);
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
    if (response.getStatus() != 200 || rawData.find("\"args\": [\"Not logged in.\"]") != std::string::npos) {
        throw EndpointException("Failed to retrieve characters and servers (login issue): " +
                                std::to_string(response.getStatus()) + "\n" + resultStream.str());
    }

    nlohmann::json data = nlohmann::json::parse(rawData);
    if (data.size() > 0 && data[0].find("characters") != data[0].end()) {
        parseCharacters(data[0]["characters"]);
    } else {
        throw EndpointException("Failed to find characters in response: " + rawData);
    }

    for (std::pair<std::string, std::string> p : this->characters) {
        mLogger->info("User {}: {}", p.first, p.second);
    }

    mLogger->info("Successfully collected characters.");
}

void AdvLandClient::collectServers() {
    std::stringstream result;
    HTTPResponse response;

    this->postRequest(result, response, "get_servers", "{}", true);
    std::string rawData = result.str();

    if (response.getStatus() != 200 || rawData.find("\"args\": [\"Not logged in.\"]") != std::string::npos) {
        throw EndpointException("Failed to retrieve characters and servers (login issue): " +
                                std::to_string(response.getStatus()) + "\n" + rawData);
    } else if (rawData.length() == 0) {
        throw EndpointException("Didn't receive any data?");
    }

    nlohmann::json data = nlohmann::json::parse(rawData);
    for (auto& server : data[0]["message"]) {
        std::string ip = server["ip"];
        std::string clusterIdentifier = server["region"];
        std::string serverIdentifier = server["name"];
        std::string gameplay = server["gameplay"];
        int port = server["port"];
        bool pvp = false;
        // Some of the responses are empty strings - the rest are non-string bools
        if (server["pvp"].is_boolean()) {
            pvp = server["pvp"];
        }

        Server parsedServer(serverIdentifier, ip, port, pvp, gameplay);

        for (ServerCluster& cluster : this->serverClusters) {
            if (cluster.getRegion() == clusterIdentifier) {
                if (cluster.hasServer(serverIdentifier)) {
                    cluster.update(serverIdentifier, parsedServer);
                } else
                    cluster.addServer(parsedServer);

                goto hasServerCluster;
            }
        }
        {
            ServerCluster cluster(clusterIdentifier);
            cluster.addServer(parsedServer);
            this->serverClusters.push_back(cluster);
        }
    hasServerCluster:
        continue;
    }
}

void AdvLandClient::postRequest(std::stringstream& out, HTTPResponse& response, std::string apiEndpoint,
                                std::string arguments, bool auth, const std::vector<CookiePair>& formData) {
    HTTPRequest request(HTTPRequest::HTTP_POST, std::string("/api/") + apiEndpoint, HTTPMessage::HTTP_1_1);
    if (auth) {
        NameValueCollection cookies;
        cookies.add("auth", this->sessionCookie);
        request.setCookies(cookies);
    }

    HTMLForm form;
    form.setEncoding(HTMLForm::ENCODING_MULTIPART);
    // The endpoints that can use this method always sends a POST request with a form containing
    // a method key with a value matching the endpoint name
    form.set("method", apiEndpoint);
    form.set("arguments", arguments);
    if (formData.size() != 0) {
        for (CookiePair p : formData) {
            form.set(p.first, p.second);
        }
    }
    form.prepareSubmit(request);
    form.write(session.sendRequest(request));

    std::istream& rawStream = session.receiveResponse(response);
    Poco::StreamCopier::copyStream(rawStream, out);
}

void AdvLandClient::parseCharacters(nlohmann::json& data) {
    // data should be a list containing maps with character data at this point
    for (auto& node : data) {
        // So iterate and grab the necessary data
        std::string id = node["id"];
        std::string name = node["name"];
        std::pair<std::string, std::string> charPair = std::make_pair(id, name);
        if (this->characters.size() == 0 ||
            std::find(characters.begin(), characters.end(), charPair) == characters.end()) {
            this->characters.push_back(charPair);
        }
    }
}

ServerCluster* AdvLandClient::getServerCluster(std::string identifier) {
    for (ServerCluster& cluster : serverClusters) {
        if (cluster.getRegion() == identifier) return &cluster;
    }
    return nullptr;
}

Server* AdvLandClient::getServerInCluster(std::string clusterIdentifier, std::string serverIdentifier) {
    ServerCluster* cluster = getServerCluster(clusterIdentifier);
    if (!cluster) return nullptr;
    return cluster->getServerByName(serverIdentifier);
}

void AdvLandClient::processInternals() {
    auto last = std::chrono::high_resolution_clock::now();

    // Some things are required to be processed locally, such as updating the coordinates when moving.
    while (running) {
        auto now = std::chrono::high_resolution_clock::now(); 
        const double delta =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - last)
                .count();
        last = now;
        auto cDelta = delta;

        while (cDelta > 0) {

            for (auto& playerPtr : bots) {
                if (!playerPtr->hasStarted()) {
                    continue;
                }
                playerPtr->getSocket().deleteEntities();
                if (playerPtr->isAlive() && playerPtr->isMoving()) {
                    nlohmann::json& entity = playerPtr->getRawJson();
                    if (entity.find("ref_speed") == entity.end() || (entity.find("ref_speed") != entity.end() && entity["ref_speed"] != entity["speed"])) {
                        entity["ref_speed"] = entity["speed"];
                        entity["from_x"] = entity["x"];
                        entity["from_y"] = entity["y"];
                        std::pair<int, int> vxy = MovementMath::calculateVelocity(entity);
                        entity["vx"] = vxy.first;
                        entity["vy"] = vxy.second;
                        mLogger->info("Calculated vectors: {}, {}", vxy.first, vxy.second);
                        entity["engaged_move"] = entity["move_num"];
                    }
                    MovementMath::moveEntity(entity, cDelta);
                    MovementMath::stopLogic(entity);
                    
                }
                // Executed if entities are on a per-player basis.
                // This is the default behavior.
#if !USE_STATIC_ENTITIES
                for (auto& [id, entity] : playerPtr->getEntities()) {
                    if (entity.find("speed") == entity.end() && entity["type"] == "monster") {
                        std::string type = entity["mtype"];
                        entity["speed"] = this->getData()["monsters"][type]["speed"].get<double>();
                    }
                    if (!getOrElse(entity, "rip", false) && !getOrElse(entity, "dead", false) && getOrElse(entity, "moving", false)) {
                        if (entity.value("move_num", 0l) != entity.value("engaged_move", 0l) || (entity.find("ref_speed") != entity.end() && entity["ref_speed"] != entity["speed"])) {
                            entity["ref_speed"] = entity["speed"];
                            entity["from_x"] = entity["x"];
                            entity["from_y"] = entity["y"];
                            std::pair<int, int> vxy = MovementMath::calculateVelocity(entity);
                            entity["vx"] = vxy.first;
                            entity["vy"] = vxy.second;
                            
                            entity["engaged_move"] = entity["move_num"];
                        }

                        MovementMath::moveEntity(entity, cDelta);
                        MovementMath::stopLogic(entity); // Processes whether we're done moving or not.
                        
                    }
                    
                }
#endif
            }
            // Executed if entities are shared across all players.
#if USE_STATIC_ENTITIES
            for (auto& [id, entity] : SocketWrapper::getEntities()) {
                if (!getOrElse(entity, "rip", false) && !getOrElse(entity, "dead", false) && getOrElse(entity, "moving", false)) {
                    if (entity.find("speed") == entity.end() && entity["type"] == "monster") {
                        std::string type = entity["mtype"];
                        entity["speed"] = this->getData()["monsters"][type]["speed"].get<double>();
                        
                    }
                    if (entity.value("move_num", 0l) != entity.value("engaged_move", 0l) || (entity.find("ref_speed") != entity.end() && entity["ref_speed"] != entity["speed"])) {
                        entity["from_x"] = entity["x"];
                        entity["from_y"] = entity["y"];
                        if (entity.find("ref_speed") == entity.end())
                            entity["ref_speed"] = entity["speed"];
                        std::pair<int, int> vxy = MovementMath::calculateVelocity(entity);
                        entity["vx"] = vxy.first;
                        entity["vy"] = vxy.second;
                        
                        entity["engaged_move"] = entity["move_num"];
                    }

                    MovementMath::moveEntity(entity, cDelta);
                    MovementMath::stopLogic(entity); // Processes whether we're done moving or not.
                }
            }
#endif
            cDelta -= 50;
        }
        // Maintain 60 "FPS" (1000 milliseconds / 60 FPS is roughly 16.66667 ms between frames)
        std::this_thread::sleep_for(std::chrono::milliseconds(17));
    }
}

bool AdvLandClient::isLocalPlayer(std::string username) {
    for (auto& p : bots) {
        if (p->getName() == username) return true;
    }
    return false;
}

void AdvLandClient::dispatchLocalCm(std::string to, const nlohmann::json& message, std::string from) {
    for (auto& p : bots) {
        if (p->getName() == to) {
            p->getSocket().receiveLocalCm(from, message);
            return;
        }
    }
    mLogger->error("Failed to find {} in the local client.", to);
}

void AdvLandClient::kill() { 
    AdvLandClient::running = false; 
    for (auto& player : bots) player->stop();
}

bool AdvLandClient::canRun() { return running; }

} // namespace advland
