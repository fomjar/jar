/**
 * @file event.h
 * @author fomjar (fomjar@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2022-04-30
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef _JAR_EVENT_H
#define _JAR_EVENT_H

#include "any.h"
#include "exec.h"

#include <map>
#include <type_traits>

namespace jar {


/**
 * @brief 事件队列。订阅和发布自定义消息，可以带上自定义参数。事件的分发执行委托给了内部的异步队列。
 * 
 * @tparam _Tp 
 * 
 * @author fomjar
 * @date 2022/04/30
 */
template <typename _Tp>
class event_queue {

public:
    event_queue() : callbacks(), mutex(), quer() {
        this->quer.start();
    }

public:
    template <typename ... _Ap>
    void sub(const _Tp & event, const func_v<_Ap...> & callback) {
        JAR_EXEC_LOCK_GUARD

        if (this->callbacks.find(event) == this->callbacks.end())
            this->callbacks[event] = std::vector<any>();

        any a = std::move(callback); // make a copy
        this->callbacks[event].push_back(a);
    }

    template <typename ... _Ap>
    void pub(const _Tp & event, const _Ap & ... args) {
        this->quer.submit((func_vv) [this, event, args...] {
            JAR_EXEC_LOCK_GUARD
            for (auto a : this->callbacks[event]) {
                auto callback = a.template cast<func_v<_Ap...>>();
                callback(args...);
            }
        });
    }


private:
    std::map<_Tp, std::vector<any>> callbacks;
    std::mutex  mutex;
    queuer      quer;

};




extern event_queue<uint64_t> event;


/**
 * @brief 订阅一个事件。从主事件队列。
 * 
 * @tparam _Ap 
 * @param e 
 * @param func 
 * 
 * @author fomjar
 * @date 2022/04/30
 */
template <typename ... _Ap>
inline void sub(const uint64_t & e, const func_v<_Ap...> & func) {
    event.sub(std::forward<const uint64_t>(e), std::forward<const func_v<_Ap...>>(func));
}

/**
 * @brief 发布一个事件。到主事件队列。
 * 
 * @tparam _Ap 
 * @param e 
 * @param args 
 * 
 * @author fomjar
 * @date 2022/04/30
 */
template <typename ... _Ap>
inline void pub(const uint64_t & e, const _Ap & ... args) {
    event.pub(std::forward<const uint64_t>(e), std::forward<const _Ap>(args)...);
}


} // namespace jar


#endif // _JAR_EVENT_H

