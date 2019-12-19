#ifndef LUNARWATCHER_MAPUTILS_HPP
#define LUNARWATCHER_MAPUTILS_HPP

#include <map>
#include <unordered_map>

namespace advland {

namespace maps {

template <typename R, typename K, typename H>
inline R get(const std::unordered_map<K, R, H>& map, const K& key, const R& fallback) {
    if (map.find(key) == map.end()) 
        return fallback;
    return map.at(key);
}

template <typename R, typename K> 
inline R get(const std::map<K, R>& map, const K& key, const R& fallback) {
    if(map.find(key) == map.end())
        return fallback;
    return map.at(key);
}

}

} // namespace lunarwatcher

#endif
