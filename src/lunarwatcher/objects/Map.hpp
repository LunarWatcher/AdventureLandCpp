#ifndef LUNARWATCHER_ADVLAND_MAP
#define LUNARWATCHER_ADVLAND_MAP

#include <string>
#include <vector>

namespace advland {

class Map {
private:
    std::vector<bool> rawMapData;
    double minX, minY;
    double xSize, ySize;
    std::string mapName;

public:
    Map() {}
    Map(std::vector<bool>& rawMapData, double& minX, double& minY, double& xSize, double& ySize, std::string mapName)
            : rawMapData(rawMapData), minX(minX), minY(minY), xSize(xSize), ySize(ySize), mapName(mapName) {}

    bool isOpen(double x, double y);

    double getMinX() { return minX; }
    double getMinY() { return minY; }
    double getXSize() { return xSize; }
    double getYSize() { return ySize; }
    std::string getMapName() { return mapName; }
    const std::vector<bool> getRawMapData() { return rawMapData; }
    
};

} // namespace advland

#endif
