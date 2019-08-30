#include "movement/MapProcessing.hpp"
#include <cmath>
#include <map>
#include <string>
#include <iostream>

namespace advland {

void MapProcessor::fill(double x, double y, double xSize, std::vector<short>& map) {
    std::vector<std::vector<int>> open = {{x, y}};
    int c = 0;
    while (open.size() - c > 0) {
        if (!map[open[c][0] + xSize * open[c][1]]) {
            map[open[x][0] + xSize * open[x][1]] = 2;
            open.push_back({open[c][0] + 1, open[c][1]});
            open.push_back({open[c][0] - 1, open[c][1]});
            open.push_back({open[c][0], open[c][1] + 1});
            open.push_back({open[c][0], open[c][1] - 1});
        }
        c++;
    }
}

void MapProcessor::box(double x1, double y1, double x2, double y2, std::vector<short>& map, int xSize) {
    for (double x = x1; x < x2; x++)
        for (double y = y1; y < y2; y++)
            if (map[xSize * y + x] != 0) {
                map[xSize * y + x] = 1;
            }
}

std::pair<double, double> MapProcessor::toGameMapCoords(const double& x, const double& y, const double& minX,
                                                        const double& minY) {
    return std::make_pair(x /* * boxSize */ + minX, y + minY);
}

void MapProcessor::processMaps(const GameData& data) {
    static std::map<std::string, int> base = {{"h", 8}, {"v", 7}, {"vn", 2}};

    for (auto& [mapName, mapData] : data["maps"].items()) {
        auto& geom = data["geometry"][mapName];
        auto& xLines = geom["x_lines"]; // [x, y, endY]
        auto& yLines = geom["y_lines"]; // [y. x, endX]

        double minX = mapData["min_x"].get<double>();
        double minY = mapData["min_y"].get<double>();
        double maxX = mapData["max_x"].get<double>();
        double maxY = mapData["max_y"].get<double>();

        double xSize = maxX - minX; // divided by boxSize if boxSize != 1
        double ySize = maxY - minY + 50;

        std::vector<short> map;
        map.resize(xSize * ySize, 0);

        for (auto& line : xLines) {
            double x = line[0].get<double>() - minX;
            double yTop = line[1].get<double>() - minY;
            double yBot = line[2].get<double>() - minY;

            double x1 = x - base["h"] - 1; // 1 = padding
            double y1 = yTop - base["vn"] - 1;
            double x2 = x + base["h"] + 1;
            double y2 = yBot + base["v"] + 1;
            box(x1, y1, x2, y2, map, xSize);
        }

        for (auto& line : yLines) {
            double y = line[0].get<double>() - minY;
            double xLeft = line[1].get<double>() - minX;
            double xRight = line[2].get<double>() - minX;

            double x1 = xLeft - base["h"] - 1; // 1 = padding
            double y1 = y - base["vn"] - 1;
            double x2 = xRight + base["h"] + 1;
            double y2 = y + base["v"] + 1;
            box(x1, y1, x2, y2, map, xSize);
        } 

    }
}

} // namespace advland

