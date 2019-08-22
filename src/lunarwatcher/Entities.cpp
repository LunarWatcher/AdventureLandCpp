#include "game/Player.hpp"
#include "net/SocketWrapper.hpp"
#include "AdvLand.hpp"
#include "lunarwatcher/math/Logic.hpp"
#include "game/PlayerSkeleton.hpp"

namespace advland {

#define PROXY_GETTER_IMPL(cls, name, capName, type) type cls::get##capName() { return data[name].get<type>(); }
#define PROXY_GETTER_JSON(cls, name, capName) const nlohmann::json cls::get##capName() { return data[name]; }
Player::Player(std::string name, std::string uid, std::string fullUrl, AdvLandClient& client, PlayerSkeleton& skeleton)
        : wrapper(uid, fullUrl, client, *this), client(client), skeleton(skeleton), name(name), characterId(uid), data(nlohmann::json::object()) {
}

void Player::start() {
    wrapper.connect();        
}

void Player::stop() {
    wrapper.close();
}

void Player::onConnect() {
    this->skeleton.onStart();
    mHasStarted = true;
}

void Player::beginMove(double tx, double ty){
    data["from_x"] = data["x"];
    data["from_y"] = data["y"];
    data["going_x"] = tx;
    data["going_y"] = ty;
    data["moving"] = true;
    auto vxy = MovementMath::calculateVelocity(data);
    data["vx"] = vxy.first;
    data["vy"] = vxy.second;
}

PROXY_GETTER_IMPL(Player, "x", X, double)
PROXY_GETTER_IMPL(Player, "y", Y, double)
PROXY_GETTER_IMPL(Player, "hp", Hp, int)
PROXY_GETTER_IMPL(Player, "max_hp", MaxHp, int)
PROXY_GETTER_IMPL(Player, "mp", Mp, int)
PROXY_GETTER_IMPL(Player, "max_mp", MaxMp, int)
PROXY_GETTER_IMPL(Player, "map", Map, std::string)
PROXY_GETTER_IMPL(Player, "m", MapId, int)
PROXY_GETTER_IMPL(Player, "range", Range, int)
PROXY_GETTER_IMPL(Player, "ctype", CType, std::string)
SocketWrapper& Player::getSocket() { return wrapper; }

const GameData& PlayerSkeleton::getGameData() {
    return character->getClient().getData();
}

#undef PROXY_GETTER_IMPL
} // namespace advland
