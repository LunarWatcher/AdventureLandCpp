#include "lunarwatcher/AdvLand.hpp"
#include "lunarwatcher/game/PlayerSkeleton.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <functional>
#include "lunarwatcher/utils/Timer.hpp"

/**
 * In this library, each bot is defined in a unit called a skeleton.
 * The super class contains the character, as well as many of the methods.
 * If you've played the JS version, you'll be familiar with the character 
 * properties. (Note that not everything is implemented yet.)
 *
 * Each skeleton defines two methods you MUST override: onStart and onStop.
 * onStart tells you when your character has connected, and this is where
 * you should start your threads. **DO NOT** run blocking operations in 
 * the onStart or onStop methods. The functions aren't called async, and
 * may block the websocket threads.
 */
class DefaultSkeleton : public advland::PlayerSkeleton{
private:
    // The thread to host the runner on
    std::thread mRunner;
    // Timer for potions
    advland::Timer potTimer;
    // Logging. This is a pointer, which means accessing is done with `mLogger->`, 
    // in combination with a function call
    static const inline auto mLogger = spdlog::stdout_color_mt("DefaultSkeleton"); 

public:
    
    // Destructor. 
    ~DefaultSkeleton();

    // These are the two mandatory methods
    void onStart() override;   
    void onStop() override;

    // This is the entry point, meaning the function called by the thread.
    // The name is up to you, but it's named bot mainly because I wasn't feeling
    // too creative, and because this code isn't intended to last forever.
    void bot();
};

DefaultSkeleton::~DefaultSkeleton() {
    // Kill the thread.
    mRunner.join(); 
}

void DefaultSkeleton::onStart() {
    
    mRunner = std::thread(std::bind(&DefaultSkeleton::bot, this));
}

void DefaultSkeleton::bot() {
    
    while (true) {
        // Loot first 
        loot();

        // Then, check health and MP
        if (character->getHp() <= character->getMaxHp() - 200 
                && potTimer.check() > 2000) { // the check function returns the time since the last reset, or the start time. If none is specified, the start time is 0, meaning 
                                              // the first call to this method will return a very high number. 
            use("hp");
            // Call reset when you're ready to start over (usually, this is done when you've taken the action you want to time/rate limit)
            potTimer.reset();
        } else if (character->getMp() <= character->getMaxMp() - 300
                && potTimer.check() > 2000) {
            use("mp");
            potTimer.reset();
        }

        // getTarget is defined in the parent class, along with a bunch of other functions 
        nlohmann::json target = getTarget();

        // Basic retargeting logic 
        if (target.is_null() || target.value("dead", false) || !canMove(target["x"], target["y"])) {
            target = getNearestMonster({{"type", "bee"}}); // {{"type", "bee"}} is a JSON object (specifically `{"type": "bee"}`). This is using nlohmann::json (nlohmann/json on GitHub)
            if (!target.is_null())
                changeTarget(target);
        } 
        
        // the is_object check may be useless. It was added during debugging and might be unnecessary now that it has been fixed.
        if (!target.is_null() && target.is_object() && canAttack(target)) {
            // mLogger->info("Attacking: {}", target.dump());
            if(!inAttackRange(target)) {
                move(character->getX()  + (double(target["x"]) - character->getX()) / 2.0,
                        character->getY() + (double(target["y"]) - character->getY()) / 2.0);
            } else {
                attack (target);
            }
        }

        // This sleeps the thread for 150 milliseconds. Sleeping is highly recommended to avoid eating CPU usage. 
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    } 
}

void DefaultSkeleton::onStop() {
    // In the default code, there's nothing that needs to happen when the socket stops.
}

int main() {
    
    // The client is the core of the bots. It controlls all the bots, as well as handling logic. 
    advland::AdvLandClient client(/*{{{*/
            "email@example.com", "1234" // A note about this: some time soon, credentials are planned to be moved to a gitignored JSON file to avoid storing it in the code. 
            /*}}}*/);
    // Initialize the skeleton
    DefaultSkeleton defaultSkeleton;
    advland::Server* server = client.getServerInCluster("EU", "I");
    if (server == nullptr) {
        std::cout << "No server?!\n";
    }else {
        // Add a player.
        client.addPlayer("YourUsername", *server, defaultSkeleton);
        // Here, you can add more players in the same syntax. DO NOT use the same skeleton instance
        // for several characters. If you intend on using the same skeleton, you have to initialize 
        // it once for each character. example:
        // DefaultSkeleton defaultSkeleton1;
        // client.addPlayer("YourOtherCharacter", *server, defaultSkeleton1)
        
        // Once you've added your players, you need to start the client.
        // There's two ways to start it: Option one, as used here:
        client.startBlocking();
        // which starts and blocks the thread. Any code after the statement
        // won't be executed until the client shuts down.
        // 
        // The other alternative is using:
        // client.startAsync(); 

        // However, note that this requires you to block the thread. If the main function exits in C++,
        // the program dies even if there's threads still running.
    }
    std::cout << "Complete! Shutting down."; 
    return 0;
}
