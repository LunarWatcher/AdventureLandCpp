#include "objects/Map.hpp"
#include "lunarwatcher/movement/MapProcessing.hpp"

bool advland::Map::isOpen(double x, double y) {
    int idx = MapProcessor::convertPosToMapIndex(x, y, minX, minY, xSize);
    return !rawMapData[idx];
}
