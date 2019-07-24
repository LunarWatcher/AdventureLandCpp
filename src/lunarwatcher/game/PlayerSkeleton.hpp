#ifndef LUNARWATCHER_ADVLAND_PLAYERSKELETON
#define LUNARWATCHER_ADVLAND_PLAYERSKELETON

#include <string>
#include <memory>

namespace advland {

class Player;
class PlayerSkeleton {
public:


    /**
     * Invoked when the socket has connected for the first time. 
     *
     * The single argument passed is the character,
     * which contains an accessible SocketWrapper instance, which can be used to
     * register for various events. All of the events can be registered for.
     *
     * The player is passed as a pointer to avoid copies, as well as enable null
     * initialization.
     */
    virtual void onStart(std::shared_ptr<Player> character) = 0;
     
};

}

#endif 
