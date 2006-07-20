
/* Copyright (c) 2005, Stefan Eilemann <eile@equalizergraphics.com> 
   All rights reserved. */

#ifndef EQBASE_HASH_H
#define EQBASE_HASH_H

#ifdef __GNUC__              // GCC 3.1 and later
#  include <ext/hash_map>
#  include <ext/hash_set>
namespace Sgi = ::__gnu_cxx; 
#else                        //  other compilers
#  include <hash_map>
#  include <hash_set>
namespace Sgi = std;
#endif

namespace eqBase
{
    /** Provides a hashing function for std::string. */
    struct hashString
    {
        size_t operator()(const std::string& string) const
            {  
                return Sgi::__stl_hash_string( string.c_str( ));
            }
    };
    /** A hash for std::string keys. */
    template<class T> class StringHash 
        : public Sgi::hash_map<std::string, T, hashString >
    {};

    /** Provides a hashing function for pointers. */
    template< class T > struct hashPtr
    {
        size_t operator()(const T& N) const
            {  
                return ((size_t)N);
            }
    };
    /** A hash for pointer keys. */
    template<class K, class T> class PtrHash 
        : public Sgi::hash_map<K, T, hashPtr<K> >
    {};
}

#endif // EQBASE_HASH_H
