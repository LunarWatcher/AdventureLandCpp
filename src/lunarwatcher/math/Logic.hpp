#ifndef LUNARWATCHER_ADVLAND_LOGIC_HPP
#define LUNARWATCHER_ADVLAND_LOGIC_HPP

#include <nlohmann/json.hpp>

namespace advland {

class MovementMath {
public:
    static void stopLogic(nlohmann::json& entity) {
        if (entity["moving"] == false)
            return;
        double x = entity["x"];
        double y = entity["y"];
        double goingX = entity["going_x"];
        double goingY = entity["going_y"];
        double fromX = entity["from_x"];
        double fromY = entity["from_y"];

        if (((fromX <= goingX && x >= goingX - 0.1) || (fromX >= goingX && x <= goingX + 0.1)) && ((fromY <= goingY && y >= goingY - 0.1) || (fromY >= goingY && y <= goingY + 0.1))) {
            entity["x"] = goingX;
            entity["y"] = goingY;

            entity["moving"] = entity.value("amoving", false);

        }
    }

    static void moveEntity(nlohmann::json& entity, double cDelta) {
    
        if (entity.value("moving", false) == true) {
            entity["x"] = double(entity["x"]) + entity.value("vx", 0) * std::min(cDelta, 50.0) / 1000.0;
            entity["y"] = double(entity["y"]) + entity.value("vy", 0) * std::min(cDelta, 50.0) / 1000.0;
        }
    }
    static std::pair<int, int> calculateVelocity(const nlohmann::json& entity) { 
        double ref = std::sqrt(0.0001 + std::pow(entity["going_x"].get<double>() - entity["from_x"].get<double>(), 2) +
                             std::pow(entity["going_y"].get<double>() - entity["from_y"].get<double>(), 2)); 
        auto speed = entity["speed"].get<double>();
        int vx = speed * (entity["going_x"].get<double>() - entity["from_x"].get<double>()) / ref;
        int vy = speed * (entity["going_y"].get<double>() - entity["from_y"].get<double>()) / ref;
        
        return std::make_pair(vx, vy);
    }

    static double pythagoras(double x1, double y1, double x2, double y2) {
        return std::sqrt(
                std::pow(x1 - x2, 2) +
                std::pow(y1 - y2, 2));
    }
};

}
#endif 
