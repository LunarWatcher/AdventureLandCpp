#include "movement/SmartMoveHelper.hpp"

namespace advland {

void SmartMoveHelper::initSmartMove(std::string map, int x, int y) {
    this->map = map;
    this->x = x;
    this->y = y;
    deinit(false);
    running = true;
    moving = true;
    searching = true;
}

void SmartMoveHelper::deinit(bool full) {
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

void SmartMoveHelper::withCallback(std::function<void()> callback) { this->callback = callback; }
std::string SmartMoveHelper::getTargetMap() { return map; }

std::string SmartMoveHelper::getProcessableMap() {
    if (mapPath.size() == 0) return map;
    return mapPath[internalOffset];
}

std::string SmartMoveHelper::getNextProcessableMap() { return mapPath[internalOffset + 1]; }

void SmartMoveHelper::bumpOffset() {
    if (mapPath.size() > 0) internalOffset++;
}

void SmartMoveHelper::injectDoorPath(std::vector<std::string>& vec) { mapPath = vec; }

void SmartMoveHelper::pushNode(const nlohmann::json& j) { checkpoints.push_back(j); }

void SmartMoveHelper::pushNodes(const std::vector<std::pair<int, int>>& movementNodes) {
    for (auto& pos : movementNodes) {
        checkpoints.push_back({{"x", pos.first}, {"y", pos.second}});
    }
}

bool SmartMoveHelper::hasMultipleDestinations() { return mapPath.size() > 0; }
int& SmartMoveHelper::getTargetX() { return x; }
int& SmartMoveHelper::getTargetY() { return y; }

nlohmann::json SmartMoveHelper::getAndRemoveFirst() {
    nlohmann::json data = checkpoints[0];
    checkpoints.erase(checkpoints.begin());
    return data;
}

nlohmann::json& SmartMoveHelper::peekNext() { return checkpoints[0]; }

void SmartMoveHelper::ignoreFirst() { checkpoints.erase(checkpoints.begin()); }

void SmartMoveHelper::finished() {

    auto callback = this->callback;
    deinit();
    if (callback) futures.push_back(std::async(std::launch::async, [=]() { callback(); }));
}

void SmartMoveHelper::stop(bool temp) {

    running = false;
    if (temp) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        running = true;
    }
}

bool SmartMoveHelper::canRun() { return running; }
void SmartMoveHelper::registerLandingCoordinates(int x, int y) { landingCoords = std::make_pair(x, y); }
std::optional<std::pair<int, int>> SmartMoveHelper::getLandingCoords() {
    auto t = landingCoords;
    landingCoords.reset();
    return t;
}

bool SmartMoveHelper::hasMore() { return checkpoints.size() > 0; }
bool SmartMoveHelper::isNextTransport() {
    if (checkpoints.size() == 0) return false;
    return getOrElse(checkpoints[0], "transport", false) == true;
}

bool SmartMoveHelper::isSearching() { return searching; }
void SmartMoveHelper::ready() { searching = false; }
void SmartMoveHelper::manageFutures() {
    if (futures.size() == 0)
        return;
    futures.erase(std::remove_if(futures.begin(), futures.end(),
                                 [](std::future<void>& future) {
                                     if (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                                         future.get();
                                         return true;
                                     }
                                     return false;
                                 }),
                  futures.end());
}
} // namespace advland
