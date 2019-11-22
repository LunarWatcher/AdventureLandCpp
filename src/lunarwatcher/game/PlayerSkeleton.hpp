#ifndef LUNARWATCHER_ADVLAND_PLAYERSKELETON
#define LUNARWATCHER_ADVLAND_PLAYERSKELETON

#include "lunarwatcher/movement/SmartMoveHelper.hpp"
#include "lunarwatcher/net/SocketWrapper.hpp"
#include "lunarwatcher/objects/GameData.hpp"
#include "lunarwatcher/utils/ParsingUtils.hpp"
#include "lunarwatcher/utils/Timer.hpp"
#include "nlohmann/json.hpp"
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string>
#include <thread>
#include <vector>
#include "lunarwatcher/utils/ThreadingUtils.hpp"

namespace advland {

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
    std::shared_ptr<uvw::WorkReq> smartMoveHandler;
    std::thread uvThread;

    SmartMoveHelper smart;
    Types::TimePoint last; 
    bool running = true;
    void initSmartMove();
    void dijkstraProcessor();

protected:
    std::shared_ptr<Player> character;
    LoopHelper loop; 

    void processInternals();
public:
    ~PlayerSkeleton() {
        if (uvThread.joinable())
            uvThread.join();
    }
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

    void startUVThread() {
        running = true;
        uvThread = std::thread([this]() {
            while (running) {
                loop.getLoop()->run<uvw::Loop::Mode::ONCE>();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }

    void stopUVThread() {
        running = false;
        uvThread.join();
    }

    // Party
    virtual void onPartyRequest(std::string /* name */) {}
    virtual void onPartyInvite(std::string /* name */) {}

    virtual void onFriendRequest() {}
    virtual void onCm(const std::string& /* name */, const nlohmann::json& /* data */) {}
    virtual void onPm(const std::string& /* name */, const std::string& /* message */) {}
    virtual void onChat(const std::string& /* name */, const std::string& /* message */) {}

    // API functions
    bool canMove(double x, double y);
    bool canWalk(const nlohmann::json& entity);
    bool canAttack(nlohmann::json& entity);
    bool canUse(const std::string& skill);
    bool inAttackRange(nlohmann::json& entity);

    void move(double x, double y);
    bool smartMove(const nlohmann::json& destination);
    bool smartMove(const nlohmann::json& destination, std::function<void()> callback);

    void say(const std::string& message);
    void partySay(const std::string& message);
    void sendPm(const std::string& message, const std::string& user);

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
    
    /**
     * Gets multiple nearby hostiles
     */
    std::vector<nlohmann::json> getNearbyHostiles(unsigned long long limit = 0, const std::string& type = "", int range = 100);

    void useSkill(const std::string& ability, const nlohmann::json& data = {});
    void useItem(const std::string& skill);
    void loot(bool safe = true /*, std::string sendChestIdsTo=""*/);

    void sendGold(std::string to, unsigned long long amount);
    void sendItem(std::string to, int itemIdx, unsigned int count = 1);

    /**
     * Equips an item from the specified inventory index.
     * The itemSlot parameter is used when specifying which
     * character slot to equip the item to.
     */
    void equip(int inventoryIdx, std::string itemSlot = "");

    /**
     * Attempts to find an item in the character's inventory.
     *
     * Returns an empty optional if none is found, otherwise the ID of
     * the item.
     */
    std::optional<int> findItem(const std::string& name);

    nlohmann::json getPlayer(std::string name);
    nlohmann::json getTarget();
    std::string getIdFromJsonOrDefault(const nlohmann::json& entity);

    void changeTarget(const nlohmann::json& entity);
    void sendTargetLogic(const nlohmann::json& entity);

    std::shared_ptr<Player>& getCharacter() { return character; }
    bool isSmartMoving() { return smart.isSmartMoving(); }
    SmartMoveHelper& getSmartHelper() { return smart; }
    const GameData& getGameData();
    SocketWrapper& getSocket();

    const nlohmann::json getTargetOf(const nlohmann::json& entity);

    bool isPvp();

    // Looping
    /**
     * Sets a timeout. 
     */
    std::shared_ptr<uvw::TimerHandle> setTimeout(LoopHelper::TimerCallback callback, int timeout);
    
    /**
     * Creates an interval
     */
    std::shared_ptr<uvw::TimerHandle> setInterval(LoopHelper::TimerCallback callback, int interval, int timeout = -1);
    
    /**
     * Creates a job. This is executed in a threadpool.
     */
    std::shared_ptr<uvw::WorkReq> createJob(std::function<void()> callback);

    /**
     * This must be used for any function calls that modify the loop (i.e. setTimeout or setInterval) when
     * the function is called inside a job (see {@link createJob}). 
     */
    void runOnUiThread(std::function<void(const uvw::AsyncEvent&, uvw::AsyncHandle&)> callback);
   
    std::shared_ptr<uvw::TimerHandle> setMainLoop(LoopHelper::TimerCallback callback, int interval) {
        return setInterval([callback, this](const auto& event, auto& handle) {
            processInternals(); 
            callback(event, handle);
        }, interval, 0);
    }
};

} // namespace advland

#endif
