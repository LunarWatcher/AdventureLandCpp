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
        
    void attack(nlohmann::json& entity);
    nlohmann::json getNearestMonster(const nlohmann::json& attribs);

    void useSkill(const std::string& ability);
    void use(const std::string& skill);
    void loot(bool safe = true/*, std::string sendChestIdsTo=""*/);

    nlohmann::json getTarget();
    void changeTarget(const nlohmann::json& entity);
    void sendTargetLogic(const nlohmann::json& entity);

    const GameData& getGameData();
        
};

}

#endif 
