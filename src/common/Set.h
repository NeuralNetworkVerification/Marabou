/*********************                                                        */
/*! \file Set.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifndef __Set_h__
#define __Set_h__

#include "List.h"
#include "T/stdlib.h"

#include <algorithm>
#include <iostream>
#include <set>

#ifdef _WIN32
#include <iterator>
#endif

template <class Value> class Set
{
    typedef std::set<Value> Super;

public:
    typedef typename Super::iterator iterator;
    typedef typename Super::const_iterator const_iterator;
    typedef typename Super::const_reverse_iterator const_reverse_iterator;

    Set()
    {
    }

    Set( const std::initializer_list<Value> &initializerList )
        : _container( initializerList )
    {
    }

    void insert( const Value &value )
    {
        _container.insert( value );
    }

    void insert( const Set &other )
    {
        for ( auto it = other.begin(); it != other.end(); ++it )
            _container.insert( *it );
    }

    void insert( const List<Value> &other )
    {
        for ( auto it = other.begin(); it != other.end(); ++it )
            _container.insert( *it );
    }

    void operator+=( const Set &other )
    {
        insert( other );
    }

    Set operator+( const Set &other )
    {
        Set result = *this;
        result.insert( other );
        return result;
    }

    bool operator==( const Set &other ) const
    {
        return _container == other._container;
    }

    bool operator!=( const Set &other ) const
    {
        return _container != other._container;
    }

    bool operator<( const Set &other ) const
    {
        return _container < other._container;
    }

    static bool containedIn( const Set &one, const Set &two )
    // Is one contained in two?
    {
        return Set::difference( one, two ).empty();
    }

    static Set difference( const Set &one, const Set &two )
    // Elements that appear in one, but do not appear in two.
    {
        Set difference;
        std::set_difference( one.begin(),
                             one.end(),
                             two.begin(),
                             two.end(),
                             std::inserter( difference._container, difference.end() ) );
        return difference;
    }

    static Set intersection( const Set &one, const Set &two )
    {
        Set intersection;
        std::set_intersection( one.begin(),
                               one.end(),
                               two.begin(),
                               two.end(),
                               std::inserter( intersection._container, intersection.end() ) );
        return intersection;
    }

    const_iterator begin() const
    {
        return _container.begin();
    }

    const_iterator end() const
    {
        return _container.end();
    }

    const_reverse_iterator rbegin() const
    {
        return _container.rbegin();
    }

    const_reverse_iterator rend() const
    {
        return _container.rend();
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

    Value getRandomElement()
    {
        auto it = begin();
        int index = T::rand() % size();
        while ( index > 0 )
        {
            ++it;
            --index;
        }

        return *it;
    }

protected:
    Super _container;
};

#endif // __Set_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
