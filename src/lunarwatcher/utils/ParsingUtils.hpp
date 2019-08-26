#ifndef LUNARWATCHER_ADVLAND_PARSINGUTILS_HPP
#define LUNARWATCHER_ADVLAND_PARSINGUTILS_HPP

#include <nlohmann/json.hpp>
#include <iostream>

namespace advland {

template <typename T, typename K>
inline T getOrElse(const nlohmann::json& n, K key,  T defaultValue) {
    if (!n.is_object()) {
        std::cerr << "ERROR: JSON not an object for " << n.dump() << std::endl;
        return defaultValue;
    }
    if (n.find(key) == n.end()) return defaultValue;
    if (n[key].is_null()) return defaultValue;
    return n[key].template get<T>(); 
}

}

#endif 
