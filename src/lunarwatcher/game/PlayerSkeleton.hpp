#ifndef LUNARWATCHER_ADVLAND_PLAYERSKELETON
#define LUNARWATCHER_ADVLAND_PLAYERSKELETON

#include "nlohmann/json.hpp"
#include <string>
#include <memory>
#include <thread>
#include <vector>
#include <functional>

namespace advland {

class Player;
class PlayerSkeleton {
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
    bool canMove(double x, double y);
    bool canWalk(const nlohmann::json& entity);
    void move(double x, double y);
    void say(std::string message);
    // TODO partySay (message: msg, party: true)
    // TODO pmSay (message: msg, name: user)
};

}

#endif 
