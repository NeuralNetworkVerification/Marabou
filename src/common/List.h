/*********************                                                        */
/*! \file List.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __List_h__
#define __List_h__

#include "CommonError.h"

#include <list>

template<class T>
class List
{
    typedef std::list<T> Super;
public:
    typedef typename Super::iterator iterator;
    typedef typename Super::const_iterator const_iterator;

    typedef typename Super::reverse_iterator reverse_iterator;
    typedef typename Super::const_reverse_iterator const_reverse_iterator;

    List<T>()
    {
    }

    List<T>( const std::initializer_list<T> &initializerList ) : _container( initializerList )
    {
    }

    void append( const List<T> &other )
    {
        for ( const auto &element : other )
            _container.push_back( element );
    }

    void append( const T &value )
    {
        _container.push_back( value );
    }

    void appendHead( const T &value )
    {
        _container.push_front( value );
    }

    iterator begin()
    {
        return _container.begin();
    }

    const_iterator begin() const
    {
        return _container.begin();
    }

    reverse_iterator rbegin()
    {
        return _container.rbegin();
    }

    const_reverse_iterator rbegin() const
    {
        return _container.rbegin();
    }

    iterator end()
    {
        return _container.end();
    }

    const_iterator end() const
    {
        return _container.end();
    }

    reverse_iterator rend()
    {
        return _container.rend();
    }

    const_reverse_iterator rend() const
    {
        return _container.rend();
    }

    iterator erase( iterator it )
    {
        return _container.erase( it );
    }

    void erase( const T &value )
    {
        for ( iterator it = begin(); it != end(); ++it )
        {
            if ( *it == value )
            {
                erase( it );
                return;
            }
        }
    }

    void clear()
    {
        _container.clear();
    }

    unsigned size() const
    {
        return _container.size();
    }

    bool empty() const
    {
        return _container.empty();
    }

    bool exists( const T &value ) const
    {
        for ( const_iterator it = begin(); it != end(); ++it )
        {
            if ( *it == value )
                return true;
        }

        return false;
    }

    T &front()
    {
        return _container.front();
    }

    const T &front() const
    {
        return _container.front();
    }

    T &back()
    {
        return _container.back();
    }

    const T &back() const
    {
        return _container.back();
    }

    void popBack()
    {
        if ( empty() )
            throw CommonError( CommonError::LIST_IS_EMPTY );

        _container.pop_back();
    }

    bool operator==( const List<T> &other ) const
    {
        return _container == other._container;
    }

    bool operator!=( const List<T> &other ) const
    {
        return _container != other._container;
    }

protected:
    Super _container;
};

#endif // __List_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
