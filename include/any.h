/**
 * @file any.h
 * @author fomjar (fomjar@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2022-04-30
 * 
 * @copyright Copyright (c) 2022
 * 
 */

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

    using func_vv = std::function<void()>;

public:
    any()               { this->set_val(0); }
    any(const any  & a) { this->set_any(std::forward<const any>(a)); }
    any(      any  & a) { this->set_any(std::forward<      any>(a)); }
    any(      any && a) { this->set_any(std::forward<      any>(a)); }
    template <typename _Tp>
    any(const _Tp  & v) { this->set_val(std::forward<const _Tp>(v)); }
    template <typename _Tp>
    any(      _Tp && v) { this->set_val(std::forward<      _Tp>(v)); }
    ~any()              { this->d(); }
    
    any & operator=(const any  & a) { this->set_any(std::forward<const any>(a)); return *this; }
    template <typename _Tp>
    any & operator=(const _Tp  & v) { this->set_val(std::forward<const _Tp>(v)); return *this; }
    template <typename _Tp>
    any & operator=(      _Tp && v) { this->set_val(std::forward<      _Tp>(v)); return *this; }

    template <typename _Tp>
           _Tp & cast() const { return * (_Tp *) this->v; }
    const char * type() const { return this->t; }

private:
    void set_any(const any & a) {
        this->t = a.t;
        this->v = a.v;
        this->d = a.d;

        auto a0 = const_cast<any *>(&a);
        a0->d = [] { };
    }
    template <typename _Tp>
    void set_val(const _Tp & v) {
        this->t = typeid(v).name();
        this->v = (void *) std::addressof(v);
        this->d = [] { };
    }
    template <typename _Tp>
    void set_val(_Tp && v) {
        this->t = typeid(v).name();
        auto p = new _Tp(v);
        this->v = (void *) p;
        this->d = [p] { delete p; };
    }

    const char * t; // type
          void * v; // value
       func_vv   d; // delete
};


} // namespace jar


#endif // _JAR_ANY_H
