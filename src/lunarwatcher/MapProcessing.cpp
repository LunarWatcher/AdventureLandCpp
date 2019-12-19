#include "movement/MapProcessing.hpp"
#include "AdvLand.hpp"
#include "game/Player.hpp"
#include "math/Logic.hpp"
#include "utils/Hashing.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <queue>
#include <string>
#include <unordered_map>
#include "game/PlayerSkeleton.hpp"
#include <limits.h>
#include "utils/MapUtils.hpp"

namespace advland {

namespace defs {
    
    const double EPS = 1e-8;
    const double REPS = std::numeric_limits<double>::epsilon();

    // NOTE: The real base is 8, 7, 2, but because of a  bug for the can_move method, the values have been cut in half.
    // If this doesn't fix all the edge cases, remove the bound check.
    const std::map<std::string, int> base = {{"h", 4}, {"v", 3}, {"vn", 1}};
    const std::vector<std::vector<int>> hitbox = {{-base.at("h"), base.at("vn")},
                                                  {base.at("h"), base.at("vn")},
                                                  {-base.at("h"), -base.at("v")},
                                                  {base.at("h"), -base.at("v")}};

} // namespace defs

void MapProcessor::box(double x1, double y1, double x2, double y2, std::vector<bool>& map, int xSize) {
    for (double x = x1; x <= x2; x++)
        for (double y = y1; y <= y2; y++) {
            unsigned long long tmp = xSize * y + x;
            if (tmp < map.size())
                map.at(tmp) = 1;
            
        }
}

int MapProcessor::bSearch(const nlohmann::json& lines, int search) {
    int high = 0, low = lines.size() - 1;
    while (high < low - 1) {
        int m = std::floor(double(low + high) / 2.0);
        if (lines.at(m).at(0) < search) {
            high = m;
        } else
            low = m - 1;
    }
    return high;
}

bool MapProcessor::canMove(const double& x1, const double& y1, const double& x2, const double& y2,
                           const nlohmann::json& geom, bool trigger) {

    double x = std::min(x1, x2);
    double y = std::min(y1, y2);
    double X = std::max(x1, x2);
    double Y = std::max(y1, y2);

    const auto& xLines = geom.at("x_lines");
    const auto& yLines = geom.at("y_lines");

    if (!trigger) {
        for (const auto& i : defs::hitbox) {
            double hitboxX = i.at(0);
            double hitboxY = i.at(1);
            if (!canMove(x1 + hitboxX, y1 + hitboxY, x2 + hitboxX, y2 + hitboxY, geom, true)) {
                return false;
            }
        }

        double pH = double(defs::base.at("h"));
        double nH = -double(defs::base.at("h"));

        double vn = double(defs::base.at("vn"));
        double v = -double(defs::base.at("v"));

        if (x2 > x1) {
            pH = -double(defs::base.at("h"));
            nH = double(defs::base.at("h"));
        }
        if (y2 > y1) {
            vn = -defs::base.at("v");
            v = defs::base.at("vn");
        }

        if (!canMove(x1 + nH, y1 + vn, x2 + nH, y2 + v, geom, true) ||
            !canMove(x1 + pH, y1 + v, x2 + nH, y2 + v, geom, true)) {
            return false;
        }

        return true;
    }

    for (unsigned long long i = bSearch(xLines, x); i < xLines.size(); i++) {
        if (i >= xLines.size())
            throw "Fail";
        auto& line = xLines.at(i);
        if (line.at(0).get<double>() == x2 &&
            ((line.at(1).get<double>() <= y2 && line.at(2).get<double>() >= y2) ||
             (line.at(0).get<double>() == x1 && y1 <= line.at(1).get<double>() && y2 > line.at(1).get<double>()))) {
            return false;
        }

        if (x > line.at(0).get<double>()) continue;
        if (X < line.at(0).get<double>()) break;

        double under = x2 - x1 + defs::REPS;
        if (under == 0)
            throw std::runtime_error("Blocked division by 0");
        double q = y1 + (y2 - y1) * (line.at(0).get<double>() - x1) / (under);
        if (!(line.at(1).get<double>() - defs::EPS <= q && q <= line.at(2).get<double>() + defs::EPS)) {
            continue;
        }
        return false;
    }

    for (unsigned long long i = bSearch(yLines, y); i < yLines.size(); i++) {
        if (i >= yLines.size()) throw "Fail";
        auto& line = yLines.at(i);
        if (line.at(0).get<double>() == y2 &&
            ((line.at(1).get<double>() <= x2 && line.at(2).get<double>() >= x2) ||
             (line.at(0).get<double>() == y1 && x1 <= line.at(1).get<double>() && x2 >= line.at(1).get<double>()))) {
            return false;
        }
        if (y > line.at(0).get<double>()) continue;
        if (Y < line.at(0).get<double>()) break;

        double under = y2 - y1 + defs::REPS;
        
        if (under == 0)
            throw std::runtime_error("Blocked division by 0");
        double q = x1 + (x2 - x1) * (line.at(0).get<double>() - y1) / (under);
        if (!(line.at(1).get<double>() - defs::EPS <= q && q <= line.at(2).get<double>() + defs::EPS)) {
            continue;
        }

        return false;
    }

    return true;
}

std::pair<double, double> MapProcessor::toGameMapCoords(const double& x, const double& y, const double& minX,
                                                        const double& minY) {
    return std::make_pair(x /* * boxSize */ + minX, y + minY);
}

unsigned long long MapProcessor::convertPosToMapIndex(const double& x, const double& y, const double& minX,
                                                      const double& minY, const double& xSize) {
    return xSize * (y - minY) + (x - minX);
}

void MapProcessor::processMaps(const GameData& data) {
    static std::map<std::string, int> base = {{"h", 8}, {"v", 7}, {"vn", 2}};
    mLogger->info("Preprocessing maps...");
    for (auto& [mapName, mapData] : data.at("maps").items()) {
        if (mapData.find("ignore") != mapData.end() && mapData.at("ignore") == true) {
            mLogger->info("Ignored map: {}", mapName);
            continue;
        }
        if (mapData.find("no_bounds") != mapData.end() && mapData.at("no_bounds") == true) {
            mLogger->info("Ignored boundless map: {}", mapName);
            continue;
        }
        mLogger->info("Processing {}...", mapName);
        auto& geom = data.at("geometry").at(mapName);
        auto& xLines = geom.at("x_lines"); // [x, y, endY]
        auto& yLines = geom.at("y_lines"); // [y. x, endX]

        double minX = geom.at("min_x").get<double>();
        double minY = geom.at("min_y").get<double>();
        double maxX = geom.at("max_x").get<double>();
        double maxY = geom.at("max_y").get<double>();

        double xSize = maxX - minX; // divided by boxSize if boxSize != 1
        double ySize = maxY - minY + 50;

        std::vector<bool> map;
        map.resize(xSize * ySize, 0);
        for (auto& line : xLines) {
            double x = line.at(0).get<double>() - minX;
            double yTop = line.at(1).get<double>() - minY;
            double yBot = line.at(2).get<double>() - minY;

            double x1 = x - base.at("h") - 1; // 1 = padding
            double y1 = yTop - base.at("vn") - 1;
            double x2 = x + base.at("h") + 1;
            double y2 = yBot + base.at("v") + 1;
            box(x1, y1, x2, y2, map, xSize);
        }

        for (auto& line : yLines) {
            double y = line.at(0).get<double>() - minY;
            double xLeft = line.at(1).get<double>() - minX;
            double xRight = line.at(2).get<double>() - minX;

            double x1 = xLeft - base.at("h") - 1; // 1 = padding
            double y1 = y - base.at("vn") - 1;
            double x2 = xRight + base.at("h") + 1;
            double y2 = y + base.at("v") + 1;
            box(x1, y1, x2, y2, map, xSize);
        }
        std::vector<Door> mapDoors;

        if (mapData.find("doors") != mapData.end()) {
            auto& doors = mapData.at("doors");
            for (auto& door : doors) {
                int spawn = door.at(5).get<int>();
                auto& tData = data.at("maps").at(door.at(4).get<std::string>()).at("spawns");

                int landingX = tData.at(spawn).at(0).get<int>();
                int landingY = tData.at(spawn).at(1).get<int>();
                std::string target = door.at(4).get<std::string>();

                Door d(door.at(0).get<int>(), door.at(1).get<int>(), {{target, std::pair{landingX, landingY}}}, {target},
                       false, {{target, spawn}});
                mapDoors.push_back(d);

                auto& ref = doorTransportMap[target];
                if (std::find(ref.begin(), ref.end(), mapName) == ref.end()) ref.push_back(mapName);
            }
        }

        if (mapData.find("npcs") != mapData.end() && mapData.at("npcs").size() > 0) {
            for (auto& npc : mapData.at("npcs")) {
                if (npc.at("id").get<std::string>() == "transporter") {
                    std::vector<std::string> doorTargets;
                    auto& places = data.at("npcs").at("transporter").at("places");
                    std::map<std::string, std::pair<int, int>> positions;
                    std::map<std::string, int> spawns;

                    for (auto& [place, _] : places.items()) {
                        auto& ref = doorTransportMap[place];
                        if (place == mapName) continue;
                        if (std::find(ref.begin(), ref.end(), mapName) == ref.end()) ref.push_back(mapName);

                        auto spawn = data.at("npcs").at("transporter").at("places").at(place).get<int>();
                        spawns["place"] = spawn;

                        auto& doorSpawns = data.at("maps").at(place).at("spawns").at(spawn);
                        positions[place] = std::make_pair(doorSpawns.at(0), doorSpawns.at(1));

                        doorTargets.push_back(place);
                    }
                    auto& position = npc.at("position");
                    Door d(position.at(0).get<int>(), position.at(1).get<int>(), positions, doorTargets, true, spawns);
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
    const static std::vector<std::vector<int>> moves 
        = {
            {7, 0}, {0, 7}, 
            {-7, 0}, {0, -7},
            {7, 7}, {-7, -7},
            {-7, 7}, {7, -7}
        };

    std::vector<std::pair<int, int>> pos;
    for (auto& move : moves) {
        int mx = x + move.at(0);
        int my = y + move.at(1);

        pos.push_back(std::pair<int, int>(mx, my));
        
    }

    return pos;
}

bool MapProcessor::isTransportDestination(std::string target, GameData& data) {
    // Places is a map containing the destination key, and a value passed as s when using
    // the transport websocket call. I have no idea what the variable signifies.
    auto& c = data.at("npcs").at("transporter").at("places");
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

std::vector<std::pair<int, int>> MapProcessor::prunePath(const std::vector<std::pair<int, int>>& rawPath,
                                                         const nlohmann::json& geom) {
    auto oldPos = rawPath[0];
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
            if (tmp == oldPos) 
                throw std::runtime_error("Invalid move position: " + std::to_string(currPos.first) + "," + std::to_string(currPos.second));

            path.push_back(tmp);
            oldPos = tmp;
            goto retry;

        } else if (i == rawPath.size() - 1)
            path.push_back(currPos);
    }
    return path;
}

bool MapProcessor::dijkstra(PlayerSkeleton& player, SmartMoveHelper& smart, std::string currMap) {
    if (!smart.canRun()) return false;
    auto& character = player.getCharacter();
    if (smart.getTargetMap() == character->getMap() &&
        canMove(character->getX(), character->getY(), smart.getTargetX(), smart.getTargetY(),
                player.getGameData()["geometry"][character->getMap()]) 
            ) {
        smart.pushNodes({std::make_pair(smart.getTargetX(), smart.getTargetY())});
        return true;
    }
    std::vector<std::tuple<int, int, std::string>> targets;
    if (currMap == "") currMap = character->getMap();

    if (currMap == smart.getTargetMap() || (smart.hasMultipleDestinations() && currMap == smart.getProcessableMap())) {
        int x, y;
        std::pair<int, int> rCurrPos;

        auto landingCoords = smart.getLandingCoords();

        if (landingCoords.has_value()) {
            rCurrPos = std::make_pair(landingCoords->first, landingCoords->second);
        } else if (character->getMap() == currMap)
            rCurrPos = std::make_pair(character->getX(), character->getY());
        std::optional<Door> door = std::nullopt;
        std::optional<std::string> targetMap = std::nullopt;
        std::optional<std::pair<int, int>> landingPosition = std::nullopt;

        if (currMap == smart.getTargetMap()) {
            x = smart.getTargetX();
            y = smart.getTargetY();

        } else {
            targetMap = smart.getNextProcessableMap();
            door = getDoorTo(currMap, targetMap.value(), character->getClient().getData());
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
        auto& map = this->maps[currMap];

        PathDist start(currPos, 0);
        PathDist end(targetPos, 0, true);
        std::priority_queue<PathDist> unvisited;
        std::map<std::pair<int, int>, float> costs;
        std::map<std::pair<int, int>, std::pair<int, int>> paths;
        unvisited.push(start);
        const auto& geom = character->getClient().getData().at("geometry").at(currMap); 
        while (!unvisited.empty()) {
            PathDist currNode = unvisited.top();

            if (currNode == end) {
                
                auto pos = currNode.position;
                std::vector<std::pair<int, int>> path;
                if (!end.door)
                    path.push_back(end.position);

                while (!(pos == start.position)) {
                    path.push_back(pos);
                    pos = paths.at(pos); 

                }

                std::reverse(path.begin(), path.end());
                if (path.size() == 0)
                    throw std::runtime_error("Path is empty?");
                auto vec = prunePath(path, geom);
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

                if (character->getMap() == smart.getTargetMap()) {
                    smart.ready();
                }
                return true;
            }
            unvisited.pop();
            auto nearby = getAdjacentPixels(currNode.position.first, currNode.position.second, map);
            float hCost; 

            for (auto& neighbor : nearby) {
                if(!canMove(currNode.position.first, currNode.position.second, neighbor.first, neighbor.second, geom))
                    continue;
                float newCost = costs[currNode.position];

                if (newCost < maps::get(costs, neighbor, std::numeric_limits<float>::infinity())) {
                    hCost = MovementMath::pythagoras(neighbor.first, neighbor.second, end.position.first, end.position.second);
                    float priority = newCost + hCost;
                    unvisited.push(PathDist(neighbor, priority));

                    costs[neighbor] = newCost;
                    paths[neighbor] = currNode.position;
                }
            }

        }
        return false;
    } else {
        std::vector<std::string> path = doorDijkstra(currMap, smart.getTargetMap());
        if (path.size() == 0) {
            mLogger->warn("Failed to find a door path from {} to {}", currMap, smart.getTargetMap());
            smart.deinit();
            return false;
        }
        smart.injectDoorPath(path);
        for (auto& mapToProcess : path)
            if (!dijkstra(player, smart, mapToProcess)) {
                if (smart.canRun()) smart.deinit();
                return false;
            }
        smart.ready();
    }
    return true;
}

std::vector<std::string> MapProcessor::doorDijkstra(std::string from, std::string to) {
    std::vector<std::string> visited;
    std::vector<std::string> unvisited;
    std::unordered_map<std::string, std::pair<int, std::string>> dists;

    std::string currPos = from;
    dists["from"] = std::make_pair(0, "");
    std::vector<std::string> route = {to};
    // Dijkstra
    while (true) {
        std::vector<std::string>& nearby = doorTransportMap[currPos];
        for (std::string& pos : nearby) {
            if (std::find(visited.begin(), visited.end(), pos) == visited.end() &&
                std::find(unvisited.begin(), unvisited.end(), pos) == unvisited.end()) {
                unvisited.push_back(pos);
                int dist = dists[currPos].first + 1;
                if (dists.find(pos) == dists.end() || dist < dists.at(pos).first)
                    dists[pos] = std::make_pair(dist, currPos);
            }
        }

        auto unv = std::find(unvisited.begin(), unvisited.end(), currPos);
        if (unv != unvisited.end()) unvisited.erase(unv);
        visited.push_back(currPos);

        if (currPos == to) {
            auto prePos = dists.at(to);

            while (prePos.second != "") {
                route.push_back(prePos.second);
                prePos = dists.at(prePos.second);
            }
            std::reverse(route.begin(), route.end());
            return route;
        }
        if (unvisited.size() == 0) return {};
        std::sort(unvisited.begin(), unvisited.end(), [dists](std::string& one, std::string& two) mutable -> bool {
            return (dists.at(one).first) < (dists.at(two).first);
        });
        currPos = unvisited.at(0);
    }
}

} // namespace advland

