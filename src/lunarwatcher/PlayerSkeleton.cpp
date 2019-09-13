#include "game/PlayerSkeleton.hpp"
#include "AdvLand.hpp"
#include "game/Player.hpp"
#include "math/Logic.hpp"
#include "utils/ParsingUtils.hpp"
#include <chrono>

namespace advland {

bool PlayerSkeleton::canMove(double x, double y) {
    auto& geom = character->getClient().getData()["geometry"][character->getMap()];
    double playerX = character->getX();
    double playerY = character->getY();
    return character->getClient().getMapProcessor().canMove(playerX, playerY, x, y, geom);
}

void PlayerSkeleton::move(double x, double y) {
    if (!canMove(x, y)) return;
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

void PlayerSkeleton::initSmartMove() {
    killSwitch = false;
    if (smart.isSearching()) smart.stop(true);
    if (smartMoveThread.joinable()) smartMoveThread.join();
    smartMoveThread = std::thread{[this] { dijkstraProcessor(); }};
}

void PlayerSkeleton::dijkstraProcessor() {
    killSwitch = true;
    mSkeletonLogger->info("Searching...");
    character->getClient().getMapProcessor().dijkstra(*this, smart);
    if (!killSwitch) return;
    if (!smart.hasMore()) {
        smart.deinit();
        mSkeletonLogger->info("Path not found :/");
        return;
    }
    nlohmann::json target = nullptr;
    mSkeletonLogger->info("Path found!");
    while (killSwitch) {
        if (target.is_null() || (!getOrElse(target, "transport", false) &&
                                 character->getX() == target["x"].get<int>() && character->getY() == target["y"])) {
            if (!smart.hasMore()) break;
            target = smart.getAndRemoveFirst();
            if (getOrElse(target, "transport", false) == true) continue;
            move(target["x"].get<int>(), target["y"].get<int>());
        } else if (getOrElse(target, "transport", false) == true || smart.isNextTransport()) {
            auto& peek = smart.isNextTransport() ? smart.peekNext() : target;
            if (MovementMath::pythagoras(peek["x"].get<int>(), peek["y"].get<int>(), character->getX(),
                                         character->getY()) < 75) {
                transport(peek["map"].get<std::string>(), peek["s"].get<int>());
                mSkeletonLogger->info(peek.dump());
                while (peek["map"].get<std::string>() != character->getMap()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
                if (smart.isNextTransport()) smart.ignoreFirst();
                target = nullptr;
                continue;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    if (killSwitch) smart.finished();
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
        else
            map = character->getMap();
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
        for (auto& [mapName, mapData] : maps.items()) {
            if (mapData.find("monsters") != mapData.end()) {
                // List of maps
                auto& mapMonsters = mapData["monsters"];
                for (auto& pack : mapMonsters) {
                    if (pack["type"] != to || getOrElse(mapData, "ignore", false) == true ||
                        getOrElse(mapData, "instance", false) == true)
                        continue;

                    // Some mobs, like phoenix and mvampire, use boundaries instead of bounday.
                    if (pack.find("boundaries") != pack.end()) {
                        // List of maps
                        auto& boundaries = pack["boundaries"];
                        if (cyclableSpawnCounts.find(to) == cyclableSpawnCounts.end()) cyclableSpawnCounts[to] = 0;
                        auto& boundary = boundaries[cyclableSpawnCounts[to]];
                        map = boundary[0].get<std::string>();
                        tx = (boundary[1].get<int>() + boundary[3].get<int>()) / 2.0;
                        ty = (boundary[2].get<int>() + boundary[4].get<int>()) / 2.0;
                        cyclableSpawnCounts[to] = (cyclableSpawnCounts[to] + 1) % boundaries.size();
                        goto exit;
                    } else if (pack.find("boundary") != pack.end()) {
                        auto& boundary = pack["boundary"];
                        map = mapName;
                        tx = (boundary[0].get<int>() + boundary[2].get<int>()) / 2.0;
                        ty = (boundary[1].get<int>() + boundary[3].get<int>()) / 2.0;
                        goto exit;
                    }
                }
            }
        }
    exit:;
    } else if (to == "upgrade" || to == "compound") {
        map = "main";
        tx = -204;
        ty = -129;
    } else if (to == "exchange") {
        map = "main";
        tx = -26;
        ty = -432;
    } else if (to == "potions") {
        std::string pMap = character->getMap();
        if (pMap == "halloween") {
            map = "halloween";
            tx = 149;
            ty = -182;
        } else if (pMap == "winterland" || pMap == "winter_inn" || pMap == "winter_cave") {
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

    auto& mapData = character->getClient().getData()["maps"][map];
    // Primitive map access check.
    // AFAIK, none of the maps have a door in an unaccessible map
    if (mapData.find("doors") == mapData.end() || mapData["doors"].size() == 0) {
        mSkeletonLogger->error("Failed to find door to map {}", map);
        return false;
    }
    auto& geom = character->getClient().getData()["geometry"][map];
    if (tx <= geom["min_x"].get<double>() || tx >= geom["max_x"].get<double>() || ty <= geom["min_y"].get<double>() ||
        ty >= geom["max_y"].get<double>()) {
        mSkeletonLogger->error("Cannot walk outside the map.");
        return false;
    }
    smart.initSmartMove(map, tx, ty);
    initSmartMove();
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

void PlayerSkeleton::useSkill(const std::string& ability, const nlohmann::json& data) {
    static std::vector<std::string> nameSkills = {
        "invis", "partyheal", "darkblessing", "agitate", "cleave", "stomp", "charge", "light", "hardshell", "track", "warcry", "mcourage", "scare"
    };
    static std::vector<std::string> nameIdSkills = {
        "supershot", "quickpunch", "quickstab", "taunt", "curse", "burst", "4fingers", "magiport", "absorb", "mluck", "rspeed", "charm", "mentalburst", "piercingshot", "huntersmark"
    };

    if (ability == "use_hp" || ability == "hp")
        useItem("hp");
    else if (ability == "use_mp" || ability == "mp")
        useItem("mp");
    else if (ability == "stop") {
        move(character->getX(), character->getY());
        getSocket().emit("stop");
    } else if (ability == "use_town" || ability == "town") {
        if (!character->isAlive()) {
            getSocket().emit("respawn");
        } else {
            getSocket().emit("town");
        }
    } else if (ability == "cburst") {
        if (data.is_array()) {
            getSocket().emit("skill", {{"name", "cburst"}, {"targets", data}});
        } else {
            auto entities = getNearbyHostiles(
                12, data.is_object() && data.find("type") != data.end() ? data["type"].get<std::string>() : "",
                character->getRange() - 2);
            
            std::vector<nlohmann::json> ids;

            int mana = (character->getMp() - 200) / entities.size();

            for (auto& entity : entities) {
                ids.push_back(nlohmann::json::array({entity["id"].get<std::string>(), mana}));
            }

            getSocket().emit("skill", {{"name", "cburst"}, {"targets", ids}});
        }
    } else if (ability == "3shot" || ability == "5shot") {
        int count = ability[0] - '0';
        if (data.is_array()) {
            getSocket().emit("skill", {{"name", ability}, {"ids", data}});
        } else {
            auto entities = getNearbyHostiles(count, data.is_object() && data.find("type") != data.end() ? data["type"].get<std::string>() : "",
                    character->getRange() - 2);
            std::vector<std::string> ids;
            for (auto& entity : entities) { ids.push_back(entity["id"].get<std::string>()); }
            getSocket().emit("skill", {{"name", ability}, {"ids", ids}});
        }
    } else if (ability == "pcoat") {
        auto item = findItem("poison");
        if (!item.has_value()) {
            mSkeletonLogger->warn("{}: You don't have the item required to use the pcoat skill (poison)", character->getName());
            return;
        }
        getSocket().emit("skill", {{"name", "pcoat"}, {"num", item.value()}});

    } else if (ability == "revive") {
        auto item = findItem("essenceoflife");
        if (!item.has_value()) {
            mSkeletonLogger->warn("{}: You don't have the item required to use the revive skill (essenceoflife)", character->getName());
            return;
        }
        getSocket().emit("skill", {{"name", ability}, {"num", item.value()}});
    } else if (ability == "poisonarrow") {
        auto item = findItem("poison");
        if (!item.has_value()) {
            mSkeletonLogger->warn("{}: You don't have the item required to use the poisonarrow skill (poison)", character->getName());
            return;
        }
        getSocket().emit("skill", {{"name", ability}, {"num", item.value()}, {"id", data.is_object() ? data["id"].get<std::string>() : data.get<std::string>()}});
    } else if (ability == "shadowstrike" || ability == "phaseout") {
        auto item = findItem("shadowstone");
        if (!item.has_value()) {
            mSkeletonLogger->warn("{}: You don't have the item required to use the {} skill (shadowstone)", character->getName(), ability);
            return;
        }
        getSocket().emit("skill", {{"name", ability}, {"num", item.value()}});
    } else if (ability == "throw") {
        if (character->getInventory()[data["slot"].get<int>()].is_null()) {
            mSkeletonLogger->warn("{}: There is no item in slot {}", character->getName(), data["slot"].get<int>());
            return;
        }
        getSocket().emit("skill", {{"name", ability}, {"num", data["slot"].get<int>()}, {"id", data["id"].get<std::string>()}});
    } else if(ability == "blink") {
        int x, y;
        if (data.is_array()) {
            x = data[0];
            y = data[1];
        } else {
            x = data["x"].get<int>();
            y = data["y"].get<int>();
        }
        getSocket().emit("skill", {{"name", ability}, {"x", x}, {"y", y}});
    } else if (ability == "energize") {
        // TODO: Add mana if/when it gets fixed
        getSocket().emit("skill", {{"name", ability}, {"id", data.is_object() ? data["id"].get<std::string>() : data.get<std::string>()}});
    } else if (std::find(nameSkills.begin(), nameSkills.end(), ability) != nameSkills.end()) {
        getSocket().emit("skill", {{"name", ability}});
    } else if (std::find(nameIdSkills.begin(), nameIdSkills.end(), ability) != nameIdSkills.end()) {
        getSocket().emit("skill", {{"name", ability}, {"id", data.is_object() ? data["id"].get<std::string>() : data.get<std::string>()}});
    } 
}

void PlayerSkeleton::useItem(const std::string& item) {
    auto& itemData = character->getClient().getData()["items"];
    for (unsigned long long i = 0; i < character->getInventory().size(); i++) {
        const nlohmann::json& iItem = character->getInventory()[i];
        if (iItem.is_null()) continue;
        if (iItem.find("name") == iItem.end() || !iItem["name"].is_string()) continue;
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

std::optional<int> PlayerSkeleton::findItem(const std::string& name) {
    for (unsigned long long i = 0; i < character->getInventory().size(); i++) {
        auto& slot = character->getInventory()[i];

        if (!slot.is_null()) {
            if (slot["name"].get<std::string>() == name) {
                return i;
            }
        }
    }
    return std::nullopt;
}

void PlayerSkeleton::loot(bool safe) {
    int looted = 0;

    std::lock_guard<std::mutex> guard(getSocket().getChestGuard());
    if (lootTimer.check() < 300) return;

    if (character->getSocket().getChests().size() == 0) return;
    for (auto& [id, chest] : character->getSocket().getChests()) {
        if (safe && chest["items"].get<int>() != 0 && chest["items"].get<int>() > character->countOpenInventory())
            continue;

        character->getSocket().emit("open_chest", {{"id", id}});

        looted++;
        if (looted == 2) break;
    }
    if (looted > 1) lootTimer.reset();
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

void PlayerSkeleton::say(const std::string& message) { character->getSocket().emit("say", {{"message", message}}); }
void PlayerSkeleton::partySay(const std::string& message) {
    character->getSocket().emit("say", {{"message", message}, {"party", true}});
}
void PlayerSkeleton::sendPm(const std::string& message, const std::string& user) {
    character->getSocket().emit("say", {{"message", message}, {"name", user}});
}

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
    } else {
        if (to.is_string())
            getSocket().emit("cm", {{"to", nlohmann::json::array({to.get<std::string>()})}, {"message", message}});
        else {
            if (!to.is_array()) {
                mSkeletonLogger->error(
                    "Attempted to call sendCm with the \"to\" parameter that isn't a string or an object. Ignoring.");
                return;
            }

            for (auto& username : to) {
                if (username.is_string()) {
                    if (character->getClient().isLocalPlayer(username)) {
                        character->getClient().dispatchLocalCm(username, message, character->getName());
                    } else {
                        getSocket().emit(
                            "cm", {{"to", nlohmann::json::array({username.get<std::string>()})}, {"message", message}});
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

void PlayerSkeleton::sendPartyInvite(std::string& name, bool isRequest) {
    character->getSocket().emit("party", {{"event", isRequest ? "request" : "invite"}, {"name", name}});
}

void PlayerSkeleton::sendPartyRequest(std::string& name) { sendPartyInvite(name, true); }

void PlayerSkeleton::acceptPartyInvite(std::string& name) {
    character->getSocket().emit("party", {{"event", "accept"}, {"name", name}});
}

void PlayerSkeleton::acceptPartyRequest(std::string& name) {
    character->getSocket().emit("party", {{"event", "raccept"}, {"name", name}});
}

void PlayerSkeleton::transport(const std::string& targetMap, int spawn) {
    if (targetMap == character->getMap()) {
        mSkeletonLogger->warn("Attempt to transport to the map you're currently in ignored.");
        return;
    }

    getSocket().emit(
        "transport",
        {{"to", targetMap},
         {"s", spawn == -1 ? character->getClient().getData()["npcs"]["transporter"]["places"]["targetMap"].get<int>()
                           : spawn}});
}

void PlayerSkeleton::leaveParty() { character->getSocket().emit("party", {{"event", "leave"}}); }

void PlayerSkeleton::respawn() { character->getSocket().emit("respawn"); }

std::vector<nlohmann::json> PlayerSkeleton::getNearbyHostiles(int limit, const std::string& type, int range) {
    if (limit == 0) return {};
    std::vector<nlohmann::json> entities;
    bool includePlayers = isPvp();

    for (auto& [id, entity] : character->getEntities()) {
        if (!entity.is_null()) {
            if (MovementMath::pythagoras(entity["x"].get<int>(), entity["y"].get<int>(), character->getX(),
                                         character->getY()) > range)
                continue;
            if (type != "" && entity.find("mtype") != entity.end() && entity["mtype"].get<std::string>() == type) {
                entities.push_back(entity);
            } else {
                if (entity["type"].get<std::string>() == "character" && includePlayers && type == "") {
                    entities.push_back(entity);
                } else if (entity["type"].get<std::string>() == "monster") {
                    entities.push_back(entity);
                }
            }
        }
        if (entities.size() == limit) break;
    }
    return entities;
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
        if (entity.value("dead", false) == true || entity.value("rip", false) == true) continue;

        double dist =
            MovementMath::pythagoras(character->getX(), character->getY(), double(entity["x"]), double(entity["y"]));

        if (dist < closest) {
            closest = dist;
            j = entity;
        }
    }
    return j;
}

std::string PlayerSkeleton::getIdFromJsonOrDefault(const nlohmann::json& entity) {
    if (!entity.is_null() && entity.find("id") != entity.end()) {
        return entity["id"].get<std::string>();
    }
    nlohmann::json target = getTarget();
    if (target.is_null() || target.find("id") == target.end()) {
        return "";
    }
    return target["id"].get<std::string>();
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

const GameData& PlayerSkeleton::getGameData() { return character->getClient().getData(); }
SocketWrapper& PlayerSkeleton::getSocket() { return character->wrapper; }

bool PlayerSkeleton::isPvp() {
    auto server = character->getServer();

    if (server.has_value() && (*server.value()).isPvp()) {
        return true;
    }
    auto& cache = character->getClient().getData()["maps"][character->getMap()];

    return cache.find("pvp") != cache.end() && cache["pvp"].get<bool>() == true;
}

} // namespace advland
