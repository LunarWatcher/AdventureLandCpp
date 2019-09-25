#ifndef LUNARWATCHER_ADVLAND_THREADINGUTILS_HPP
#define LUNARWATCHER_ADVLAND_THREADINGUTILS_HPP

#include <uv.h>

namespace advland {

class ThreadHelper {
public:
    void test() {
        uv_timer_t timer;
        uv_timer_init(uv_default_loop(), &timer);
        uv_timer_start(&timer, [](uv_timer_t* handle) {

        }, 5000, 5000);
    }

};

}

#endif 
