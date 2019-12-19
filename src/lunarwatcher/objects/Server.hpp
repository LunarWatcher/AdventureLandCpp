#ifndef LUNARWATCHER_ADVLAND_SERVER
#define LUNARWATCHER_ADVLAND_SERVER
#include "lunarwatcher/meta/Exceptions.hpp"
#include <algorithm>
#include <string>
#include <vector>
#include <iostream> // TODO: remove
namespace advland {

class Server {
private:
    std::string name;
    std::string ip;
    int port;
    bool pvp;
    std::string gameplay;
    std::string parentCluster;

public:
    Server(std::string name, std::string ip, int port, bool pvp, std::string gameplay, std::string parentCluster)
            : name(name), ip(ip), port(port), pvp(pvp), gameplay(gameplay), parentCluster(parentCluster) {}

    std::string& getName() { return name; }
    std::string& getIp() { return ip; }
    std::string& getGameplay() { return gameplay; }
    std::string& getRegion() { return parentCluster; }
    int getPort() { return port; }
    bool isPvp() { return pvp; }
};

class ServerCluster {
private:
    std::string name;
    std::vector<Server> servers;

public:
    ServerCluster(std::string name) : name(name) {}

    std::string& getRegion() { return name; }

    bool hasServer(std::string serverIdentifier) {
        for (Server& server : servers) {
            if (server.getName() == serverIdentifier) {
                return true;
            }
        }
        return false;
    }

    void update(std::string& identifier, Server server) {
        if (servers.size() == 0) throw IllegalArgumentException("Cannot update an empty vector");
        servers.erase(std::remove_if(servers.begin(), servers.end(), 
                [identifier](Server& server) { 
                    return server.getName() == identifier; 
                }), servers.end());
        this->servers.push_back(server);
    }

    void addServer(Server& server) { 
        servers.push_back(server); 
    }
    std::vector<Server>& getServers() { return servers; }
    Server* getServerByName(std::string name) {
        for (Server& server : servers) {
            if (server.getName() == name) 
                return &server;
        }
        return nullptr;
    }
};
} // namespace advland
#endif
