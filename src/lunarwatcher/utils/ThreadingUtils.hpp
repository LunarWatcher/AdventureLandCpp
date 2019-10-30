#ifndef LUNARWATCHER_ADVLAND_THREADINGUTILS_HPP
#define LUNARWATCHER_ADVLAND_THREADINGUTILS_HPP

#include "uvw/async.hpp"
#include "uvw/loop.hpp"
#include "uvw/timer.hpp"
#include "uvw/work.hpp"

#include <functional>
#include <memory>

namespace advland {

class LoopHelper {
private:
    std::shared_ptr<uvw::Loop> loop;

public:
    using TimerCallback = std::function<void(const uvw::TimerEvent&, uvw::TimerHandle&)>;
    using Millis = std::chrono::milliseconds;

    LoopHelper() { loop = uvw::Loop::create(); }

    std::shared_ptr<uvw::TimerHandle> setTimeout(TimerCallback callback, int timeout) {
        auto timer = loop->resource<uvw::TimerHandle>();
        timer->on<uvw::TimerEvent>([callback](const uvw::TimerEvent& event, auto& handle) {
            callback(event, handle);
            handle.close();
            handle.stop();
        });
        timer->start(Millis(timeout), Millis(0));
        return timer;
    }

    /**
     * Set an interval.
     *
     * @param callback    The function to call
     * @param interval    The rate to call the function at, in milliseconds
     * @param timeout     The timeout before the first call
     * @returns           A timer instance you can access. Note that using it isn't required. The handle is also passed
     *                    to the callback as the second parameter
     */
    std::shared_ptr<uvw::TimerHandle> setInterval(TimerCallback callback, int interval, int timeout = -1) {
        if (timeout < 0) timeout = interval;

        auto timer = loop->resource<uvw::TimerHandle>();
        timer->on<uvw::TimerEvent>(callback);
        timer->start(Millis(timeout), Millis(interval));

        return timer;
    }

    /**
     * Creates a job that runs in a separate thread.
     *
     * @param callback   The function to call
     * @returns          A canceleable WorkReq
     */
    std::shared_ptr<uvw::WorkReq> createJob(std::function<void()> callback) {
        auto job = loop->resource<uvw::WorkReq>(callback);
        job->queue();
        return job;
    }

    /**
     * This uses libuv's async feature to connect to the main thread.
     * This MUST be used to do anything that interacts with the loop, like adding a new timeout or creating a new
     * interval.
     *
     * Failing to use this may lead to various problems and incorrect behavior (not necessarily undefined, but I'm not
     * sure). For an instance, you might end up with
     */
    void exec(std::function<void(const uvw::AsyncEvent& event, uvw::AsyncHandle& handle)> callback) {
        auto asyncHandle = loop->resource<uvw::AsyncHandle>();
        asyncHandle->on<uvw::AsyncEvent>([callback](const auto& evt, auto& handle) {
            callback(evt, handle);
            handle.close();
        });
        asyncHandle->send();
    }

    void run() { loop->run(); }

    /**
     * This method should only be used for extensions on uvw not defined by this class.
     */
    std::shared_ptr<uvw::Loop> getLoop() { return loop; }
};

} // namespace advland

#endif
