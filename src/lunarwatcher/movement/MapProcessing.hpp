#ifndef MAPPROCESSING_HPP
#define MAPPROCESSING_HPP

#include "lunarwatcher/objects/GameData.hpp"
#include "lunarwatcher/objects/Map.hpp"
#include <utility>
#include <vector>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace advland {

typedef std::vector<std::pair<int, int>> CoordMap;

class MapProcessor {
private:
    auto static inline const mLogger = spdlog::stdout_color_mt("MapProcessor");
    std::map<std::string, Map> maps;

    int bSearch(const nlohmann::json& lines, int search);
    bool canMove(double x1, double y1, double x2, double y2, const nlohmann::json& geom);

    void box(double x1, double y1, double x2, double y2, std::vector<bool>& map, int xSize);
    std::pair<double, double> toGameMapCoords(const double& x, const double& y, const double& minX, const double& minY);
public:
    static int convertPosToMapIndex(const double& x, const double& y, const double& minX, const double& minY, const double& xSize);
    void processMaps(const GameData& data); 
};

}

#endif
