#include "AdvLand.hpp"
#include "game/Player.hpp"
#include "game/PlayerSkeleton.hpp"
#include "lunarwatcher/math/Logic.hpp"
#include "net/SocketWrapper.hpp"

namespace advland {

#define PROXY_GETTER_IMPL(cls, name, capName, type)                                                                    \
    type cls::get##capName() { return data[name].get<type>(); }
Player::Player(std::string name, std::string uid, std::string fullUrl,
               const std::pair<std::string, std::string>& server, AdvLandClient& client, PlayerSkeleton& skeleton)
        : wrapper(uid, fullUrl, client, *this), client(client), skeleton(skeleton), name(name), characterId(uid),
          data(nlohmann::json::object()), server(server) {}

void Player::start() { wrapper.connect(); }

void Player::stop() {
    wrapper.close(); 
    this->skeleton.stopUVThread();
}

void Player::onConnect() {
    this->skeleton.onStart();
    mHasStarted = true;
    this->skeleton.startUVThread();
}

void Player::beginMove(double tx, double ty) {
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
PROXY_GETTER_IMPL(Player, "speed", Speed, int)
PROXY_GETTER_IMPL(Player, "gold", Gold, long long)
PROXY_GETTER_IMPL(Player, "id", Id, std::string)

SocketWrapper& Player::getSocket() { return wrapper; }

int Player::countOpenInventory() {
    int count = 0;
    for (auto& item : getInventory()) {
        if (item.is_null()) count++;
    }
    return count;
}

std::optional<Server*> Player::getServer() {

    std::string& cluster = server.first;
    std::string& identifier = server.second;

    Server* possible = client.getServerInCluster(cluster, identifier);
    if (possible != nullptr) return possible;

    return std::nullopt;
}

#undef PROXY_GETTER_IMPL
} // namespace advland
