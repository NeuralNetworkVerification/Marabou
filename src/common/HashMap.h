/*********************                                                        */
/*! \file HashMap.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __HashMap_h__
#define __HashMap_h__

#include "CommonError.h"
#include "List.h"
#include "Set.h"
#include <unordered_map>

template<class Key, class Value>
class HashMap
{
    typedef std::unordered_map<Key,Value> Super;
public:
    typedef typename Super::iterator iterator;
    typedef typename Super::const_iterator const_iterator;

    Value &operator[]( const Key &key )
    {
        return _container[key];
    }

    bool empty() const
    {
        return _container.empty();
    }

    bool exists( const Key &key ) const
    {
        return _container.find( key ) != _container.end();
    }

    unsigned size() const
    {
        return _container.size();
    }

    void erase( Key key )
    {
        if ( !exists( key ) )
            throw CommonError( CommonError::KEY_DOESNT_EXIST_IN_HASHMAP );

        _container.erase( key );
    }

    iterator begin()
    {
        return _container.begin();
    }

    iterator end()
    {
        return _container.end();
    }

    const_iterator begin() const
    {
        return _container.begin();
    }

    const_iterator end() const
    {
        return _container.end();
    }

    void clear()
    {
        _container.clear();
    }

    Value get( const Key &key ) const
    {
        if ( !exists( key ) )
            throw CommonError( CommonError::KEY_DOESNT_EXIST_IN_HASHMAP );

        return _container.at( key );
    }

    void set( const Key &key, const Value &value )
    {
        _container[key] = value;
    }

    bool operator==( const HashMap<Key,Value> &other ) const
    {
        return _container == other._container;
    }

    bool operator!=( const HashMap<Key,Value> &other ) const
    {
        return _container != other._container;
    }

    const Value &at( const Key &key ) const
    {
        if ( !exists( key ) )
            throw CommonError( CommonError::KEY_DOESNT_EXIST_IN_HASHMAP );

        return _container.at( key );
    }

    Value &at( const Key &key )
    {
        if ( !exists( key ) )
            throw CommonError( CommonError::KEY_DOESNT_EXIST_IN_HASHMAP );

        return _container.at( key );
    }

    // Set<Key> keys() const
    // {
    //     Set<Key> result;
    //     for ( auto it : _container )
    //         result.insert( it.first );

    //     return result;
    // }

private:
    Super _container;
};

#endif // __HashMap_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
