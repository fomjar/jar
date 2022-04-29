
#ifndef _JAR_ANY_H
#define _JAR_ANY_H

#include <functional>
#include <memory>


namespace jar {



/**
 * @brief 任意对象的包装器。针对左引用采用地址引用的策略，针对右引用采用值拷贝的策略。
 * 
 * 用法如下：
 * 
 * any a = 3;
 * any b = 3.3f;
 * any c = std::string("3.3.3");
 * 
 * a.cast<int>(); // 3
 * b.cast<float>(); // 3.3
 * c.cast<std::string>(); // "3.3.3"
 * 
 * @author fomjar
 * @date 2022/04/30
 */
class any {

#define TYPE_DIFF(type1, type2) typename = typename std::enable_if<!std::is_same<type1, type2>::value>::type
#define TYPE_SAME(type1, type2) typename = typename std::enable_if<std::is_same<type1, type2>::value>::type

    using func = std::function<void()>;

public:
    any()               { this->set(0); }
    any(const any  & a) { this->set(std::forward<const any>(a)); }
    template <typename _Tp, TYPE_DIFF(_Tp, any)>
    any(const _Tp  & v) { this->set(std::forward<const _Tp>(v)); }
    template <typename _Tp, TYPE_DIFF(_Tp, any)>
    any(      _Tp && v) { this->set(std::forward<      _Tp>(v)); }
    ~any()              { this->del(); }
    
    any & operator=(const any  & a) { this->set(std::forward<const any>(a)); return *this; }
    template <typename _Tp, TYPE_DIFF(_Tp, any)>
    any & operator=(const _Tp  & v) { this->set(std::forward<const _Tp>(v)); return *this; }
    template <typename _Tp, TYPE_DIFF(_Tp, any)>
    any & operator=(      _Tp && v) { this->set(std::forward<      _Tp>(v)); return *this; }

    template <typename _Tp>
    _Tp & cast() { return * (_Tp *) this->v; }

private:
    void set(const any & a) {
        this->t     = a.t;
        this->v     = a.v;
        this->del   = a.del;

        auto a0 = const_cast<any *>(&a);
        a0->del = [] { };
    }
    template <typename _Tp, TYPE_DIFF(_Tp, any)>
    void set(const _Tp & v) {
        this->t     = typeid(v).name();
        this->v     = (void *) std::addressof(v);
        this->del   = [] { };
    }
    template <typename _Tp, TYPE_DIFF(_Tp, any)>
    void set(_Tp && v) {
        this->t     = typeid(v).name();
        auto p      = new _Tp(v);
        this->v     = (void *) p;
        this->del   = [p] { delete p; };
    }

    const char * t;
          void * v;
          func   del;
};


} // namespace jar


#endif // _JAR_ANY_H
