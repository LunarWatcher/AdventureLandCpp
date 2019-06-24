#ifndef LUNARWATCHER_ADVLAND_GAMEDATA
#define LUNARWATCHER_ADVLAND_GAMEDATA
#include "nlohmann/json.hpp"

namespace advland {
class GameData {
private:
    nlohmann::json data;

public:
    // This should never be used intentionally!
    GameData() {}
    GameData(std::string& rawJson) : data(nlohmann::json::parse(rawJson)) {}
    GameData(const GameData& old) : data(old.data) {}

    const nlohmann::json getData() { return data; }
};
} // namespace advland

#endif
