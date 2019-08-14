/*********************                                                        */
/*! \file HashSet.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifndef __HashSet_h__
#define __HashSet_h__

#include "T/stdlib.h"
#include <algorithm>
#include <iostream>
#include <unordered_set>

template<class Value>
class HashSet
{
    typedef std::unordered_set<Value> Super;
public:
    typedef typename Super::iterator iterator;
    typedef typename Super::const_iterator const_iterator;

    HashSet<Value>()
    {
    }

    HashSet<Value>( const std::initializer_list<Value> &initializerList ) : _container( initializerList )
    {
    }

    void insert( const Value &value )
    {
        _container.insert( value );
    }

    void insert( const HashSet<Value> &other )
    {
        for ( auto it = other.begin(); it != other.end(); ++it )
            _container.insert( *it );
    }

    void operator+=( const HashSet<Value> &other )
    {
        insert( other );
    }

    HashSet<Value> operator+( const HashSet<Value> &other )
    {
        HashSet<Value> result = *this;
        result.insert( other );
        return result;
    }

    bool operator==( const HashSet<Value> &other ) const
    {
        return _container == other._container;
    }

    bool operator!=( const HashSet<Value> &other ) const
    {
        return _container != other._container;
    }

    bool operator<( const HashSet<Value> &other ) const
    {
        return _container < other._container;
    }

    const_iterator begin() const
    {
        return _container.begin();
    }

    const_iterator end() const
    {
        return _container.end();
    }

    unsigned size() const
    {
        return _container.size();
    }

    bool empty() const
    {
        return _container.empty();
    }

    const_iterator find( const Value &value ) const
    {
        return _container.find( value );
    }

    bool exists( const Value &value ) const
    {
        return ( _container.find( value ) != end() );
    }

    void clear()
    {
        _container.clear();
    }

    void erase( const Value &value )
    {
        _container.erase( value );
    }

    const Super &container() const
    {
        return _container;
    }

    void print() const
    {
        for ( auto it = begin(); it != end(); ++it )
            std::cout << *it;
    }

protected:
    Super _container;
};

#endif // __HashSet_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
