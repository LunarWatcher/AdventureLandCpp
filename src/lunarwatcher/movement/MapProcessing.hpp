#ifndef MAPPROCESSING_HPP
#define MAPPROCESSING_HPP

#include "lunarwatcher/objects/GameData.hpp"
#include <utility>
#include <vector>

namespace advland {

typedef std::vector<std::pair<int, int>> CoordMap;

class MapProcessor {
private:
    void box(double x1, double y1, double x2, double y2, std::vector<short>& map, int xSize);
    void fill(double x, double y, double xSize, std::vector<short>& map);
    std::pair<double, double> toGameMapCoords(const double& x, const double& y, const double& minX, const double& minY);
public:
    void processMaps(const GameData& data); 
};

}

#endif
