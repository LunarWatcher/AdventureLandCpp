#ifndef LUNARWATCHER_ADVLAND_TIMER_HPP
#define LUNARWATCHER_ADVLAND_TIMER_HPP

#include <chrono>

namespace advland {

namespace Types {
    typedef std::chrono::high_resolution_clock Clock;
    typedef std::chrono::time_point<Clock> TimePoint;
}

class Timer{
private:

    Types::TimePoint start; 
public:
    /**
     * @param init   Whether to initialize the timepoint with the current time or not.
     */
    Timer(bool init = false) {
        if (init) {
            start = Types::Clock::now();
        } 
    }

    /**
     * returns the time since the last reset
     */
    double check() {
        Types::TimePoint now = Types::Clock::now(); 
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
    }
    
    /**
     * @param hard  Whether to set the start time to the current time, or the last check()ed type
     */
    void reset() {
        start = Types::Clock::now();
    }
    
};
}
#endif 
