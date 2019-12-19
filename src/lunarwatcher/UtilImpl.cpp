#include "objects/Map.hpp"
#include "lunarwatcher/movement/MapProcessing.hpp"
#include "utils/ThreadingUtils.hpp"
#include "uv.h"

namespace advland {

bool Map::isOpen(double x, double y) {
    int idx = MapProcessor::convertPosToMapIndex(x, y, minX, minY, xSize);
    return !rawMapData[idx];
}

}
