#include "utils/SocketIOParser.hpp"
#include "objects/Map.hpp"
#include "lunarwatcher/movement/MapProcessing.hpp"

void advland::onMessage(std::string& message, advland::SocketWrapper& wrapper) {
    wrapper.sendPing(); 
}

std::string advland::createMessage(int type, std::string& message) {
    return std::to_string(type) + message;
}
bool advland::Map::isOpen(double x, double y) {
    int idx = MapProcessor::convertPosToMapIndex(x, y, minX, minY, xSize);
    return !rawMapData[idx];
}
