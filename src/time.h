
#ifndef _JAR_TIME_H
#define _JAR_TIME_H

#include <chrono>
#include <string>

namespace jar {

/**
 * @brief 从1970/01/01 00:00:00到当前时间为止经过的时间，单位：微秒。
 * 
 * @return long long 
 * 
 * @author fomjar
 * @date 2022/04/30
 */
inline long long now() {
    return std::chrono::system_clock::now().time_since_epoch().count();
}

/**
 * @brief 格式化当前时间。
 * 
 * @param format YYYY: 年, MM: 月, DD: 日, hh: 时, mm: 分, ss: 秒, SSS: 毫秒, SSSSSS: 微秒
 * @return std::string 
 * 
 * @author fomjar
 * @date 2022/04/30
 */
inline std::string now2str(const std::string & format = "YYYY/MM/DD hh:mm:ss.SSSSSS") {
    auto tp = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(tp);
    auto tm = localtime(&tt);

    std::string str = format;

    auto fmt = [] (long long n, size_t size) -> std::string {
        std::string s = std::to_string(n);
        while (s.length() < size) s.insert(0, "0");
        return s;
    };

    while (std::string::npos != str.find("YYYY"))   str.replace(str.find("YYYY"),   4, fmt(tm->tm_year + 1900,  4));
    while (std::string::npos != str.find("MM"))     str.replace(str.find("MM"),     2, fmt(tm->tm_mon + 1,      2));
    while (std::string::npos != str.find("DD"))     str.replace(str.find("DD"),     2, fmt(tm->tm_mday,         2));
    while (std::string::npos != str.find("hh"))     str.replace(str.find("hh"),     2, fmt(tm->tm_hour,         2));
    while (std::string::npos != str.find("mm"))     str.replace(str.find("mm"),     2, fmt(tm->tm_min,          2));
    while (std::string::npos != str.find("ss"))     str.replace(str.find("ss"),     2, fmt(tm->tm_sec,          2));

    while (std::string::npos != str.find("SSSSSS"))     str.replace(str.find("SSSSSS"),     6, fmt(tp.time_since_epoch().count()        % 1000000,      6));
    while (std::string::npos != str.find("SSS"))        str.replace(str.find("SSS"),        3, fmt(tp.time_since_epoch().count() / 1000 % 1000,         3));

    return str;
}

} // namespace jar


#endif // _JAR_TIME_H
