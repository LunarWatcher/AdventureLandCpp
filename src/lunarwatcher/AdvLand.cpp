#include "AdvLand.hpp"
#include "game/Player.hpp"
#include "game/PlayerSkeleton.hpp"
#include "math/Logic.hpp"
#include "meta/Exceptions.hpp"
#include "meta/Typedefs.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include "utils/ParsingUtils.hpp"
#include <fstream>
#include <regex>

namespace advland {
bool AdvLandClient::running = true;

AdvLandClient::AdvLandClient() : AdvLandClient("credentials.json") {}

AdvLandClient::AdvLandClient(const std::string& credentialFileLocation) {
    std::ifstream creds(credentialFileLocation);
    if (!creds) {
        mLogger->error("Failed to find credentials.json. To pass the email and password directly, please use "
                       "AdvLandClient(std::string, std::string). [NOT RECOMMENDED]");
        mLogger->error("If you intended to use this function, make sure the file exists in the current working "
                       "directory. If it does exist, make sure the permission are correct.");
        throw IOException("Failed to find credentials");
    }
    nlohmann::json tmp;
    creds >> tmp;
    construct(tmp["email"], tmp["password"]);
}
AdvLandClient::AdvLandClient(const nlohmann::json& email, const nlohmann::json& password) {
    construct(email, password);
}

void AdvLandClient::construct(const nlohmann::json& email, const nlohmann::json& password) {
    ix::initNetSystem();
    login(email.get<std::string>(), password.get<std::string>());
    collectGameData();
    collectCharacters();
    validateSession();
    collectServers();

    mapProcessor.processMaps(data);
}

AdvLandClient::~AdvLandClient() {
    ix::uninitNetSystem();
    runner.join();
}

void AdvLandClient::addPlayer(const std::string& name, Server& server, PlayerSkeleton& skeleton) {

    for (auto& [id, username] : characters) {
        if (username == name) {
            std::shared_ptr<Player> bot =
                std::make_shared<Player>(username, id, server.getIp() + ":" + std::to_string(server.getPort()),
                                         std::make_pair(server.getRegion(), server.getName()), *this, skeleton);
            this->bots.push_back(bot);
            skeleton.injectPlayer(bot); // Inject the player into the skeleton. Might be better to do in onConnect
            return;
        }
    }
}

void AdvLandClient::startBlocking() {
    for (auto& player : bots) {
        player->start();
    }
    processInternals();
}

void AdvLandClient::startAsync() {
    for (auto& player : bots) {
        player->start();
    }
    this->runner = std::thread(std::bind(&AdvLandClient::processInternals, this));
}

void AdvLandClient::login(const std::string& email, const std::string& password) {
    std::stringstream indexStream;

    auto r = ::advland::Get(cpr::Url{"https://adventure.land/"});

    if (r.error.message != "") {
        mLogger->error("Error received when attempting to connect: {}", r.error.message);
        throw LoginException("");
    }

    int result = r.status_code;
    if (result != 200) {
        mLogger->info("Failed to connect: {}", result);
        throw LoginException("Failed to load index. Received code: " + std::to_string(result));
    }

    mLogger->debug("Connection established. Continuing with login");

    // Prepares the request
    auto loginResponse = ::advland::Post(cpr::Url{"https://adventure.land/api/signup_or_login"},
                                         cpr::Payload{{"method", "signup_or_login"},
                                                      {"arguments", "{\"email\":\"" + email + "\", \"password\": \"" +
                                                                        password + "\", \"only_login\": true}"}});
    // Check for base login failure
    std::string& rawJson = loginResponse.text;
    if (rawJson.find("\"type\": \"ui_error\"") != std::string::npos) {
        throw LoginException(rawJson);
    }

    auto& cookies = loginResponse.cookies;
    auto& auth = cookies["auth"];
    if (auth != "") {
        mLogger->info("Login succeeded");
        this->sessionCookie = auth;
        this->userId = this->sessionCookie.substr(0, this->sessionCookie.find("-"));
        return;
    }

    throw LoginException("Failed to find authentication key");
}

void AdvLandClient::validateSession() {
    auto r = ::advland::Get(cpr::Url{"https://adventure.land/"}, cpr::Cookies{{"auth", this->sessionCookie}});

    std::string haystack = r.text;

    std::regex regex("user_auth=\"([a-zA-Z0-9]+)\"");
    std::smatch match;

    std::regex_search(haystack, match, regex);

    this->userAuth = match[1];

    if (userAuth == "") throw LoginException("Failed to retrieve user authentication token");
}

void AdvLandClient::collectGameData() {
    mLogger->info("Fetching game data...");
    auto r = ::advland::Get(cpr::Url{"https://adventure.land/data.js"});

    if (r.status_code != 200) throw EndpointException("Failed to GET data.js");

    std::string& rawJson = r.text;
    rawJson = rawJson.substr(6, rawJson.length() - 8);
    this->data = GameData(rawJson);
    mLogger->info("Game data collected.");
}

void AdvLandClient::collectCharacters() {
    mLogger->info("Collecting characters...");
    auto r = ::advland::Post(cpr::Url{"https://adventure.land/api/servers_and_characters"},
                             cpr::Cookies{{"auth", this->sessionCookie}},
                             cpr::Payload{{"method", "servers_and_characters"}, {"arguments", "{}"}});
    std::string& rawData = r.text;
    if (r.status_code != 200 || rawData.find("\"args\": [\"Not logged in.\"]") != std::string::npos) {
        throw EndpointException("Failed to retrieve characters and servers (login issue): " +
                                std::to_string(r.status_code) + "\n" + rawData);
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

    cpr::Response response = this->postRequest("get_servers", "{}", true);
    std::string& rawData = response.text;

    if (response.status_code != 200 || rawData.find("\"args\": [\"Not logged in.\"]") != std::string::npos) {
        throw EndpointException("Failed to retrieve characters and servers (login issue): " +
                                std::to_string(response.status_code) + "\n" + rawData);
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

        Server parsedServer(serverIdentifier, ip, port, pvp, gameplay, clusterIdentifier);

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

cpr::Response AdvLandClient::postRequest(std::string apiEndpoint, std::string arguments, bool auth,
                                         const cpr::Payload& formData) {
    cpr::Cookies cookies = {};
    if (auth) cookies = cpr::Cookies{{"auth", this->sessionCookie}};
    cpr::Payload primaryPayload = {{"method", apiEndpoint}, {"arguments", arguments}};

    auto r = ::advland::Post(cpr::Url{"https://adventure.land/api/" + apiEndpoint}, cookies, formData, primaryPayload);
    return r;
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
    using namespace std::chrono_literals;
    while (running) {
        std::this_thread::sleep_for(1s);
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
    for (auto& player : bots)
        player->stop();
}

bool AdvLandClient::canRun() { return running; }

} // namespace advland
