#ifndef LUNARWATCHER_ADVLAND_PLAYERSKELETON
#define LUNARWATCHER_ADVLAND_PLAYERSKELETON

#include "lunarwatcher/net/SocketWrapper.hpp"
#include "lunarwatcher/objects/GameData.hpp"
#include "lunarwatcher/utils/Timer.hpp"
#include "nlohmann/json.hpp"
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string>
#include <thread>
#include <vector>
#include <iostream>
#include <future>
#include "lunarwatcher/utils/ParsingUtils.hpp"

namespace advland {

class SmartMoveHelper {
private:
    int x, y;
    std::string map;
    std::vector<nlohmann::json> checkpoints;
    bool moving;
    bool searching;
    std::function<void()> callback;
    std::vector<std::string> mapPath;

    int internalOffset;
    unsigned short moveId = -1;
    bool running = true;

    std::optional<std::pair<int, int>> landingCoords;
    std::vector<std::future<void>> futures;
public:
    SmartMoveHelper() {}
    void initSmartMove(std::string map, int x, int y) {
        this->map = map;
        this->x = x;
        this->y = y;
        deinit(false);
        moveId++;
        running = true;
        moving = true;
        searching = true;
    }
    void deinit(bool full = true) {
        if (full) {
            map = "";
            moving = false;
            running = false;
            searching = false;
        }
        callback = 0;
        mapPath.clear();
        checkpoints.clear();
        internalOffset = 0;
    }
    bool isSmartMoving() { return moving; }
    void withCallback(std::function<void()> callback) { this->callback = callback; }
    std::string getTargetMap() { return map; }
    std::string getProcessableMap() {
        if (mapPath.size() == 0) return map;
        return mapPath[internalOffset];
    }
    std::string getNextProcessableMap() {
        return mapPath[internalOffset + 1];
    }
    void bumpOffset() {
        if (mapPath.size() > 0) internalOffset++;
    }
    void injectDoorPath(std::vector<std::string>& vec) { mapPath = vec; }
    void pushNode(const nlohmann::json& j) { checkpoints.push_back(j); }
    void pushNodes(const std::vector<std::pair<int, int>> movementNodes) {
        for (auto& pos : movementNodes) {
            checkpoints.push_back({{"x", pos.first}, {"y", pos.second}});
        }
    }
    bool hasMultipleDestinations() { return mapPath.size() > 0; }
    int& getTargetX() { return x; }
    int& getTargetY() { return y; }
    unsigned short& getMoveId() { return moveId; }

    nlohmann::json getAndRemoveFirst() {
        nlohmann::json data = checkpoints[0];
        checkpoints.erase(checkpoints.begin());
        return data;
    }
    nlohmann::json& peekNext() { return checkpoints[0]; }
    void ignoreFirst() { checkpoints.erase(checkpoints.begin()); }
    void finished() {
        auto callback = this->callback;
        deinit();
        if (callback)
            futures.push_back(
                    std::async(std::launch::async, [=]() { callback(); }));
    }

    void stop(bool temp = false) {
        running = false; 
        if (temp) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            running = true;
        }
    }
    bool canRun() { return running; }
    void registerLandingCoordinates(int x, int y) { landingCoords = std::make_pair(x, y); }
    std::optional<std::pair<int, int>> getLandingCoords() {
        auto t = landingCoords;
        landingCoords.reset();
        return t;
    }
    bool hasMore() {
        return checkpoints.size() > 0;
    }
    bool isNextTransport() {
        if (checkpoints.size() == 0)
            return false;
        return getOrElse(checkpoints[0], "transport", false) == true;
    }

    bool isSearching() { return searching; }
    void ready() { searching = false; }

    void manageFutures() {
        futures.erase(
                std::remove_if(futures.begin(), futures.end(), [](std::future<void>& future) {
                    if (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                        future.get();
                        return true;
                    }
                    return false;
                }), futures.end());
    }
};

class Player;
class PlayerSkeleton {
private:
    static auto inline const mSkeletonLogger = spdlog::stdout_color_mt("PlayerSkeleton");
    // "Normal" timers
    Timer attackTimer;
    Timer lootTimer;

    // Skill timers (change frequently enough to warrant the use of a map)
    std::map<std::string, Timer> timers;
    std::string lastIdSent;
    std::map<std::string, int> cyclableSpawnCounts;

    bool killSwitch;
    std::thread smartMoveThread;

    SmartMoveHelper smart;
    void initSmartMove();
    void dijkstraProcessor();

protected:
    std::shared_ptr<Player> character;

public:
    void injectPlayer(std::shared_ptr<Player> character) { this->character = character; }

    /**
     * Invoked when the socket has connected for the first time.
     * This is where you can create your own flow, adding timeouts,
     * threads, or anything else.
     *
     *
     */
    virtual void onStart() = 0;

    /**
     * Invoked when the socket disconnects.
     */
    virtual void onStop() = 0;

    // Party
    virtual void onPartyRequest(std::string /* name */) {}
    virtual void onPartyInvite(std::string /* name */) {}

    virtual void onFriendRequest() {}
    virtual void onCm(std::string /* name */, const nlohmann::json& /* data */) {}

    // API functions
    bool canMove(double x, double y);
    bool canWalk(const nlohmann::json& entity);
    bool canAttack(nlohmann::json& entity);
    bool canUse(const std::string& skill);
    bool inAttackRange(nlohmann::json& entity);
    
    void move(double x, double y);
    bool smartMove(const nlohmann::json& destination);
    bool smartMove(const nlohmann::json& destination, std::function<void()> callback);

    void say(std::string message);
    // TODO partySay (message: msg, party: true)
    // TODO pmSay (message: msg, name: user)

    void sendPartyInvite(std::string& name, bool isRequest = false);
    void sendPartyRequest(std::string& name);
    void acceptPartyInvite(std::string& name);
    void acceptPartyRequest(std::string& name);
    void leaveParty();

    /**
     * Transports to a target map.
     */
    void transport(const std::string& targetMap, int spawn = -1);
    /**
     * Transports into town (equivalent of pressing X in the game)
     */
    void townTransport();

    /**
     * Sends a CM (Code Message). Code messages to other characters run on this client are
     * sent locally, and not through the AL server.
     * Note that this has a high code cost, and may trigger disconnection.
     *
     * Strings can also be passed directly to the message argument - it doesn't have to be
     * a full JSON object. Strings, as well as numbers, are actually considered JSON objects.
     */
    void sendCm(const nlohmann::json& to, const nlohmann::json& message);

    /**
     * @param entity  The entity to attack, or alternatively the ID of an entity or the
     *                username of a player
     */
    void attack(nlohmann::json& entity);

    /**
     * @param entity  Similar to attack, the entity to heal, or the ID of an entity or
     *                the username of a player
     */
    void heal(nlohmann::json& entity);
    void respawn();
    nlohmann::json getNearestMonster(const nlohmann::json& attribs);

    void useSkill(const std::string& ability);
    void use(const std::string& skill);
    void loot(bool safe = true /*, std::string sendChestIdsTo=""*/);

    void sendGold(std::string to, unsigned long long amount);
    void sendItem(std::string to, int itemIdx, unsigned int count = 1);

    /**
     * Equips an item from the specified inventory index.
     * The itemSlot parameter is used when specifying which
     * character slot to equip the item to.
     */
    void equip(int inventoryIdx, std::string itemSlot = "");

    nlohmann::json getPlayer(std::string name);
    nlohmann::json getTarget();

    void changeTarget(const nlohmann::json& entity);
    void sendTargetLogic(const nlohmann::json& entity);

    Player& getCharacter() { return *character; }
    bool isSmartMoving() { return smart.isSmartMoving(); }
    SmartMoveHelper& getSmartHelper() { return smart; }
    const GameData& getGameData();
    SocketWrapper& getSocket();
};

} // namespace advland

#endif
