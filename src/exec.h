
#ifndef _JAR_EXEC_H
#define _JAR_EXEC_H

#include "defs.h"

#include <functional>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <future>


namespace jar {


template <typename RET, typename ... ARG>
using func      = std::function<RET(ARG...)>;
template <typename ... ARG>
using func_v    = func<void, ARG...>;

using func_vv   = func<void>;



class executor {

#define LOCK_TASKS \
            std::lock_guard<std::mutex> guard(this->mutex);

public:
    executor() :
        tasks(),
        mutex(),
        condition(),
        _is_run(false),
        thread(nullptr) { };
    virtual ~executor() { this->stop(); }

    bool is_run() const { return this->_is_run; }

    void start() {
        if (this->is_run())
            return;
        
        this->_is_run = true;
        this->thread = new std::thread(this->worker());
    }

    void stop() {
        if (!this->is_run())
            return;

        this->_is_run = false;
        this->clear();
        if (this->thread) {
            this->condition.notify_all();
            this->thread->join();
            delete this->thread;
            this->thread = nullptr;
        }
    }

    void clear() {
        LOCK_TASKS
        this->tasks.clear();
        this->tasks.shrink_to_fit();
    }
    
    template <typename RET, typename ... ARG>
    std::promise<RET> submit(const func<RET, ARG...> & task, const ARG & ... args) {
        std::promise<RET> prom;
        LOCK_TASKS
        this->tasks.push_back([=, &prom] { prom.set_value(task(args...)); });
        this->condition.notify_all();
        return prom;
    }
    
    template <typename ... ARG>
    void submit(const func_v<ARG...> & task, const ARG & ... args) {
        LOCK_TASKS
        this->tasks.push_back([=] { task(args...); });
        this->condition.notify_all();
    }

protected:
    virtual func_vv worker() = 0;

    std::vector<func_vv>    tasks;
    std::mutex              mutex;
    std::condition_variable condition;

private:
    bool                    _is_run;
    std::thread           * thread;    

};

using exec = executor;

class queuer : public exec { 

protected:
    func_vv worker() override {
        return [this] {
            const long CHECK_MILLIS = 1 * 1000;
            while (this->is_run()) {
                if (this->tasks.empty()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(CHECK_MILLIS));
                    continue;
                }
                {
                    LOCK_TASKS
                    for (auto task : this->tasks)
                        task();
                }
                this->clear();
            }
        };
    }

};


} // namespace jar


#endif // _JAR_EXEC_H
