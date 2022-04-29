
#ifndef _JAR_EXEC_H
#define _JAR_EXEC_H


#include "time.h"

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



/**
 * @brief 异步执行器。
 * 
 * @see queuer
 * @see delayer
 * @see looper
 * @see animator
 * 
 * @author fomjar
 * @date 2022/04/30
 */
class executor {


#define JAR_EXEC_LOCK_GUARD \
            std::lock_guard<std::mutex> guard(this->mutex);

#define JAR_EXEC_LOCK_WAIT_FOR(duration) \
            { \
                std::unique_lock<std::mutex> lock(this->mutex); \
                this->condition.wait_for(lock, duration); \
            }

#define JAR_EXEC_EXECUTE_TASKS \
            { \
                for (auto task : this->tasks) { \
                    task(); \
                } \
            }


public:
    executor() :
        tasks(),
        mutex(),
        condition(),
        _is_running(false),
        thread(nullptr) { };
    virtual ~executor() { this->stop(); }

    bool    is_running()    const { return this->_is_running; }
    size_t  size()          const { return this->tasks.size(); }
    bool    is_idle()       const { return this->tasks.empty(); }

    /**
     * @brief 启动线程。
     */
    void start() {
        if (this->is_running())
            return;
        
        this->thread = new std::thread([this] {
            this->_is_running = true;
            this->worker()();
            this->_is_running = false;
        });
    }

    /**
     * @brief 停止线程。清空任务。
     */
    void stop() {
        this->_is_running = false;
        this->clear();
        if (this->thread) {
            this->condition.notify_all();
            this->thread->join();
            delete this->thread;
            this->thread = nullptr;
        }
    }

    /**
     * @brief 清空任务。
     */
    void clear() {
        JAR_EXEC_LOCK_GUARD
        this->tasks.clear();
        this->tasks.shrink_to_fit();
    }
    
    /**
     * @brief 提交任务。执行方式和时机取决于实现。
     * 
     * @tparam RET 
     * @tparam ARG 
     * @param task 
     * @param args 
     * @return std::promise<RET> 
     */
    template <typename RET, typename ... ARG>
    std::promise<RET> submit(const func<RET, ARG...> & task, const ARG & ... args) {
        std::promise<RET> prom;
        JAR_EXEC_LOCK_GUARD
        this->tasks.push_back([=, &prom] { prom.set_value(task(args...)); });
        this->condition.notify_all();
        return prom;
    }
    
    /**
     * @brief 提交任务。执行方式和时机取决于实现。
     * 
     * @tparam ARG 
     * @param task 
     * @param args 
     */
    template <typename ... ARG>
    void submit(const func_v<ARG...> & task, const ARG & ... args) {
        JAR_EXEC_LOCK_GUARD
        this->tasks.push_back([=] { task(args...); });
        this->condition.notify_all();
    }

protected:
    virtual func_vv worker() = 0;

    std::vector<func_vv>    tasks;
    std::mutex              mutex;
    std::condition_variable condition;

private:
    bool            _is_running;
    std::thread   * thread;    

};

using exec = executor;



/**
 * @brief 异步执行队列。按任务提交顺序异步执行任务。
 * 
 * @see exec
 * 
 * @author fomjar
 * @date 2022/04/30
 */
class queuer : public exec { 

protected:
    func_vv worker() override {
        return [this] {
            const long CHECK_SECONDS = 1;
            while (this->is_running()) {
                if (this->is_idle()) {
                    JAR_EXEC_LOCK_WAIT_FOR(std::chrono::seconds(CHECK_SECONDS))
                    continue;
                }
                JAR_EXEC_LOCK_GUARD
                JAR_EXEC_EXECUTE_TASKS
                this->tasks.clear();
            }
        };
    }

};



/**
 * @brief 延时异步执行器。可以设置执行前的定时时长。
 * 
 * @see exec
 * 
 * @author fomjar
 * @date 2022/04/30
 */
class delayer : public exec {

public:
    template <class _Rep, class _Period>
    delayer(const std::chrono::duration<_Rep, _Period> & duration = std::chrono::seconds(1)) :
        duration(std::chrono::duration_cast<std::chrono::microseconds>(duration)) { }

public:
    template <class _Rep, class _Period>
    void set_delay(const std::chrono::duration<_Rep, _Period> & duration)
        { this->duration = std::chrono::duration_cast<std::chrono::microseconds>(duration); }

protected:
    func_vv worker() override {
        return [this] {
            JAR_EXEC_LOCK_WAIT_FOR(this->duration);
            if (this->is_running() && !this->is_idle()) {
                JAR_EXEC_LOCK_GUARD
                JAR_EXEC_EXECUTE_TASKS
            }
        };
    }

private:
    std::chrono::microseconds duration;

};



/**
 * @brief 循环异步执行器。可以设置循环间隔时长。
 * 
 * @see exec
 * 
 * @author fomjar
 * @date 2022/04/30
 */
class looper : public exec {

public:
    template <class _Rep, class _Period>
    looper(const std::chrono::duration<_Rep, _Period> & interval = std::chrono::seconds(1)) :
        interval(std::chrono::duration_cast<std::chrono::microseconds>(interval)) { }

public:
    template <class _Rep, class _Period>
    void set_interval(const std::chrono::duration<_Rep, _Period> & interval)
        { this->interval = std::chrono::duration_cast<std::chrono::microseconds>(interval); }

protected:
    func_vv worker() override {
        return [this] {
            while (this->is_running()) {
                JAR_EXEC_LOCK_WAIT_FOR(this->interval);
                if (this->is_running() && !this->is_idle()) {
                    JAR_EXEC_LOCK_GUARD
                    JAR_EXEC_EXECUTE_TASKS
                }
            }
        };
    }

private:
    std::chrono::microseconds interval;

};



/**
 * @brief 动画引擎。以固定频率执行任务，与looper的区别在于，它以绝对的间隔时间来执行任务，因为它将任务的执行时间也计算在内。
 * 
 * @see exec
 * 
 * @author fomjar
 * @date 2022/04/30
 */
class animator : public exec {

public:
    animator(float frequency = 24.0f) : frequency(frequency) { }

public:
    void set_frequency(float frequency) { this->frequency = frequency; }

protected:
    func_vv worker() override {
        return [this] {
            while (this->is_running()) {
                auto beg = now();
                {
                    JAR_EXEC_LOCK_GUARD
                    JAR_EXEC_EXECUTE_TASKS
                }
                auto end = now();
                auto cost = end - beg;
                auto interval = 1000000LL / this->frequency;
                if (cost >= interval)
                    std::this_thread::yield();
                else
                    JAR_EXEC_LOCK_WAIT_FOR(std::chrono::microseconds((long long) (interval - cost)));
            }
        };
    }

private:
    float frequency;

};



/**
 * @brief 线程池。内部的子线程均为queuer的实现。
 * 
 * @author fomjar
 * @date 2022/04/30
 */
class executor_pool {

public:
    executor_pool() : execs(), mutex() { }
    ~executor_pool() { this->stop(); }
    
public:
    size_t size() const { return this->execs.size(); }

    /**
     * @brief 停止所有运行中的线程。
     */
    void stop() {
        JAR_EXEC_LOCK_GUARD
        for (auto exec : this->execs) {
            exec->stop();
            delete exec;
        }
        this->execs.clear();
        this->execs.shrink_to_fit();
    }

    /**
     * @brief 保留至少给定大小的线程数量。不足将自动补充，超过则忽略。
     * 
     * @param size 
     */
    void reserve(size_t size) {
        if (this->size() >= size) return;

        JAR_EXEC_LOCK_GUARD
        while (this->size() < size) {
            auto exec = new queuer;
            exec->start();
            this->execs.push_back(exec);
        }
    }

    /**
     * @brief 尽量收缩至指定大小。将自动释放空闲线程。
     * 
     * @param size 
     */
    void shrink(size_t size) {
        if (this->size() <= size) return;

        JAR_EXEC_LOCK_GUARD
        while (this->size() > size) {
            bool has_idle = false;
            for (auto i = this->execs.end() - 1; i != this->execs.begin() - 1; i--) {
                if ((*i)->is_idle()) {
                    has_idle = true;
                    (*i)->stop();
                    delete (*i);
                    this->execs.erase(i);
                    break;
                }
            }
            if (!has_idle) break;
        }
    }

    /**
     * @brief 提交任务。执行方式和时机取决于实现。
     * 
     * @tparam RET 
     * @tparam ARG 
     * @param task 
     * @param args 
     * @return std::promise<RET> 
     */
    template <typename RET, typename ... ARG>
    std::promise<RET> submit(const func<RET, ARG...> & task, const ARG & ... args) {
        return this->choose()->submit(task, args...);
    }
    
    /**
     * @brief 提交任务。执行方式和时机取决于实现。
     * 
     * @tparam ARG 
     * @param task 
     * @param args 
     */
    template <typename ... ARG>
    void submit(const func_v<ARG...> & task, const ARG & ... args) {
        this->choose()->submit(task, args...);
    }

protected:
    virtual exec * choose() = 0;

    std::vector<exec *> execs;
    std::mutex          mutex;
};

using pool = executor_pool;


/**
 * @brief 固定大小的线程池实现。当没有空闲线程时，任务将派发给任务数量最少的线程，排队等待执行。
 * 
 * @see pool
 * 
 * @author fomjar
 * @date 2022/04/30
 */
class fixed_pool : public pool {

public:
    fixed_pool(size_t fixed_size = 4) {
        this->reserve(fixed_size);
    }

protected:
    exec * choose() override {
        if (0 == this->size())
            return nullptr;
        
        auto e = this->execs.front();
        for (auto exec : this->execs) {
            if (exec->size() < e->size())
                e = exec;
        }
        return e;
    }

};


/**
 * @brief 缓冲大小的线程池实现。当没有空闲线程时自动创建新线程，当线程空闲超过1分钟时会自动释放，但至少会保留指定数量的线程。
 * 
 * @see pool
 * 
 * @author fomjar
 * @date 2022/04/30
 */
class cached_pool : public pool {

public:
    cached_pool(size_t cached_size = 4) :
        monitor(std::chrono::minutes(2)) {
        this->monitor.submit((func_vv) [this, cached_size] {
            if (this->size() > cached_size)
                this->shrink(cached_size);
        });
        this->monitor.start();
    }

protected:
    exec * choose() override {
        for (auto exec : this->execs) {
            if (exec->is_idle())
                return exec;
        }
        this->reserve(this->size() + 1);
        return this->execs.back();
    }

private:
    looper  monitor;

};




} // namespace jar


#endif // _JAR_EXEC_H
