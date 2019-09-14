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

class MapProcessor {
private:
    auto static inline const mLogger = spdlog::stdout_color_mt("MapProcessor");

    // <target, from>
    std::map<std::string, std::vector<std::string>> doorTransportMap;
    std::map<std::string, Map> maps;
    
    int bSearch(const nlohmann::json& lines, int search);

    std::vector<std::pair<int, int>> getAdjacentPixels(const double& x, const double& y, Map& map);

    std::vector<std::string> doorDijkstra(std::string from, std::string to);

    void box(double x1, double y1, double x2, double y2, std::vector<bool>& map, int xSize);
    std::pair<double, double> toGameMapCoords(const double& x, const double& y, const double& minX, const double& minY);

    /**
     * Reduces the amount of data points in a path by returning the points the character can walk to
     * As an example, the Dijkstra method creates a vector of all the points in the path. This 
     * is a perfectly valid approach on a small scale - however, AL movements aren't limited to pixels,
     * and spamming the move() method for each pixel is a VERY BAD IDEA!
     *
     * What this does is check if the char canMove() to the position. This is only done when the path is
     * found to reduce wasted computations.
     */
    std::vector<std::pair<int, int>> prunePath(std::vector<std::pair<int, int>> raw, const nlohmann::json& geom);

    /**
     * Dijkstra helper class
     */
    class PathDist {
    public:
        double dist;
        std::optional<std::pair<int, int>> lastPosition;
        PathDist() : dist(0) {}
        PathDist(double dist) : dist(dist) {}
        PathDist(double dist, std::optional<std::pair<int, int>> lastPos) : dist(dist), lastPosition(lastPos) {}
    };

public:
    static unsigned long long convertPosToMapIndex(const double& x, const double& y, const double& minX, const double& minY,
                                    const double& xSize);
    static bool isTransportDestination(std::string target, GameData& data);

    void processMaps(const GameData& data);
    bool canMove(const double& x1, const double& y1, const double& x2, const double& y2, const nlohmann::json& geom, bool trigger = false);
    
    std::optional<Door> getDoorTo(std::string currMap, std::string destination, GameData& data);
    bool dijkstra(PlayerSkeleton& player, SmartMoveHelper& smart, std::string currMap = "");
};

} // namespace advland

#endif
