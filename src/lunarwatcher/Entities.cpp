#include "game/Player.hpp"
#include "net/SocketWrapper.hpp"
#include "AdvLand.hpp"

namespace advland {

Player::Player(std::string uid, std::string fullUrl, AdvLandClient& client, PlayerSkeleton& skeleton)
        :  wrapper(uid, fullUrl, client), client(client), skeleton(skeleton), characterId(uid) {
}

Player::Player(const Player& p) : wrapper(p.wrapper), client(p.client), skeleton(p.skeleton), characterId(p.characterId){    
}

void Player::start() {
    wrapper.connect();        
}

SocketWrapper& Player::getSocket() { return wrapper; }
} // namespace advland
