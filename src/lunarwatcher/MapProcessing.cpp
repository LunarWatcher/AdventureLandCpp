#include "movement/MapProcessing.hpp"
#include "AdvLand.hpp"
#include "game/Player.hpp"
#include "math/Logic.hpp"
#include "utils/Hashing.hpp"
#include <cmath>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>

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

bool MapProcessor::canMove(double x1, double y1, double x2, double y2, const nlohmann::json& geom, int px, int py,
                           bool trigger) {
    static std::map<std::string, int> base = {{"h", 8}, {"v", 7}, {"vn", 2}};
    static std::vector<std::vector<int>> hitbox = {{-int(base["h"]), int(base["vn"])},
                                                   {int(base["h"]), int(base["vn"])},
                                                   {-int(base["h"]), -int(base["v"])},
                                                   {-int(base["h"]), -int(base["v"])}};

    if (trigger) {
        x1 += px;
        y1 += py;
    }
    double x = std::min(x1, x2);
    double y = std::min(y1, y2);
    double X = std::max(x1, x2);
    double Y = std::max(y1, y2);

    auto& xLines = geom["x_lines"];
    auto& yLines = geom["y_lines"];

    if (!trigger) {
        for (auto& i : hitbox) {
            int hitboxX = i[0];
            int hitboxY = i[1];
            int targetX = x2 + hitboxX;
            int targetY = y2 + hitboxY;
            if (!canMove(x1, y1, targetX, targetY, geom, hitboxX, hitboxY, true)) {
                return false;
            }
        }

        double pH = double(base["h"]);
        double nH = -pH;

        double vn = double(base["vn"]);
        double v = -double(base["v"]);

        if (x2 > x1) {
            pH = -pH;
            nH = -nH;
        }
        if (y2 > y1) {
            double tmp = vn;
            vn = -v;
            v = -tmp;
        }

        if (!canMove(x1, y1, x2 + nH, y2 + v, geom, nH, vn, true) ||
            !canMove(x1, y1, x2 + pH, y2 + v, geom, pH, v, true)) {
            return false;
        }

        return true;
    }

    for (unsigned long long i = bSearch(xLines, x); i < xLines.size(); i++) {
        auto& line = xLines[i];
        if (line[0] == x2 && ((line[1] <= y2 && line[2] >= y2) || (line[0] == x1 && line[1] >= y1 && y2 > line[0]))) {
            return false;
        }

        if (x > line[0]) continue;
        if (X < line[0]) break;

        double q = y1 + (y2 - y1) * (double(line[0]) - x1) / (x2 - x1 + 1e-8);
        if (!(double(line[1]) - 1e-8 <= q && q <= double(line[2]) + 1e-8)) {
            continue;
        }
        return false;
    }

    for (unsigned long long i = bSearch(yLines, Y); i < yLines.size(); i++) {
        auto& line = yLines[i];
        if (line[0] == y2 && ((line[1] <= x2 && line[2] >= x2) || (line[0] == y1 && x1 <= line[1] && x2 >= line[1]))) {
            return false;
        }
        if (y > line[0]) continue;
        if (Y < line[0]) break;

        double q = x1 + (x2 - x1) * (double(line[0]) - y1) / (y2 - y1 + 1e-8);
        if (!(double(line[1]) - 1e-8 <= q && q <= double(line[2]) + 1e-8)) continue;

        return false;
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
        std::vector<Door> mapDoors;

        if (mapData.find("doors") != mapData.end()) {
            auto& doors = mapData["doors"];
            for (auto& door : doors) {
                int spawn = door[5].get<int>();
                auto& tData = data["maps"][door[4].get<std::string>()]["spawns"];

                int landingX = tData[spawn][0].get<int>();
                int landingY = tData[spawn][1].get<int>();
                std::string target = door[4].get<std::string>();

                Door d(door[0].get<int>(), door[1].get<int>(), {{target, std::pair{landingX, landingY}}}, {target},
                       false, {{target, spawn}});
                mapDoors.push_back(d);

                auto& ref = doorTransportMap[target];
                if (std::find(ref.begin(), ref.end(), mapName) == ref.end()) ref.push_back(mapName);
            }
        }

        if (mapData.find("npcs") != mapData.end() && mapData["npcs"].size() > 0) {
            for (auto& npc : mapData["npcs"]) {
                if (npc["id"].get<std::string>() == "transporter") {
                    std::vector<std::string> doorTargets;
                    auto& places = data["npcs"]["transporter"]["places"];
                    std::map<std::string, std::pair<int, int>> positions;
                    std::map<std::string, int> spawns;

                    for (auto& [place, _] : places.items()) {
                        auto& ref = doorTransportMap[place];
                        if (place == mapName) continue;
                        if (std::find(ref.begin(), ref.end(), mapName) == ref.end()) ref.push_back(mapName);

                        auto spawn = data["npcs"]["transporter"]["places"][place].get<int>();
                        spawns[place] = spawn;

                        auto& spawns = data["maps"][place]["spawns"][spawn];
                        positions[place] = std::make_pair(spawns[0], spawns[1]);

                        doorTargets.push_back(place);
                    }
                    auto& position = npc["position"];
                    Door d(position[0].get<int>(), position[1].get<int>(), positions, doorTargets, true, spawns);
                    mapDoors.push_back(d);

                    break;
                }
            }
        }
        mLogger->info("Processed doors for {}", mapName);

        maps[mapName] = Map{map, mapDoors, minX, minY, maxX, maxY, xSize, ySize, mapName};
    }

    mLogger->info("Maps processed.");
}

std::vector<std::pair<int, int>> MapProcessor::getAdjacentPixels(const double& x, const double& y, Map& map) {
    static std::vector<std::vector<int>> moves = {{10, 0}, {0, 10}, {-10, 0}, {0, -10}};

    std::vector<std::pair<int, int>> pos;
    for (auto& move : moves) {
        int mx = x + move[0];
        int my = y + move[1];

        int idx = convertPosToMapIndex(mx, my, map.getMinX(), map.getMinY(), map.getXSize());
        if (!map.getRawMapData()[idx]) {
            pos.push_back(std::pair<int, int>(mx, my));
        }
    }

    return pos;
}

bool MapProcessor::isTransportDestination(std::string target, GameData& data) {
    // Places is a map containing the destination key, and a value passed as s when using
    // the transport websocket call. I have no idea what the variable signifies.
    auto& c = data["npcs"]["transporter"]["places"];
    return c.find(target) != c.end();
}

std::optional<Door> MapProcessor::getDoorTo(std::string currMap, std::string destination, GameData& data) {
    auto& map = maps.at(currMap);
    if (map.hasTransporter() && isTransportDestination(destination, data)) {
        return map.getTransporter();
    }
    for (auto& door : map.getDoors()) {
        // Skip transporters (eliminated earlier)
        if (door.isTransporter()) continue;
        // Find doors leading to the destination
        if (door.isTarget(destination)) {
            // And return it
            return door;
        }
    }
    // Otherwise, return an empty optional.
    // Realistically, this should never happen when used
    // with the dijkstra method.
    return {};
}

std::vector<std::pair<int, int>> MapProcessor::prunePath(std::vector<std::pair<int, int>> rawPath,
                                                         const nlohmann::json& geom) {
    auto& oldPos = rawPath[0];
    if (rawPath.size() == 1) {
        // Safe-guard against short moves
        return {oldPos};
    }
    std::vector<std::pair<int, int>> path;
    for (unsigned long long i = 1; i < rawPath.size(); i++) {
    retry:
        auto& currPos = rawPath[i];
        if (!canMove(oldPos.first, oldPos.second, currPos.first, currPos.second, geom)) {
            // Last moveable point
            auto& tmp = rawPath[i - 1];
            if (tmp == oldPos) continue;

            path.push_back(tmp);
            oldPos = tmp;
            goto retry;

        } else if (i == rawPath.size() - 1)
            path.push_back(currPos);
    }
    return path;
}

void MapProcessor::dijkstra(PlayerSkeleton& player, SmartMoveHelper& smart, std::string currMap) {
    if (!smart.canRun()) return;

    auto& character = player.getCharacter();
    if (smart.getTargetMap() == character.getMap() &&
        canMove(character.getX(), character.getY(), smart.getTargetX(), smart.getTargetY(),
                character.getClient().getData()["geometry"][smart.getTargetMap()])) {
        smart.pushNodes({std::make_pair(character.getX(), character.getY())});
        return;
    }
    std::vector<std::tuple<int, int, std::string>> targets;
    if (currMap == "") currMap = character.getMap();

    if (currMap == smart.getTargetMap() || (smart.hasMultipleDestinations() && currMap == smart.getProcessableMap())) {
        int x, y;
        std::pair<int, int> rCurrPos;

        auto landingCoords = smart.getLandingCoords();

        std::optional<Door> door;
        std::optional<std::string> targetMap;
        std::optional<std::pair<int, int>> landingPosition;

        if (landingCoords.has_value()) {
            rCurrPos = std::make_pair(landingCoords->first, landingCoords->second);
        } else if (character.getMap() == currMap)
            rCurrPos = std::make_pair(character.getX(), character.getY());

        if (currMap == smart.getTargetMap()) {
            x = smart.getTargetX();
            y = smart.getTargetY();

        } else {
            targetMap = smart.getNextProcessableMap();
            door = getDoorTo(currMap, targetMap.value(), character.getClient().getData());
            if (!door.has_value()) {
                // This should never happen, unless the doorDijkstra method is horribly
                // broken somewhere
                throw "CRITICAL FAILURE: Failed to find door to destination.";
            }

            landingPosition = door->getLandingPosition(*targetMap);
            smart.registerLandingCoordinates(landingPosition->first, landingPosition->second);
            x = door->getX();
            y = door->getY();
        }
        auto targetPos = std::pair<int, int>(x, y);
        auto currPos = std::pair<int, int>(rCurrPos);
        std::unordered_map<std::pair<int, int>, bool, PairHash> visited;
        std::vector<std::pair<int, int>> unvisited;

        auto& mapObj = maps[currMap];
        auto& geom = player.getCharacter().getClient().getData()["geometry"][currMap];

        // < <x, y>, <dist,
        std::unordered_map<std::pair<int, int>, PathDist, PairHash> dists;

        dists[currPos] = PathDist{0};
        Timer timer;

        while (smart.canRun()) {

            auto adjacentTiles = getAdjacentPixels(currPos.first, currPos.second, mapObj);
            std::vector<std::pair<int, int>> neighbors;

            for (auto& pos : adjacentTiles) {
                if (visited.find(pos) == visited.end() &&
                    std::find(unvisited.begin(), unvisited.end(), pos) == unvisited.end() &&
                    canMove(pos.first, pos.second, currPos.first, currPos.second, geom)) {
                    neighbors.push_back(pos);
                    unvisited.push_back(pos);
                }
            }

            PathDist& d = dists[currPos];

            for (auto& p : neighbors) {
                // Pythagoras to determine the distance
                double dist =
                    d.dist + std::sqrt(std::pow(currPos.first - p.first, 2) + std::pow(currPos.second - p.second, 2));
                if (dists.find(p) == dists.end() || dist < dists[p].dist) dists[p] = PathDist{dist, currPos};
            }

            if (visited.size() > 0) unvisited.erase(unvisited.begin());
            visited[currPos] = true;
            // *currPos == *targetpos
            if (MovementMath::pythagoras(currPos.first, currPos.second, targetPos.first, targetPos.second) <= 20) {
                std::vector<std::pair<int, int>> path;
                auto& dist = dists[currPos];
                while (dist.lastPosition.has_value()) {
                    path.push_back(*dist.lastPosition);
                    dist = dists[*dist.lastPosition];
                }
                std::reverse(path.begin(), path.end());
                path.push_back(targetPos);

                auto vec = prunePath(path, character.getClient().getData()["geometry"][currMap]);
                smart.pushNodes(vec);
                smart.bumpOffset();
                if (door.has_value()) {
                    smart.pushNode({{"x", door->getX()},
                                    {"y", door->getY()},
                                    {"map", targetMap.value()},
                                    {"landingX", landingPosition->first},
                                    {"landingY", landingPosition->second},
                                    {"transport", true},
                                    {"s", door->getSpawn(targetMap.value())}});
                }

                if (character.getMap() == smart.getTargetMap()) {
                    smart.ready();
                }
                return;
            }
            if (unvisited.size() == 0) {
                mLogger->warn("Failed to find a path.");
                return;
            }
            currPos = (*unvisited.begin());
        }

    } else {
        std::vector<std::string> path = doorDijkstra(currMap, smart.getTargetMap());
        if (path.size() == 0) {
            mLogger->warn("Failed to find a door path from {} to {}", currMap, smart.getTargetMap());
            smart.deinit();
            return;
        }
        smart.injectDoorPath(path);
        for (auto& mapToProcess : path)
            dijkstra(player, smart, mapToProcess);
        smart.ready();
    }
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

