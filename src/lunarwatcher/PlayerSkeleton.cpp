#include "game/PlayerSkeleton.hpp"
#include <chrono>
#include "game/Player.hpp"
#include "math/Logic.hpp"
#include "AdvLand.hpp"

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
                {"m", character->getMapId()} // TODO: find method that returns map index
            });
}

bool PlayerSkeleton::canWalk(const nlohmann::json& entity) {    
    if ((entity.is_object() && entity.size() == 0) || entity["rip"] == true || (entity.find("s") != entity.end() && entity["s"]["stunned"] == true)) {
        return false;
    }
    
    return true;
}

void PlayerSkeleton::useSkill(const std::string& ability) {
    if (ability == "use_hp" || ability == "hp")
        use("hp");
    else if (ability == "use_mp" || ability == "mp") 
        use("mp");
}

void PlayerSkeleton::use(const std::string& item) {
    
    for (const nlohmann::json& item : character->getInventory()) {
        if (item.find("name") != item.end()
                && item["name"] == item) {
            character->getSocket().emit("use", {{"item", item}});
            break;
        }
    }
}

bool PlayerSkeleton::canUse(const std::string& name) {
    auto& skills = character->getClient().getData()["skills"];
    
    // Some skills are mapped. 
    if (skills.find(name) != skills.end() 
            && skills[name].find("class") != skills[name].end()) {
        // Some of these define a character class that's able to use the skill.
        // If these don't match, the player can obviously not use the skill.
        auto& tmp = skills[name]["class"];
        if (std::find(tmp.begin(), tmp.end(), character->getCType()) == tmp.end()) {
            
            return false;
        }
    }    
    // Check time
    if (timers.find(name) == timers.end()) {
        timers[name] = {};
        return true;
    }

    Timer& t = timers[name];
    double time = t.check();
    double timeout = 0;
    if (name == "mp" || name == "hp")
        timeout = skills["use_" + name].value("cooldown", 2000);
    else
        timeout = skills[name].value("cooldown", 0);
    return time > timeout;
}

bool PlayerSkeleton::canAttack(nlohmann::json& entity) {
    if (!inAttackRange(entity)
            || attackTimer.check() < 1000.0 / entity.value("frequency", 1000.0))
        return false;
    return true;
}

bool PlayerSkeleton::inAttackRange(nlohmann::json& entity) {
    if (entity.find("x") == entity.end() || entity.find("y") == entity.end()) {
        mSkeletonLogger->warn("Attemped to call inAttackRange on positionless entity:\n{}", entity.dump()); 
        return false;
    }
    if (MovementMath::pythagoras(character->getX(), character->getY(), entity["x"], entity["y"]) > character->getRange()) 
        return false;
    return true;
}

void PlayerSkeleton::say(std::string message) {
    character->getSocket().emit("say", {{"message", message}});
}

void PlayerSkeleton::attack (nlohmann::json& entity) {
    if (!canAttack(entity)) return;
    attackTimer.reset();
    character->getSocket().emit("attack", {{"id", entity["id"]}});
}

nlohmann::json PlayerSkeleton::getNearestMonster(const nlohmann::json& attribs) {
    double closest = 999990;
    auto& entities = character->getEntities();
    if (entities.size() == 0) {
        return {};
    }
    nlohmann::json j;

    for (auto& [id, entity] : entities) {
        
        if (entity.value("type", "") != "monster"
        || (attribs.find("type") != attribs.end() && entity.value("mtype", "") != attribs["type"]) 
        || (attribs.find("min_xp") != attribs.end() && entity.value("xp", 0) < int(attribs["min_xp"]))
        || (attribs.find("max_att") != attribs.end() && entity.value("attack", 0) > int(attribs["attack"])))
            continue;

        double dist = MovementMath::pythagoras(character->getX(), character->getY(), double(entity["x"]), double(entity["y"]));
       
        if (dist < closest) {
            closest = dist; 
            j = entity;
        }
    } 
    return j;
}

nlohmann::json PlayerSkeleton::getTarget() {
    if (character->data.find("ctarget") != character->data.end()) {
        if(character->getEntities().find(character->data["ctarget"]) != character->getEntities().end()){
            nlohmann::json& json = character->getEntities()[character->data["ctarget"]];
            mSkeletonLogger->info("Dumping target info: {}", json.dump());
            if (json["dead"] == true || int(json["hp"]) <= 0) {
                return nullptr;
            }
        } else return nullptr;
    }
    return nullptr;
}

void PlayerSkeleton::changeTarget(const nlohmann::json& entity) {
    if (entity.is_null()) {
        character->data["ctarget"] = nullptr;
        sendTargetLogic(nullptr);
    } else if(entity.find("id") == entity.end()) 
        return;
    if (entity["id"] != lastIdSent) {
        character->data["ctarget"] = entity["id"]; 
        sendTargetLogic(entity);
    }
}

void PlayerSkeleton::sendTargetLogic(const nlohmann::json& target) {
    if (target.is_null()) {
        character->getSocket().emit("target", {{"id", ""}});
        lastIdSent = "";
    } else {
        character->getSocket().emit("target", {{"id", target["id"].get<std::string>()}});
        lastIdSent = target["id"];
    }
}

}
