#include "game/PlayerSkeleton.hpp"
#include "AdvLand.hpp"
#include "game/Player.hpp"
#include "math/Logic.hpp"
#include "utils//ParsingUtils.hpp"
#include <chrono>
#include <math.h>

namespace advland {

int PlayerSkeleton::searchGeometry(const nlohmann::json& lines, int minMoveVal) {
    int returnValue = 0;
    int length = lines.size() - 1;
    int d;
    while (returnValue < length - 1) {
        d = (returnValue + length) / 2;
        if (lines[d][0] < minMoveVal) {
            returnValue = d;
        } else
            length = d - 1;
    }
    return returnValue;
}

bool PlayerSkeleton::canMove(double x, double y, int px, int py, bool trigger) {
    // TODO implement method
    auto& geom = character->getClient().getData()["geometry"][character->getMap()]; 
    auto& cJson = character->getRawJson();
    double playerX = character->getX();
    double playerY = character->getY();

    if (trigger) {
        playerX += px;
        playerY += py;
    }
    
    auto minXTarget = std::min(playerX, x); // k
    auto maxXTarget = std::max(playerX, x); // j

    auto minYTarget = std::min(playerY, y); // h
    auto maxYTarget = std::max(playerY, y); // g

    auto& xLines = geom["x_lines"]; // bsearch_start a
    auto& yLines = geom["y_lines"]; // bsearch_start a

    if (!trigger) {
        // Match the hitbox
        auto base = cJson["base"];
        std::vector<std::vector<int>> hitbox = {{-int(base["h"]), int(base["vn"])},
                                                {int(base["h"]), int(base["vn"])},
                                                {-int(base["h"]), -int(base["v"])},
                                                {-int(base["h"]), -int(base["v"])}};
        for (auto& i : hitbox) {
            int hitboxX = i[0]; // z
            int hitboxY = i[1]; // y
            int targetX = x + hitboxX;
            int targetY = y + hitboxY;
            if(!canMove(targetX, targetY, hitboxX, hitboxY, true)) {
                return false;
            }
        }

        double pH = double(base["h"]);
        double nH = -pH;

        double vn = double(base["vn"]);
        double v = -double(base["v"]);

        if (x > playerX) { 
            pH = -pH;
            nH = -nH;
        }
        if (y > playerY) {
            double tmp = vn;
            vn = -v;
            v = -tmp;
        }

        if (!canMove(x + nH, y + v, nH, vn, true)
                || !canMove(x + pH, y + v, pH, v, true)) {
            return false;
        }

        return true;
    }

    for (unsigned long long i = searchGeometry(xLines, minXTarget); i < xLines.size(); i++) {
        auto& line = xLines[i];
        if (line[0] == x && ((line[1] <= y && line[2] >= y) ||
                             (line[0] == playerX && line[1] >= playerY && y > line[0]))) {
            return false;
        }

        if (minXTarget > line[0]) continue;
        if (maxXTarget < line[0]) break;

        double q = playerY +
                   (y - playerY) * (double(line[0]) - playerX) / (x - playerX + 1e-8);
        if (!(double(line[1]) - 1e-8 <= q && q <= double(line[2]) + 1e-8)) {
            continue;
        }
        return false;
    }

    for (unsigned long long i = searchGeometry(yLines, minYTarget); i < yLines.size(); i++) {
        auto& line = yLines[i];
        if (line[0] == y && ((line[1] <= x && line[2] >= x) ||
                             (line[0] == playerY && playerX <= line[1] && x >= line[1]))) {
            return false;
        }
        if (minYTarget > line[0]) continue;
        if (maxYTarget < line[0]) break;

        double q = playerX +
                   (x - playerX) * (double(line[0]) - playerY) / (x - playerY + 1e-8);
        if (!(double(line[1]) - 1e-8 <= q && q <= double(line[2]) + 1e-8)) continue;

        return false;
    }

    return true;
}

void PlayerSkeleton::move(double x, double y) {
    if(!canMove(x, y)) return;
    /*
     * Note to bot devs: checking if it's possible to move is required. There's no
     * server-sided position checking, which means raw socket calls can be used to 
     * move to invalid positions, such as at the top of a mountain. 
     * Doing so can result in the character ending up in jail (I think - don't 
     * quote me on that).
     */
    character->beginMove(x, y);
    character->getSocket().emit("move", {{"x", character->getX()},
                                         {"y", character->getY()},
                                         {"going_x", x},
                                         {"going_y", y},
                                         {"m", character->getMapId()}});
}

bool PlayerSkeleton::canWalk(const nlohmann::json& entity) {
    if ((entity.is_object() && entity.size() == 0) || entity["rip"] == true ||
        (entity.find("s") != entity.end() && entity["s"]["stunned"] == true)) {
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
    bool consumableItem = false;
    auto& itemData = character->getClient().getData()["items"];
    for (unsigned long long i = 0; i < character->getInventory().size(); i++) {
        const nlohmann::json& iItem = character->getInventory()[i];
        if (iItem.is_null()) continue;
        auto& iItemData = itemData[iItem["name"].get<std::string>()];
        if (iItemData.find("gives") != iItemData.end()) {
            auto& gives = iItemData["gives"];
            for (auto& effect : gives) {
                if (effect[0] == item) {
                    consumableItem = true;
                    character->getSocket().emit("equip", {{"num", i}}); 
                    return;
                }
            }
        }
    }

    if (!consumableItem) {
        character->getSocket().emit("use", {{"item", item}});
    }
}

void PlayerSkeleton::loot(bool safe) {
    int looted = 0;
    if (lootTimer.check() < 300) 
        return;

    if (safe && character->countOpenInventory() == 0)
        return;
    if (character->getSocket().getChests().size() == 0)
        return;
    for (auto& [id, chest] : character->getSocket().getChests()) {
        if (safe && chest["items"].get<int>() > character->countOpenInventory()) continue;  
        
        character->getSocket().emit("open_chest", {{"id", id}});

        looted++;
        if (looted == 2)
            return;
    }
    if (looted > 1) 
        lootTimer.reset();
}

bool PlayerSkeleton::canUse(const std::string& name) {
    auto& skills = character->getClient().getData()["skills"];

    // Some skills are mapped.
    if (skills.find(name) != skills.end() && skills[name].find("class") != skills[name].end()) {
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
    if (!inAttackRange(entity) || !(attackTimer.check() > 1000.0 / character->getRawJson().value("frequency", 1000.0)))
        return false;
    return true;
}

bool PlayerSkeleton::inAttackRange(nlohmann::json& entity) {
    if (entity.find("x") == entity.end() || entity.find("y") == entity.end()) {
        mSkeletonLogger->warn("Attemped to call inAttackRange on positionless entity:\n{}", entity.dump());
        return false;
    }
    if (MovementMath::pythagoras(character->getX(), character->getY(), entity["x"], entity["y"]) >
        character->getRange())
        return false;
    return true;
}

void PlayerSkeleton::say(std::string message) { character->getSocket().emit("say", {{"message", message}}); }

void PlayerSkeleton::attack(nlohmann::json& entity) {
    if (!canAttack(entity)) return;
    attackTimer.reset();
    character->getSocket().emit("attack", {{"id", entity["id"]}});
}
void PlayerSkeleton::heal(nlohmann::json& entity) {
    if (!canAttack(entity)) return;
    attackTimer.reset();
    character->getSocket().emit("heal", {{"id", entity["id"]}});
}

nlohmann::json PlayerSkeleton::getNearestMonster(const nlohmann::json& attribs) {
    double closest = 999990;
    auto& entities = character->getEntities();
    if (entities.size() == 0) {
        return {};
    }
    nlohmann::json j;

    for (auto& [id, entity] : entities) {

        if (entity.value("type", "") != "monster" ||
            (attribs.find("type") != attribs.end() && entity.value("mtype", "") != attribs["type"]) ||
            (attribs.find("min_xp") != attribs.end() && entity.value("xp", 0) < int(attribs["min_xp"])) ||
            (attribs.find("max_att") != attribs.end() && entity.value("attack", 0) > int(attribs["attack"])))
            continue;

        double dist =
            MovementMath::pythagoras(character->getX(), character->getY(), double(entity["x"]), double(entity["y"]));

        if (dist < closest) {
            closest = dist;
            j = entity;
        }
    }
    return j;
}

nlohmann::json PlayerSkeleton::getTarget() {
    if (character->data.find("ctarget") != character->data.end()) {
        if (character->getEntities().find(character->data["ctarget"]) != character->getEntities().end()) {
            nlohmann::json& json = character->getEntities()[character->data["ctarget"]];
            if (getOrElse(json, "dead", false) || int(json["hp"]) <= 0) {
                return nullptr;
            }
        } else
            return nullptr;
    }
    return nullptr;
}

void PlayerSkeleton::changeTarget(const nlohmann::json& entity) {
    if (entity.is_null()) {
        character->data["ctarget"] = nullptr;
        sendTargetLogic(nullptr);
    } else if (entity.find("id") == entity.end())
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

} // namespace advland
