#include "movement/MapProcessing.hpp"
#include <cmath>
#include <map>
#include <string>
#include <iostream>

namespace advland {

void MapProcessor::box(double x1, double y1, double x2, double y2, std::vector<bool>& map, int xSize) {
    for (double x = x1; x < x2; x++)
        for (double y = y1; y < y2; y++)
            if (map[xSize * y + x] == 0) {
                map[xSize * y + x] = 1;
            }
}

int MapProcessor::bSearch(const nlohmann::json& lines, int search) {
    int low = 0, high = lines.size() - 1;
    while (low + 1 != high) {
        int m = std::floor((low + high) / 2);
        if (lines[m][0] >= search) {
            high = m;
        } else low = m;
    }
    return high;
}

bool MapProcessor::canMove(double x1, double y1, double x2, double y2, const nlohmann::json& geom) {
    double x = std::min(x1, x2);
    double y = std::min(y1, y2);
    double X = std::max(x1, x2);
    double Y = std::max(y1, y2);

    auto& xLines = geom["x_lines"];
    auto& yLines = geom["y_lines"];

    for (unsigned long long i = bSearch(xLines, x); i < xLines.size(); i++) {
        auto& line = xLines[i];
        if (line[0].get<double>() > X)
            break;
        double yCom = ((y2 - y1) / (x2 - x1) * (line[0].get<double>() - x1));
        if (yCom + y1 < line[2].get<double>() && yCom + y1 > line[1].get<double>())
            return false;
    }

    for (unsigned long long i = bSearch(yLines, y); i < yLines.size(); i++) {
        auto& line = yLines[i];
        if (line[0].get<double>() > Y)
            break;
        double xCom = ((x2 - x1) / (y2 - y1) * (line[0].get<double>() - y1));
        if (xCom + x1 < line[2].get<double>() && xCom + x1 > line[1].get<double>()) 
            return false;
    }

    return true;
}

std::pair<double, double> MapProcessor::toGameMapCoords(const double& x, const double& y, const double& minX,
                                                        const double& minY) {
    return std::make_pair(x /* * boxSize */ + minX, y + minY);
}

int MapProcessor::convertPosToMapIndex(const double& x, const double& y, const double& minX, const double& minY, const double& xSize) {
    return xSize * (y - minY) + (x - minX);
}

void MapProcessor::processMaps(const GameData& data) {
    static std::map<std::string, int> base = {{"h", 8}, {"v", 7}, {"vn", 2}};
    mLogger->info("Preprocessing maps...");
    for (auto& [mapName, mapData] : data["maps"].items()) {
        if (mapData.find("ignore") != mapData.end() && mapData["ignore"] == true) {
            mLogger->info("Ignored map: {}", mapName);
            continue;
        }
        if (mapData.find("no_bounds") != mapData.end() && mapData["no_bounds"] == true) {
            mLogger->info("Ignored boundless map: {}", mapName);
            continue;
        }
        mLogger->info("Processing {}...", mapName);
        auto& geom = data["geometry"][mapName];
        auto& xLines = geom["x_lines"]; // [x, y, endY]
        auto& yLines = geom["y_lines"]; // [y. x, endX]

        double minX = geom["min_x"].get<double>();
        double minY = geom["min_y"].get<double>();
        double maxX = geom["max_x"].get<double>();
        double maxY = geom["max_y"].get<double>();

        double xSize = maxX - minX; // divided by boxSize if boxSize != 1
        double ySize = maxY - minY + 50;

        std::vector<bool> map;
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
        maps[mapName] = Map {map, minX, minY, xSize, ySize, mapName}; 
    }
    mLogger->info("Maps processed.");
}

} // namespace advland

