#ifndef LUNARWATCHER_ADVLAND_MAP
#define LUNARWATCHER_ADVLAND_MAP

#include "lunarwatcher/meta/Exceptions.hpp"
#include <algorithm>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace advland {

class Door {
private:
    int x, y;

    std::vector<std::string> targetMaps;
    bool transporter;
    // Used with the websocket event
    std::map<std::string, int> spawns;
    std::map<std::string, std::pair<int, int>> landingPositions;

public:
    Door(int x, int y, const std::map<std::string, std::pair<int, int>>& landingPositions,
         std::vector<std::string> targetMaps, bool transporter, const std::map<std::string, int>& spawns)
            : x(x), y(y), targetMaps(targetMaps), transporter(transporter), spawns(spawns),
              landingPositions(landingPositions) {}

    int& getX() { return x; }
    int& getY() { return y; }

    std::pair<int, int>& getLandingPosition(std::string map = "") {
        if (!transporter) return landingPositions[targetMaps[0]];
        if (map == "") throw IllegalArgumentException("Can't pass an empty string to a transporter.");
        return landingPositions[map];
    }
    std::vector<std::string>& getTargetMap() { return targetMaps; }

    bool isTarget(std::string& possibleDest) {
        return std::find(targetMaps.begin(), targetMaps.end(), possibleDest) != targetMaps.end();
    }

    bool& isTransporter() { return transporter; }
    const int& getSpawn(std::string targetMap = "") {
        if (!transporter) return spawns[targetMaps[0]];
        if (targetMap == "") throw IllegalArgumentException("Can't pass an empty string to a transporter");
        return spawns[targetMap];
    }
};

class Map {
private:
    std::vector<bool> rawMapData;
    std::vector<Door> doors;
    std::optional<Door> transporter;

    double minX, minY;
    double maxX, maxY;
    double xSize, ySize;
    std::string mapName;

public:
    Map() {}
    Map(std::vector<bool>& rawMapData, std::vector<Door> doors, double& minX, double& minY, double& maxX, double& maxY,
        double& xSize, double& ySize, std::string mapName)
            : rawMapData(rawMapData), doors(doors), minX(minX), minY(minY), maxX(maxX), maxY(maxY), xSize(xSize),
              ySize(ySize), mapName(mapName) {
        for (auto& door : doors) {
            if (door.isTransporter()) {
                transporter = door;
                break;
            }
        }
    }

    bool isOpen(double x, double y);

    double getMinX() { return minX; }
    double getMinY() { return minY; }
    double getMaxX() { return maxX; }
    double getMaxY() { return maxY; }
    double getXSize() { return xSize; }
    double getYSize() { return ySize; }
    std::string getMapName() { return mapName; }
    const std::vector<bool>& getRawMapData() { return rawMapData; }
    std::vector<Door>& getDoors() { return doors; }

    bool hasTransporter() { return transporter.has_value(); }
    std::optional<Door> getTransporter() { return transporter; }
};

} // namespace advland

#endif
