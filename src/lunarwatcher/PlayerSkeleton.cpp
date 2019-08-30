#include "game/PlayerSkeleton.hpp"
#include "AdvLand.hpp"
#include "game/Player.hpp"
#include "math/Logic.hpp"
#include "utils/ParsingUtils.hpp"
#include <chrono>

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
    
    auto minXTarget = std::min(playerX, x);
    auto maxXTarget = std::max(playerX, x);

    auto minYTarget = std::min(playerY, y);
    auto maxYTarget = std::max(playerY, y);

    auto& xLines = geom["x_lines"]; 
    auto& yLines = geom["y_lines"]; 

    if (!trigger) {
        // Match the hitbox
        auto base = cJson["base"];
        std::vector<std::vector<int>> hitbox = {{-int(base["h"]), int(base["vn"])},
                                                {int(base["h"]), int(base["vn"])},
                                                {-int(base["h"]), -int(base["v"])},
                                                {-int(base["h"]), -int(base["v"])}};
        for (auto& i : hitbox) {
            int hitboxX = i[0];
            int hitboxY = i[1];
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

bool PlayerSkeleton::smartMove(const nlohmann::json& destination) {
    // The map to move to. Populated if one is specified.
    // Otherwise, the map defaults to the current map for x/y moves.
    std::string map;
    std::string to;
    
    double tx, ty;

    if (destination.is_string()) {
        to = destination.get<std::string>();
    } else {
        if (destination.find("x") != destination.end()) {
            tx = destination["x"].get<double>();
            ty = destination["y"].get<double>();
        } else if (destination.find("to") != destination.end()) {
            to = destination["to"].get<std::string>();
        }
        if (destination.find("map") != destination.end())
            map = destination["map"].get<std::string>();

    }
    if (to == "town") to = "main";
   
    // Structure helper:
    // Map
    auto& gameData = this->getGameData();
    // Map
    auto& monsters = gameData["monsters"];
    if (monsters.find(to) != monsters.end()) {
        // map 
        const nlohmann::json& maps = gameData["maps"];
        bool die = false;
        for (auto& [mapName, mapData] : maps.items()) {
            if (mapData.find("monsters") != mapData.end()) {
                // List of maps 
                auto& mapMonsters = mapData["monsters"];
                for (auto& pack : mapMonsters) {
                    if (pack["type"] != to
                            || mapData["ignore"] == true 
                            || mapData["instance"] == true) continue;
                     
                    // Some mobs, like phoenix and mvampire, use boundaries instead of bounday. 
                    if (pack.find("boundaries") != pack.end()) { 
                        // List of maps 
                        auto& boundaries = pack["boundaries"];   
                        if (cyclableSpawnCounts.find(to) == cyclableSpawnCounts.end()) 
                            cyclableSpawnCounts[to] = 0;
                        auto& boundary = boundaries[cyclableSpawnCounts[to]];
                        map = boundary[0].get<std::string>();
                        tx = (boundary[1].get<int>() + boundary[3].get<int>()) / 2.0;
                        ty = (boundary[2].get<int>() + boundary[4].get<int>()) / 2.0;
                        cyclableSpawnCounts[to] = (cyclableSpawnCounts[to] + 1) % boundaries.size();
                        die = true;
                        break;
                    } else if (pack.find("boundary") != pack.end()) {
                        auto& boundary = pack["boundary"];
                        map = mapName;
                        tx = (boundary[0].get<int>() + boundary[2].get<int>()) / 2.0;
                        ty = (boundary[1].get<int>() + boundary[3].get<int>()) / 2.0;
                        die = true;
                        break;
                    }

                }
            }
            if (die) break;
        }
    } else if(to == "upgrade" || to == "compound"){
        map = "main";
        tx = -204;
        ty = -129;
    } else if (to == "exchange") {
        map = "main";
        tx = -26;
        ty = -432;
    } else if(to == "potions") {
        std::string pMap = character->getMap();
        if (pMap == "halloween") {
            map = "halloween";
            tx = 149;
            ty = -182;
        } else if(pMap == "winterland" || pMap == "winter_inn" || pMap == "winter_cave") {
            map = "winter_inn";
            tx = -84;
            ty = -173;
        } else {
            map = "main";
            tx = 56;
            ty = -122;
        }
    } else if (to == "scrolls") {
        map = "main";
        tx = -465;
        ty = -71;
    }
    if (map == "") {
        mSkeletonLogger->error("Failed to recognize smart_move target: {}", destination.dump());
        return false;
    }
    smart.initSmartMove(map, tx, ty);
    return true;
}

bool PlayerSkeleton::smartMove(const nlohmann::json& destination, std::function<void()> callback) {
    if (smartMove(destination)) {
        this->smart.withCallback(callback);
        return true;
    }
    return false;
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
    auto& itemData = character->getClient().getData()["items"];
    for (unsigned long long i = 0; i < character->getInventory().size(); i++) {
        const nlohmann::json& iItem = character->getInventory()[i];
        if (iItem.is_null()) continue;
        auto& iItemData = itemData[iItem["name"].get<std::string>()];
        if (iItemData.find("gives") != iItemData.end()) {
            auto& gives = iItemData["gives"];
            for (auto& effect : gives) {
                if (effect[0] == item) {
                    character->getSocket().emit("equip", {{"num", i}}); 
                    return;
                }
            }
        }
    }
    character->getSocket().emit("use", {{"item", item}});
}


void PlayerSkeleton::equip(int inventoryIdx, std::string itemSlot) {
    character->getSocket().emit("equip", {{"num", inventoryIdx}, {"slot", itemSlot}});
}

void PlayerSkeleton::loot(bool safe) {
    int looted = 0;
    
    std::lock_guard<std::mutex> guard(getSocket().getChestGuard());
    if (lootTimer.check() < 300) 
        return;
 
    if (character->getSocket().getChests().size() == 0)
        return;
    for (auto& [id, chest] : character->getSocket().getChests()) { 
        if (safe && chest["items"].get<int>() != 0 && chest["items"].get<int>() > character->countOpenInventory()) continue;  
        
        character->getSocket().emit("open_chest", {{"id", id}});

        looted++;
        if (looted == 2)
            break;   
    }
    if (looted > 1) 
        lootTimer.reset();
}

void PlayerSkeleton::sendGold(std::string to, unsigned long long amount) {
    if (to == "") return;
    getSocket().emit("send", {{"name", to}, {"gold", amount}});
}

void PlayerSkeleton::sendItem(std::string to, int inventoryIdx, unsigned int count) {
    if (to == "") return;
    getSocket().emit("send", {{"name", to}, {"num", inventoryIdx}, {"q", count}});
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

void PlayerSkeleton::sendCm(const nlohmann::json& to, const nlohmann::json& message) {
    if (to.is_array() && to.size() == 0) {
        mSkeletonLogger->error("Cannot send a code message to no one.");
        return;
    }
    if (to.is_string() && character->getClient().isLocalPlayer(std::string(to))) {
        character->getClient().dispatchLocalCm(to, message, character->getName());
    } else if (to.is_array() && to.size() == 1 && character->getClient().isLocalPlayer(std::string(to[0]))) { 
        std::string charName = std::string(to[0]);
        if (character->getClient().isLocalPlayer(charName))
            character->getClient().dispatchLocalCm(charName, message, character->getName());
    }else {
        if (to.is_string())
            getSocket().emit("cm", {{"to", nlohmann::json::array({to.get<std::string>()})}, {"message", message}});
        else {
            if (!to.is_array()) {
                mSkeletonLogger->error("Attempted to call sendCm with the \"to\" parameter that isn't a string or an object. Ignoring.");
                return;
            }

            for (auto& username : to) {
                if (username.is_string()) {
                    if (character->getClient().isLocalPlayer(username)) {
                        character->getClient().dispatchLocalCm(username, message, character->getName());
                    } else {
                        getSocket().emit("cm", {{"to", nlohmann::json::array({username.get<std::string>()})}, {"message", message}});
                    }
                }
            }

        }
    }
}

void PlayerSkeleton::attack(nlohmann::json& entity) {
    if (!canAttack(entity)) return;
    attackTimer.reset();
    if (entity.is_object())
        character->getSocket().emit("attack", {{"id", entity["id"].get<std::string>()}});
    else 
        character->getSocket().emit("attack", {{"id", entity.get<std::string>()}});
}

void PlayerSkeleton::heal(nlohmann::json& entity) {
   if (!canAttack(entity)) return;
    attackTimer.reset();
    if (entity.is_object()) 
        character->getSocket().emit("heal", {{"id", entity["id"].get<std::string>()}});
    else 
        character->getSocket().emit("heal", {{"id", entity.get<std::string>()}});
}

void PlayerSkeleton::sendPartyInvite(std::string name, bool isRequest) {
    character->getSocket().emit("party", {
            {"event", isRequest ? "request" : "invite"},
            {"name", name}
        });
}

void PlayerSkeleton::sendPartyRequest(std::string name) {
    sendPartyInvite(name, true);
}

void PlayerSkeleton::acceptPartyInvite(std::string name) {
    character->getSocket().emit("party", {{"event", "accept"}, {"name", name}});
}

void PlayerSkeleton::acceptPartyRequest(std::string name) {
    character->getSocket().emit("party", {{"event", "raccept"}, {"name", name}});
}

void PlayerSkeleton::leaveParty() {
    character->getSocket().emit("party", {{"event", "leave"}});
}

void PlayerSkeleton::respawn() {
    character->getSocket().emit("respawn");
}

nlohmann::json PlayerSkeleton::getNearestMonster(const nlohmann::json& attribs) {
    double closest = 999990;
    auto& entities = character->getEntities();
    if (entities.size() == 0) {
        return {};
    }
    nlohmann::json j;

    for (auto& [id, entity] : entities) {
        if (entity.is_null()) continue;
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
            return json;
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

const GameData& PlayerSkeleton::getGameData() {
    return character->getClient().getData();
}
SocketWrapper& PlayerSkeleton::getSocket() { return character->wrapper; }

} // namespace advland
