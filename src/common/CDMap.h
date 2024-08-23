/*********************                                                        */
/*! \file CDMap.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** This is a wrapper around the CVC4 CDHashMap class with two additional convenience methods.

 **/

#ifndef __CDMap_h__
#define __CDMap_h__

#include "CommonError.h"

#include <context/cdhashmap.h>

template <class Key, class Data, class HashFcn = std::hash<Key>>
class CDMap : public CVC4::context::CDHashMap<Key, Data, HashFcn>
{
    typedef CVC4::context::CDHashMap<Key, Data, HashFcn> Super;

public:
    CDMap( CVC4::context::Context *context )
        : Super( context )
    {
    }

    Data get( const Key &k ) const
    {
        auto it = Super::find( k );
        if ( it != Super::end() )
        {
            return ( *it ).second;
        }
        throw CommonError( CommonError::KEY_DOESNT_EXIST_IN_HASHMAP );
    }

    bool exists( const Key &k ) const
    {
        return Super::find( k ) != Super::end();
    }
};

#endif // __CDMap_h__
