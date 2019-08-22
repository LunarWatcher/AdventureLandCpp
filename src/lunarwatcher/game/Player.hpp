#ifndef LUNARWATCHER_ADVLAND_PLAYER
#define LUNARWATCHER_ADVLAND_PLAYER

#include "PlayerSkeleton.hpp"
#include "lunarwatcher/net/SocketWrapper.hpp"
#include <type_traits>
#include <nlohmann/json.hpp>

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
    // TODO: variables, standard functions
    
    bool mHasStarted = false;
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
    Player(std::string name, std::string uid, std::string fullUrl, AdvLandClient& client, PlayerSkeleton& skeleton);
    
    
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
#if USE_STATIC_ENTITIES
    static 
#endif
    std::map<std::string, nlohmann::json>& getEntities() { 
#if USE_STATIC_ENTITIES
        return SocketWrapper::getEntities();
#else
        return wrapper.getEntities();
#endif
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

    const nlohmann::json& getInventory() {
        return data["items"];
    }
    
    friend class PlayerSkeleton;
};
#undef PROXY_GETTER
} // namespace advland

#endif
