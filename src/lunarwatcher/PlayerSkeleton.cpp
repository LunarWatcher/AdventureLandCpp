#include "game/PlayerSkeleton.hpp"
#include <chrono>
#include "game/Player.hpp"

namespace advland {

bool PlayerSkeleton::canMove(double x, double y) {
    // TODO implement method
    return false;
}

void PlayerSkeleton::move(double x, double y) {
    character->beginMove(x, y);    
    character->getSocket().emit("move", 
            {
                {"x", character->getX()}, 
                {"y", character->getY()},
                {"going_x", x},
                {"going_y", y},
                {"m", /*character->getMap()*/ 0} // TODO: find method that returns map index
            });
}

bool PlayerSkeleton::canWalk(const nlohmann::json& entity) {
    
    if ((entity.is_object() && entity.size() == 0) || entity["rip"] == true || (entity.find("s") != entity.end() && entity["s"]["stunned"] == true)) {
        return false;
    }
    
    return true;
}

void PlayerSkeleton::say(std::string message) {
    character->getSocket().emit("say", {{"message", message}});
}

}
