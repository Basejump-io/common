#ifndef _QCC_HASHFUN_H
#define _QCC_HASHFUN_H

namespace std {
/**
 * Functor to compute a hash for char* suitable for use with
 * std::unordered_map, std::unordered_set, std::hash_map, std::hash_set.
 */
template <>
//struct hash<const char*> {
inline size_t hash<const char*>::operator()(const char* __s1) const {
    /* Fix for change from hash_map to unordered_map:
       Headers included by unordered_map use the address of a char* to determine the hash,
       instead of using the string contents. This function is derived from the __stl_hash_string
       function present in backward/hash_fun.h header in Linux and is the same as the one present
       in STLPort.*/
//        const char* __s1 = (const char*)s.c_str();
    unsigned long __h = 0;
    for (; *__s1; ++__s1)
        __h = 5 * __h + *__s1;
    return size_t(__h);
}
};
//}

#endif
