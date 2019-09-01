#ifndef MAPPROCESSING_HPP
#define MAPPROCESSING_HPP

#include "lunarwatcher/game/PlayerSkeleton.hpp"
#include "lunarwatcher/objects/GameData.hpp"
#include "lunarwatcher/objects/Map.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <utility>
#include <vector>

namespace advland {

typedef std::vector<std::pair<int, int>> CoordMap;

class MapProcessor {
private:
    auto static inline const mLogger = spdlog::stdout_color_mt("MapProcessor");

    // <target, from>
    std::map<std::string, std::vector<std::string>> doorTransportMap;
    std::map<std::string, Map> maps;

    int bSearch(const nlohmann::json& lines, int search);
    bool canMove(double x1, double y1, double x2, double y2, const nlohmann::json& geom);

    std::map<std::pair<int, int>, bool> getAdjacentPixels(const double& x, const double& y, Map& map);

    /**
     * Plots out several general door paths, which are the base paths to base further 
     * work on. 
     */
    std::vector<std::vector<std::string>> getDoorPathsTo(std::string from, std::string map);
    /**
     * Subfunction of getDoorPathsTo. This calculates the shortest paths by the least amount
     * of doors (the cost is equal to the amount of doors)
     */
    std::vector<std::string> doorDijkstra(std::string from, std::string to,
                                          std::map<std::string, std::vector<std::string>>& doorMap);

    void box(double x1, double y1, double x2, double y2, std::vector<bool>& map, int xSize);
    std::pair<double, double> toGameMapCoords(const double& x, const double& y, const double& minX, const double& minY);

public:
    static int convertPosToMapIndex(const double& x, const double& y, const double& minX, const double& minY,
                                    const double& xSize);
    void processMaps(const GameData& data);

    std::vector<std::pair<int, int>> dijkstra(PlayerSkeleton& player, SmartMoveHelper& smart);
};

} // namespace advland

#endif
