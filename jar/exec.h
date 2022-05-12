/**
 * @file exec.h
 * @author fomjar (fomjar@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2022-04-30
 * 
 * @copyright Copyright (c) 2022
 * 
 */

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



template <typename _Fp>
using func      = std::function<_Fp>;
template <typename ... _Ap>
using func_v    = func<void(_Ap...)>;
using func_vv   = func<void(void)>;



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
        name("jar::exec #" + std::to_string(++executor::name_idx)),
        _is_running(false),
        thread(nullptr) { };
    virtual ~executor() { this->stop(); }

    bool    is_running()    const { return this->_is_running; }
    size_t  size()          const { return this->tasks.size(); }
    bool    is_idle()       const { return this->tasks.empty(); }

    void set_name(const std::string & name) { this->name = name; }
    std::string get_name() const { return this->name; }

    /**
     * @brief 启动线程。
     */
    void start() {
        if (this->is_running())
            return;
        
        this->_is_running = true;
        this->thread = new std::thread([this] {
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
            // 已执行完的线程无法join
            if (this->thread->joinable()) {
                this->condition.notify_all();
                this->thread->join();
            }
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
     * @brief 此线程的join操作。
     */
    void join() {
        if (this->is_running() && this->thread->joinable()) {
            this->thread->join();
        }
    }
    
    /**
     * @brief 提交任务。执行方式和时机取决于实现。
     * 
     * @tparam _Rp 
     * @tparam _Ap 
     * @param prom 
     * @param task 
     * @param args 
     */
    template <typename _Rp, typename ... _Ap>
    void submit(const std::promise<_Rp> & prom, const func<_Rp(_Ap...)> & task, const _Ap & ... args) {
        JAR_EXEC_LOCK_GUARD
        this->tasks.push_back([=, &prom] { const_cast<std::promise<_Rp> &>(prom).set_value(task(args...)); });
        this->condition.notify_all();
    }
    
    /**
     * @brief 提交任务。执行方式和时机取决于实现。
     * 
     * @tparam _Ap 
     * @param prom 
     * @param task 
     * @param args 
     */
    template <typename ... _Ap>
    void submit(const std::promise<void> & prom, const func_v<_Ap...> & task, const _Ap & ... args) {
        JAR_EXEC_LOCK_GUARD
        this->tasks.push_back([=, &prom] {
            task(args...);
            const_cast<std::promise<void> &>(prom).set_value();
        });
        this->condition.notify_all();
    }

    /**
     * @brief 提交任务。执行方式和时机取决于实现。
     * 
     * @tparam _Rp 
     * @tparam _Ap 
     * @param task 
     * @param args 
     */
    template <typename _Rp, typename ... _Ap>
    void submit(const func<_Rp(_Ap...)> & task, const _Ap & ... args) {
        JAR_EXEC_LOCK_GUARD
        this->tasks.push_back([=] { task(args...); });
        this->condition.notify_all();
    }

protected:
    virtual func_vv worker() = 0;

    std::vector<func_vv>    tasks;
    std::mutex              mutex;
    std::condition_variable condition;
    std::string             name;

private:
    bool            _is_running;
    std::thread   * thread;

private:
    static uint32_t name_idx;

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

public:
    queuer() {
        this->set_name("jar::queuer #" + std::to_string(++queuer::name_idx));
    }

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

private:
    static uint32_t name_idx;

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
        duration(std::chrono::duration_cast<std::chrono::microseconds>(duration))
    {
        this->set_name("jar::delayer #" + std::to_string(++delayer::name_idx));
    }

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

private:
    static uint32_t name_idx;

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
        interval(std::chrono::duration_cast<std::chrono::microseconds>(interval))
    {
        this->set_name("jar::looper #" + std::to_string(++looper::name_idx));
    }

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

private:
    static uint32_t name_idx;

};



/**
 * @brief 动画引擎。以固定频率执行任务，与looper的区别在于，它以绝对的时间频率来执行任务，因为它将任务的执行时间也计算在内。
 * 
 * @see exec
 * 
 * @author fomjar
 * @date 2022/04/30
 */
class animator : public exec {

public:
    animator(float frequency = 24.0f) : frequency(frequency) {
        this->set_name("jar::animator #" + std::to_string(++animator::name_idx));
    }

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

private:
    static uint32_t name_idx;

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
     * @tparam _Rp 
     * @tparam _Ap 
     * @param prom 
     * @param task 
     * @param args 
     */
    template <typename _Rp, typename ... _Ap>
    void submit(const std::promise<_Rp> & prom, const func<_Rp(_Ap...)> & task, const _Ap & ... args) {
        this->choose()->submit(
            std::forward<const std::promise<_Rp>>(prom),
            std::forward<const func<_Rp(_Ap...)>>(task),
            std::forward<const _Ap>(args)...
        );
    }
    
    /**
     * @brief 提交任务。执行方式和时机取决于实现。
     * 
     * @tparam _Ap 
     * @param prom 
     * @param task 
     * @param args 
     */
    template <typename ... _Ap>
    void submit(const std::promise<void> & prom, const func_v<_Ap...> & task, const _Ap & ... args) {
        this->choose()->submit(
            std::forward<const std::promise<void>>(prom),
            std::forward<const func_v<_Ap...>>(task),
            std::forward<const _Ap>(args)...
        );
    }
    /**
     * @brief 提交任务。执行方式和时机取决于实现。
     * 
     * @tparam _Rp 
     * @tparam _Ap 
     * @param task 
     * @param args 
     */
    template <typename _Rp, typename ... _Ap>
    void submit(const func<_Rp(_Ap...)> & task, const _Ap & ... args) {
        this->choose()->submit(
            std::forward<const func<_Rp(_Ap...)>>(task),
            std::forward<const _Ap>(args)...
        );
    }

protected:
    virtual exec * choose() = 0;

    std::vector<exec *> execs;
    std::mutex          mutex;
};

using exec_pool = executor_pool;


/**
 * @brief 固定大小的线程池实现。当没有空闲线程时，任务将派发给任务数量最少的线程，排队等待执行。
 * 
 * @see exec_pool
 * 
 * @author fomjar
 * @date 2022/04/30
 */
class fixed_pool : public exec_pool {

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
 * @see exec_pool
 * 
 * @author fomjar
 * @date 2022/04/30
 */
class cached_pool : public exec_pool {

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


extern cached_pool pool;


/**
 * @brief 异步执行。
 * 
 * @tparam _Rp 
 * @tparam _Ap 
 * @param prom 
 * @param task 
 * @param args 
 * 
 * @author fomjar
 * @date 2022/05/01
 */
template <typename _Rp, typename ... _Ap>
inline void async(const std::promise<_Rp> & prom, const func<_Rp(_Ap...)> & task, const _Ap & ... args) {
    pool.submit(
        std::forward<const std::promise<_Rp>>(prom),
        std::forward<const func<_Rp(_Ap...)>>(task),
        std::forward<const _Ap>(args)...
    );
}

/**
 * @brief 异步执行。
 * 
 * @tparam _Ap 
 * @param prom 
 * @param task 
 * @param args 
 * 
 * @author fomjar
 * @date 2022/05/01
 */
template <typename ... _Ap>
inline void async(const std::promise<void> & prom, const func_v<_Ap...> & task, const _Ap & ... args) {
    pool.submit(
        std::forward<const std::promise<void>>(prom),
        std::forward<const func_v<_Ap...>>(task),
        std::forward<const _Ap>(args)...
    );
}

/**
 * @brief 异步执行。
 * 
 * @tparam _Rp 
 * @tparam _Ap 
 * @param task 
 * @param args 
 * 
 * @author fomjar
 * @date 2022/05/01
 */
template <typename _Rp, typename ... _Ap>
inline void async(const func<_Rp(_Ap...)> & task, const _Ap & ... args) {
    pool.submit(
        std::forward<const func<_Rp(_Ap...)>>(task),
        std::forward<const _Ap>(args)...
    );
}

/**
 * @brief 延迟执行。
 * 
 * @tparam _Rep
 * @tparam _Period
 * @tparam _Rp 
 * @tparam _Ap 
 * @param prom 
 * @param dura
 * @param task 
 * @param args 
 * 
 * @author fomjar
 * @date 2022/05/01
 */
template <typename _Rep, typename _Period, typename _Rp, typename ... _Ap>
inline void delay(
    const std::promise<_Rp> & prom,
    const std::chrono::duration<_Rep, _Period> & dura,
    const func<_Rp(_Ap...)> & task,
    const               _Ap & ... args
) {
    pool.submit((func_vv) [=, &prom] {
        delayer e(std::forward<const std::chrono::duration<_Rep, _Period>>(dura));
        e.submit(
            std::forward<const std::promise<_Rp>>(prom),
            std::forward<const func<_Rp(_Ap...)>>(task),
            std::forward<const               _Ap>(args)...
        );
        e.start();
        e.join();
    });
}

/**
 * @brief 延迟执行。
 * 
 * @tparam _Rep
 * @tparam _Period
 * @tparam _Ap 
 * @param prom 
 * @param dura
 * @param task 
 * @param args 
 * 
 * @author fomjar
 * @date 2022/05/01
 */
template <typename _Rep, typename _Period, typename ... _Ap>
inline void delay(
    const std::promise<void> & prom,
    const std::chrono::duration<_Rep, _Period> & dura,
    const     func_v<_Ap...> & task,
    const                _Ap & ... args
) {
    pool.submit((func_vv) [=, &prom] {
        delayer e(std::forward<const std::chrono::duration<_Rep, _Period>>(dura));
        e.submit(
            std::forward<const std::promise<void>>(prom),
            std::forward<const     func_v<_Ap...>>(task),
            std::forward<const                _Ap>(args)...
        );
        e.start();
        e.join();
    });
}

/**
 * @brief 延迟执行。
 * 
 * @tparam _Rep
 * @tparam _Period
 * @tparam _Rp 
 * @tparam _Ap 
 * @param dura
 * @param task 
 * @param args 
 * 
 * @author fomjar
 * @date 2022/05/01
 */
template <typename _Rep, typename _Period, typename _Rp, typename ... _Ap>
inline void delay(
    const std::chrono::duration<_Rep, _Period> & dura,
    const func<_Rp(_Ap...)> & task,
    const               _Ap & ... args
) {
    pool.submit((func_vv) [=] () {
        delayer e(std::forward<const std::chrono::duration<_Rep, _Period>>(dura));
        e.submit(
            std::forward<const func<_Rp(_Ap...)>>(task),
            std::forward<const               _Ap>(args)...
        );
        e.start();
        e.join();
    });
}

/**
 * @brief 循环执行。
 * 
 * @tparam _Rep 
 * @tparam _Period 
 * @tparam _Rp 
 * @tparam _Ap 
 * @param intv 
 * @param task 
 * @param args 
 * @return looper* 
 * 
 * @author fomjar
 * @date 2022/05/02
 */
template <typename _Rep, typename _Period, typename _Rp, typename ... _Ap>
inline looper * loop (
    const std::chrono::duration<_Rep, _Period> & intv,
    const func<_Rp(_Ap...)> & task,
    const               _Ap & ... args
) {
    auto e = new looper(std::forward<const std::chrono::duration<_Rep, _Period>>(intv));
    e->submit(
        std::forward<const func<_Rp(_Ap...)>>(task),
        std::forward<const               _Ap>(args)...
    );
    e->start();
    return e;
}

/**
 * @brief 定频执行。
 * 
 * @tparam _Rp 
 * @tparam _Ap 
 * @param freq 
 * @param task 
 * @param args 
 * @return animator* 
 * 
 * @author fomjar
 * @date 2022/05/02
 */
template <typename _Rp, typename ... _Ap>
inline animator * anim (
                      float   freq,
    const func<_Rp(_Ap...)> & task,
    const               _Ap & ... args
) {
    auto e = new animator(freq);
    e->submit(
        std::forward<const func<_Rp(_Ap...)>>(task),
        std::forward<const               _Ap>(args)...
    );
    e->start();
    return e;
}


} // namespace jar


#endif // _JAR_EXEC_H
