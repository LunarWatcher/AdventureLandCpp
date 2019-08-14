#ifndef LUNARWATCHER_ADVLAND_PLAYER
#define LUNARWATCHER_ADVLAND_PLAYER

#include "PlayerSkeleton.hpp"
#include "lunarwatcher/net/SocketWrapper.hpp"
#include <type_traits>

namespace advland {
class PlayerSkeleton;
class AdvLandClient;

class Player {
private:
    SocketWrapper wrapper;
    AdvLandClient& client;
    PlayerSkeleton& skeleton;

    std::string characterId;

    // TODO: variables, standard functions

public:
    /**
     * Creates a new player.
     *
     * @param name     The player name. Used to pass the right variables to the socket connection.
     * @param fullUrl  The server IP and port to connect to.
     */
    Player(std::string uid, std::string fullUrl, AdvLandClient& client, PlayerSkeleton& skeleton);
    
    SocketWrapper& getSocket();
    void start();
};

} // namespace advland

#endif
