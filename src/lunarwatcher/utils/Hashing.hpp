#ifndef LUNARWATCHER_ADVLAND_HASHING_HPP
#define LUNARWATCHER_ADVLAND_HASHING_HPP
#include <utility>

namespace advland {

struct PairHash {
    template <class T1, class T2>
    std::size_t operator () (const std::pair<T1,T2> &p) const {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);
        h1 <<= sizeof(uintmax_t) * 4;
        
        return h1 ^ h2;
    }
};

}

#endif
