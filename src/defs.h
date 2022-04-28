
#ifndef _JAR_DEFS_H
#define _JAR_DEFS_H


#define TYPE_DIFF(type1, type2) typename = typename std::enable_if<!std::is_same<type1, type2>::value>::type
#define TYPE_SAME(type1, type2) typename = typename std::enable_if<std::is_same<type1, type2>::value>::type


#endif // _JAR_DEFS_H
