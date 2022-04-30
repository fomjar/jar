

#ifndef _JAR_EVENT_H
#define _JAR_EVENT_H

#include "any.h"
#include "exec.h"

#include <map>

namespace jar {


template <typename _Tp>
class event_queue {

public:
    event_queue() : callbacks(), mutex(), queuer() {
        this->queuer.start();
    }

public:
    template <typename ... ARG>
    void sub(const _Tp & event, const func_v<ARG...> & callback) {
        JAR_EXEC_LOCK_GUARD

        if (this->callbacks.find(event) == this->callbacks.end())
            this->callbacks[event] = std::vector<any>();

        any a = std::move(callback); // make a copy
        this->callbacks[event].push_back(a);
    }

    template <typename ... ARG>
    void pub(const _Tp & event, const ARG & ... args) {
        this->queuer.submit((func_vv) [this, event, args...] {
            JAR_EXEC_LOCK_GUARD
            for (auto a : this->callbacks[event]) {
                auto callback = a.template cast<func_v<ARG...>>();
                callback(args...);
            }
        });
    }


private:
    std::map<_Tp, std::vector<any>> callbacks;
    std::mutex  mutex;
    queuer      queuer;

};

} // namespace jar


#endif // _JAR_EVENT_H

