#ifndef LUNARWATCHER_ADVLAND_PLAYERSKELETON
#define LUNARWATCHER_ADVLAND_PLAYERSKELETON

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include "nlohmann/json.hpp"
#include <string>
#include <memory>
#include <thread>
#include <vector>
#include <functional>
#include <chrono>
#include "lunarwatcher/utils/Timer.hpp"
#include <map>
#include "lunarwatcher/objects/GameData.hpp"
#include "lunarwatcher/net/SocketWrapper.hpp"

namespace advland {

class Player;
class PlayerSkeleton {
private:
    static auto inline const mSkeletonLogger = spdlog::stdout_color_mt("PlayerSkeleton"); 
    // "Normal" timers
    Timer attackTimer;
    Timer lootTimer;

    // Skill timers (change frequently enough to warrant the use of a map)
    std::map<std::string, Timer> timers;
    std::string lastIdSent;

    int searchGeometry(const nlohmann::json& lines, int minMoveVal);
protected:
    std::shared_ptr<Player> character;
public:
    void injectPlayer(std::shared_ptr<Player> character) {
        this->character = character; 
    }

    /**
     * Invoked when the socket has connected for the first time. 
     * This is where you can create your own flow, adding timeouts,
     * threads, or anything else. 
     *
     * 
     */
    virtual void onStart() = 0;
    
    /**
     * Invoked when the socket disconnects.
     */
    virtual void onStop() = 0;

    // Party 
    virtual void onPartyRequest(std::string /* name */) {}  
    virtual void onPartyInvite(std::string /* name */) {}
    
    virtual void onFriendRequest() {}
    virtual void onCm(std::string /* name */, const nlohmann::json& /* data */) {}
    
    // API functions
    bool canMove(double x, double y, int px = 0, int py = 0, bool trigger=false);
    bool canWalk(const nlohmann::json& entity);
    bool canAttack(nlohmann::json& entity);
    bool canUse(const std::string& skill);
    bool inAttackRange(nlohmann::json& entity);
     
    void move(double x, double y);
    void say(std::string message);
    // TODO partySay (message: msg, party: true)
    // TODO pmSay (message: msg, name: user)
   
    void sendPartyInvite(std::string name, bool isRequest = false);
    void sendPartyRequest(std::string name);
    void acceptPartyInvite(std::string name);
    void acceptPartyRequest(std::string name);
    void leaveParty();

    /**
     * Sends a CM (Code Message). Code messages to other characters run on this client are 
     * sent locally, and not through the AL server.
     * Note that this has a high code cost, and may trigger disconnection. 
     *
     * Strings can also be passed directly to the message argument - it doesn't have to be 
     * a full JSON object. Strings, as well as numbers, are actually considered JSON objects.
     */
    void sendCm(const nlohmann::json& to, const nlohmann::json& message);

    /**
     * @param entity  The entity to attack, or alternatively the ID of an entity or the 
     *                username of a player
     */
    void attack(nlohmann::json& entity);

    /**
     * @param entity  Similar to attack, the entity to heal, or the ID of an entity or 
     *                the username of a player
     */
    void heal(nlohmann::json& entity);
    void respawn();
    nlohmann::json getNearestMonster(const nlohmann::json& attribs);

    void useSkill(const std::string& ability);
    void use(const std::string& skill);
    void loot(bool safe = true/*, std::string sendChestIdsTo=""*/);

    void sendGold(std::string to, unsigned long long amount);
    void sendItem(std::string to, int itemIdx, unsigned int count = 1);

    /**
     * Equips an item from the specified inventory index. 
     * The itemSlot parameter is used when specifying which 
     * character slot to equip the item to.
     */
    void equip(int inventoryIdx, std::string itemSlot = "");
    
    nlohmann::json getPlayer(std::string name);
    nlohmann::json getTarget();
    
    void changeTarget(const nlohmann::json& entity);
    void sendTargetLogic(const nlohmann::json& entity);

    const GameData& getGameData();
    SocketWrapper& getSocket();
        
};

}

#endif 
