
#ifndef _JAR_ANY_H
#define _JAR_ANY_H

#include <functional>
#include <memory>

#include "defs.h"


namespace jar {


/**
 * @brief Any object's wrapper.
 * 
 */
class any {

    using func = std::function<void()>;

public:
    any()               { this->set(0); }
    any(const any  & a) { this->set(std::forward<const any>(a)); }
    template <typename T, TYPE_DIFF(T, any)>
    any(const   T  & v) { this->set(std::forward<const   T>(v)); }
    template <typename T, TYPE_DIFF(T, any)>
    any(        T && v) { this->set(std::forward<        T>(v)); }
    ~any()              { this->del(); }
    
    any & operator=(const any  & a) { this->set(std::forward<const any>(a)); return *this; }
    template <typename T, TYPE_DIFF(T, any)>
    any & operator=(const   T  & v) { this->set(std::forward<const   T>(v)); return *this; }
    template <typename T, TYPE_DIFF(T, any)>
    any & operator=(        T && v) { this->set(std::forward<        T>(v)); return *this; }

    template <typename T>
    T & cast() { return * (T *) this->v; }

private:
    void set(const any & a) {
        this->t     = a.t;
        this->v     = a.v;
        this->del   = a.del;

        auto a0 = const_cast<any *>(&a);
        a0->del = [] { };
    }
    template <typename T, TYPE_DIFF(T, any)>
    void set(const T & v) {
        this->t     = typeid(v).name();
        this->v     = (void *) std::addressof(v);
        this->del   = [] { };
    }
    template <typename T, TYPE_DIFF(T, any)>
    void set(T && v) {
        this->t     = typeid(v).name();
        auto p      = new T(v);
        this->v     = (void *) p;
        this->del   = [p] { delete p; };
    }

    const char * t;
          void * v;
          func   del;
};


} // namespace jar


#endif // _JAR_ANY_H
