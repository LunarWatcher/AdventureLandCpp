#ifndef LUNARWATCHER_ADVLAND_PLAYER
#define LUNARWATCHER_ADVLAND_PLAYER

#include "lunarwatcher/net/SocketWrapper.hpp"
#include <type_traits>
#include <nlohmann/json.hpp>
#include "lunarwatcher/objects/Server.hpp"

namespace advland {

#define PROXY_GETTER(capName, type) type get##capName(); 

class PlayerSkeleton;
class AdvLandClient;

class Player {
private:
    SocketWrapper wrapper;
    AdvLandClient& client;
    PlayerSkeleton& skeleton;
    std::string name;
    std::string characterId;
    nlohmann::json data;
    nlohmann::json party;

    bool mHasStarted = false;
    // cluster, identifier 
    std::pair<std::string, std::string> server; 

public:
    /**
     * Creates a new player.
     *
     * @param name     The player name.
     * @param uid      The ID of the character. Used to connect
     * @param fullUrl  The server IP and port to connect to.
     * @param client   The client associated with the account.
     * @param skeleton Contains the user-defined code.
     */
    Player(std::string name, std::string uid, std::string fullUrl, const std::pair<std::string, std::string>& server, AdvLandClient& client, PlayerSkeleton& skeleton);
    
    
    void start();
    void stop();

    void updateJson(const nlohmann::json& json) {
        this->data.update(json);
    }    
    nlohmann::json& getRawJson() { return data; }
    std::string getUid() { return characterId; }
    std::string getUsername() { return name; }

    bool isMoving() { 
        return data.value("moving", false);
    }
    bool isAlive() { return data.value("rip", false) == false; }
    bool hasStarted() { return mHasStarted; }
    std::map<std::string, nlohmann::json>& getEntities() { 
        return wrapper.getEntities();
    }

    void beginMove(double tx, double ty);

    AdvLandClient& getClient() { return client; }
    PlayerSkeleton& getSkeleton() { return skeleton; }
    SocketWrapper& getSocket();
    
    void onConnect();
    // Proxies
    PROXY_GETTER(X, double)
    PROXY_GETTER(Y, double)
    PROXY_GETTER(Hp, int)
    PROXY_GETTER(MaxHp, int)
    PROXY_GETTER(Mp, int)
    PROXY_GETTER(MaxMp, int)
    PROXY_GETTER(Map, std::string) 
    PROXY_GETTER(MapId, int)
    PROXY_GETTER(Range, int)
    PROXY_GETTER(CType, std::string)
    PROXY_GETTER(Speed, int)   
    PROXY_GETTER(Gold, long long)
    PROXY_GETTER(Id, std::string)

    const std::string& getName() { return name; }
    const nlohmann::json& getInventory() {
        return data["items"];
    }
    int countOpenInventory();
   
    /**
     * Updates the party. This function should NEVER be called from bot code - this is used to update
     * the party data by using the party_update event.
     */
    void setParty(const nlohmann::json& j) {
        party.update(j);
    }

    const nlohmann::json& getParty() { return party; }

    std::optional<Server*> getServer();

    friend class PlayerSkeleton;
};
#undef PROXY_GETTER
} // namespace advland

#endif
