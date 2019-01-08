/*********************                                                        */
/*! \file Map.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __Map_h__
#define __Map_h__

#include "CommonError.h"
#include "List.h"
#include "Set.h"
#include <map>

template<class Key, class Value>
class Map
{
    typedef std::map<Key,Value> Super;
public:
    typedef typename Super::iterator iterator;
    typedef typename Super::const_iterator const_iterator;
    typedef typename Super::reverse_iterator reverse_iterator;
    typedef typename Super::const_reverse_iterator const_reverse_iterator;

    Map<Value, List<Key>> flip()
    {
        typedef List<Key> ListOfKeys;
        Map<Value, ListOfKeys> result;

        for ( const auto &pair : _container )
            result[pair.second].append( pair.first );

        return result;
    }

    Value &operator[]( const Key &key )
    {
        return _container[key];
    }

    Set<Value> values() const
    {
        Set<Value> result;
        for ( auto it : _container )
            result.insert( it.second );

        return result;
    }

    Set<Key> keys() const
    {
        Set<Key> result;
        for ( auto it : _container )
            result.insert( it.first );

        return result;
    }

    const Value &operator[]( const Key &key ) const
    {
        return _container.at( key );
    }

    const Value &at( const Key &key ) const
    {
        if ( !exists( key ) )
            throw CommonError( CommonError::KEY_DOESNT_EXIST_IN_MAP );

        return _container.at( key );
    }

    Value &at( const Key &key )
    {
        if ( !exists( key ) )
            throw CommonError( CommonError::KEY_DOESNT_EXIST_IN_MAP );

        return _container.at( key );
    }

    bool operator==( const Map<Key,Value> &other ) const
    {
        return _container == other._container;
    }

    bool operator!=( const Map<Key,Value> &other ) const
    {
        return _container != other._container;
    }

    bool operator<( const Map<Key,Value> &other ) const
    {
        return _container < other._container;
    }

    Value get( const Key &key ) const
    {
        if ( !exists( key ) )
            throw CommonError( CommonError::KEY_DOESNT_EXIST_IN_MAP );

        return _container.at( key );
    }

    bool empty() const
    {
        return _container.empty();
    }

    unsigned size() const
    {
        return _container.size();
    }

    void insert( const Key &key, Value value )
    {
        _container.insert( std::make_pair( key, value ) );
    }

    bool exists( const Key &key ) const
    {
        return _container.find( key ) != _container.end();
    }

    void erase( Key key )
    {
        if ( !exists( key ) )
            throw CommonError( CommonError::KEY_DOESNT_EXIST_IN_MAP );

        _container.erase( key );
    }

    iterator erase( iterator it )
    {
        return _container.erase( it );
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

    reverse_iterator rbegin()
    {
        return _container.rbegin();
    }

    reverse_iterator rend()
    {
        return _container.rend();
    }

    const_reverse_iterator rbegin() const
    {
        return _container.rbegin();
    }

    const_reverse_iterator rend() const
    {
        return _container.rend();
    }

    void clear()
    {
        _container.clear();
    }

    void setIfDoesNotExist( Key key, const Value &value )
    {
        if ( !exists( key ) )
            _container[key] = value;
    }

    Key keyWithLargestValue() const
    {
        if ( empty() )
            throw CommonError( CommonError::MAP_IS_EMPTY );

        Key maxKey = _container.begin()->first;

        for ( const auto &pair : _container )
            if ( pair.second > _container.at( maxKey ) )
                maxKey = pair.first;

        return maxKey;
    }

    Key keyByValue( const Value &value ) const
    {
        for ( const auto &pair : _container )
            if ( pair.second == value )
                return pair.first;

        throw CommonError( CommonError::KEY_DOESNT_EXIST_IN_MAP );
    }

private:
    Super _container;
};

#endif // __Map_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
