#ifndef LUNARWATCHER_ADVLAND_SMARTMOVEHELPER_HPP
#define LUNARWATCHER_ADVLAND_SMARTMOVEHELPER_HPP
#include "lunarwatcher/utils/ParsingUtils.hpp"
#include <functional>
#include <future>
#include <nlohmann/json.hpp>
#include <optional>
#include <utility>
#include <vector>

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
    bool running = true;

    std::optional<std::pair<int, int>> landingCoords;
    std::vector<std::future<void>> futures;

public:
    SmartMoveHelper() = default;
    void initSmartMove(std::string map, int x, int y);

    /**
     * Called if the smart move search fails, is aborted, or 
     * the source is changed.
     */
    void deinit(bool full = true);

    /**
     * Returns whether smartMove is active or not. 
     * Also includes searching
     */
    bool isSmartMoving() { return moving; }

    /**
     * Registers a callback. Note that only one callback is 
     * supported, and it's cleared if deinit is called.
     */
    void withCallback(std::function<void()> callback);

    /**
     * Gets the target map (the map the player is moving to)
     */ 
    std::string getTargetMap();

    /**
     * Gets the processable map. This is only used by the smartMove
     * calculation process, and is used to help determine which
     * map to find a path in when dealing with multiple maps.
     */
    std::string getProcessableMap();

    /**
     * Identical to getProcessableMap, but returns the map one index up 
     * in the list.
     */
    std::string getNextProcessableMap();

    /**
     * Bumps the offset used by getProcessableMap and getNextProcessableMap
     */
    void bumpOffset();

    /**
     * Adds the door path (calculated separately, used to find paths between
     * positions in multiple maps)
     */
    void injectDoorPath(std::vector<std::string>& vec);
    
    /**
     * Adds a coordinate or point to visit
     */
    void pushNode(const nlohmann::json& j);

    /**
     * Converts a vector of ints produced by the smartMove method 
     * to nodes, and adds them to the internal list of target coordinates
     */
    void pushNodes(const std::vector<std::pair<int, int>>& movementNodes);

    /**
     * Returns whether the current path involves multiple maps or not
     */
    bool hasMultipleDestinations();

    /**
     * Returns the X coordinate for the final destination
     */
    int& getTargetX();

    /**
     * Returns the Y coordinate for the final destination
     */
    int& getTargetY();

    /**
     * Gets the first coordinate from the list, and removes it from the list.
     * THIS SHOULD ONLY BE USED BY THE SMART MOVE IMPLEMENTATION! Other usage
     * may result in broken behavior 
     */
    nlohmann::json getAndRemoveFirst();
 
    /**
     * Returns the first item of the coordinate vector without removing it
     */
    nlohmann::json& peekNext();

    /**
     * Deletes the first item in the coordinate vector without returning it 
     */
    void ignoreFirst();

    /**
     * Called when the character reaches the final destination. Also triggers
     * the callback function.
     */
    void finished();

    /**
     * Stops the calculation and/or movement processing.
     */
    void stop(bool temp = false);

    /**
     * Internal: determines whether the threads dealing with smartMove can
     * continue or not 
     */
    bool canRun();

    /**
     * Internal: registers landing coordinates. Used to determine starting position
     * when dealing with multiple maps and dhoors 
     */
    void registerLandingCoordinates(int x, int y);

    /**
     * Returns the landing coordinates created with registerLandingCoordinates
     */
    std::optional<std::pair<int, int>> getLandingCoords();

    /**
     * Returns whether there's checkpoints left or not 
     */ 
    bool hasMore();

    /**
     * Checks if the next node is a transport or door 
     */
    bool isNextTransport();

    /**
     * Returns whether the algorithm is still searching or not 
     */
    bool isSearching();

    /**
     * Internal: called when the algorithm is done searching
     */
    void ready();

    /**
     * Internal: manages futures created to support callbacks.
     */
    void manageFutures();
    const std::vector<nlohmann::json>& getCheckpoints() { return checkpoints; }
};

} // namespace advland

#endif
