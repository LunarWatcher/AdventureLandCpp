#include "movement/MapProcessing.hpp"
#include "AdvLand.hpp"
#include "game/Player.hpp"
#include <cmath>
#include <iostream>
#include <map>
#include <string>

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
        } else
            low = m;
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
        if (line[0].get<double>() > X) break;
        double yCom = ((y2 - y1) / (x2 - x1) * (line[0].get<double>() - x1));
        if (yCom + y1 < line[2].get<double>() && yCom + y1 > line[1].get<double>()) return false;
    }

    for (unsigned long long i = bSearch(yLines, y); i < yLines.size(); i++) {
        auto& line = yLines[i];
        if (line[0].get<double>() > Y) break;
        double xCom = ((x2 - x1) / (y2 - y1) * (line[0].get<double>() - y1));
        if (xCom + x1 < line[2].get<double>() && xCom + x1 > line[1].get<double>()) return false;
    }

    return true;
}

std::pair<double, double> MapProcessor::toGameMapCoords(const double& x, const double& y, const double& minX,
                                                        const double& minY) {
    return std::make_pair(x /* * boxSize */ + minX, y + minY);
}

int MapProcessor::convertPosToMapIndex(const double& x, const double& y, const double& minX, const double& minY,
                                       const double& xSize) {
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
        mLogger->info("Processing doors and transporter(s)...");
        if (mapData.find("doors") != mapData.end()) {
            auto& doors = mapData["doors"];
            for (auto& door : doors) {
                std::string target = door[4].get<std::string>();
                auto& ref = doorTransportMap[target];
                if (std::find(ref.begin(), ref.end(), mapName) == ref.end()) ref.push_back(mapName);
            }
        }

        if (mapData.find("npcs") != mapData.end() && mapData["npcs"].size() > 0) {
            for (auto& npc : mapData["npcs"]) {
                if (npc["id"].get<std::string>() == "transporter") {
                    auto& places = data["npcs"]["transporter"]["places"];
                    for (auto& [place, _] : places.items()) {
                        auto& ref = doorTransportMap[place];
                        if (place == mapName) continue;
                        if (std::find(ref.begin(), ref.end(), mapName) == ref.end()) ref.push_back(mapName);
                    }
                    break;
                }
            }
        }
        mLogger->info("Processed doors for {}", mapName);

        maps[mapName] = Map{map, minX, minY, maxX, maxY, xSize, ySize, mapName};
    }
    mLogger->info("Maps processed.");
    mLogger->info("Commence sanity-checking door dijkstra");
    auto mainWint = doorDijkstra("main", "winter_inn");
    std::ostringstream one;
    std::copy(mainWint.begin(), mainWint.end(), std::ostream_iterator<std::string>(one, ", "));
    mLogger->info("Raw main-winterland dijkstra: {}", one.str());
}

std::map<std::pair<int, int>, bool> MapProcessor::getAdjacentPixels(const double& x, const double& y, Map& map) {
    std::map<std::pair<int, int>, bool> pos;
    for (double mx = x - 1; mx <= x + 1; mx++) {
        if (mx < map.getMinX() || mx > map.getMaxX()) continue;
        for (double my = y - 1; my <= y + 1; my++) {
            if (my < map.getMinY() || my > map.getMaxY()) {
                continue;
            }

            int idx = convertPosToMapIndex(mx, my, map.getMinX(), map.getMinY(), map.getXSize());
            pos[std::make_pair<int, int>(mx, my)] = map.getRawMapData()[idx];
        }
    }
    return pos;
}

std::vector<std::pair<int, int>> MapProcessor::dijkstra(PlayerSkeleton& player, SmartMoveHelper& smart) {
    auto& character = player.getCharacter();
    std::vector<std::tuple<int, int, std::string>> targets;

    if (character.getMap() != smart.getMap()) {
        auto& gameData = character.getClient().getData();
    }

    std::vector<std::tuple<int, int, std::string>> visited;
}


std::vector<std::string> MapProcessor::doorDijkstra(std::string from, std::string to) {
    std::vector<std::string> visited;
    std::vector<std::string> unvisited;
    std::map<std::string, std::pair<int, std::string>> dists;

    std::string& currPos = from;
    dists[from] = std::make_pair(0, "");
    std::vector<std::string> route = {to};
    // Dijkstra
    while (true) {
        std::vector<std::string>& nearby = doorTransportMap[currPos];
        for (std::string& pos : nearby) {
            if (std::find(visited.begin(), visited.end(), pos) == visited.end() &&
                std::find(unvisited.begin(), unvisited.end(), pos) == unvisited.end()) {
                unvisited.push_back(pos);
            }
        }
        for (std::string& u : unvisited) {
            int dist = dists[currPos].first + 1;
            if (dists.find(u) == dists.end() || dist < dists[u].first) dists[u] = std::make_pair(dist, currPos);
        }

        auto unv = std::find(unvisited.begin(), unvisited.end(), currPos);
        if (unv != unvisited.end()) unvisited.erase(unv);
        visited.push_back(currPos);

        if (currPos == to) {
            auto prePos = dists[to];

            while (prePos.second != "") {
                route.push_back(prePos.second);
                prePos = dists[prePos.second];
            }
            std::reverse(route.begin(), route.end());
            return route;
        }
        if (unvisited.size() == 0) return {};
        std::sort(unvisited.begin(), unvisited.end(), [dists](std::string& one, std::string& two) mutable -> bool {
            return (dists[one].first) < (dists[two].first);
        });
        currPos = unvisited[0];
    }
}

} // namespace advland

