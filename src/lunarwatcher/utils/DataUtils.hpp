#ifndef LUNARWATCHER_ADVLAND_DATAUTILS_HPP
#define LUNARWATCHER_ADVLAND_DATAUTILS_HPP

#include "nlohmann/json.hpp"
#include <fstream>
#include "lunarwatcher/meta/Exceptions.hpp"

namespace advland {

namespace ConfigHelper {

    /**
     * Helper method: Loads a JSON object from a file. You can use this to load your own JSON config 
     * to create modifiable bots without having to recompile.
     *
     * @param filename    Self-explanatory: the file to load. 
     * @param out         The JSON object to load into. Should be an empty/newly initialized instance of nlohmann::json.
     */
    inline void loadJsonFromFile(const std::string& fileName, nlohmann::json& out) {
        std::ifstream s(fileName);
        if(!s.is_open()) {
            throw IOException("Failed to find the file \"" + fileName + "\"");
        }

        s >> out;
    }

} 

} // namespace advland

#endif
