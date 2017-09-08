/*********************                                                        */
/*! \file Vector.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __Vector_h__
#define __Vector_h__

#include <algorithm>
#include <cstdio>

#include "CommonError.h"
#include <vector>

template<class T>
class Vector
{
    typedef std::vector<T> Super;
public:
    typedef typename Super::iterator iterator;
    typedef typename Super::const_iterator const_iterator;

    Vector<T>()
    {
    }

    Vector<T>( const std::initializer_list<T> &initializerList ) : _container( initializerList )
    {
    }

    virtual void append( T value )
    {
        _container.push_back( value );
    }

    virtual void insertHead( T value )
    {
        _container.insert( _container.begin(), value );
    }

    virtual ~Vector()
    {
    }

    T get( int index ) const
    {
        return _container.at( index );
    }

    T &operator[]( int index )
    {
        return _container[index];
    }

    bool empty() const
    {
        return size() == 0;
    }

    unsigned size() const
    {
        return _container.size();
    }

    bool exists( const T &value ) const
    {
        for ( unsigned i = 0; i < size(); ++i )
        {
            if ( get( i ) == value )
                return true;
        }

        return false;
    }

    void erase( const T &value )
    {
        for ( iterator it = _container.begin(); it != _container.end(); ++it )
        {
            if ( (*it) == value )
            {
                _container.erase( it );
                return;
            }
        }

        throw CommonError( CommonError::VALUE_DOESNT_EXIST_IN_VECTOR );
    }

    void eraseByValue( T value )
    {
        for ( iterator it = _container.begin(); it != _container.end(); ++it )
        {
            if ( (*it) == value )
            {
                _container.erase( it );
                return;
            }
        }

        throw CommonError( CommonError::VALUE_DOESNT_EXIST_IN_VECTOR );
    }

    iterator begin()
    {
        return _container.begin();
    }

    iterator end()
    {
        return _container.end();
    }

    void erase( iterator &it )
    {
        _container.erase( it );
    }

    void eraseAt( unsigned index )
    {
        if ( index >= size() )
            throw CommonError( CommonError::VECTOR_OUT_OF_BOUNDS );

        iterator it = _container.begin();

        while ( index > 0 )
        {
            ++it;
            --index;
        }

        _container.erase( it );
    }

    void clear()
    {
        _container.clear();
    }

    Vector<T> operator+( const Vector<T> &other )
    {
        Vector<T> output;

        for ( unsigned i = 0; i < this->size(); ++i )
            output.append( ( *this )[i] );

        for ( unsigned i = 0; i < other.size(); ++i )
            output.append( other.get( i ) );

        return output;
    }

    Vector<T> &operator+=( const Vector<T> &other )
    {
        (*this) = (*this) + other;
        return *this;
    }

    T popFirst()
    {
        if ( size() == 0 )
            throw CommonError( CommonError::POPPING_FROM_EMPTY_VECTOR );

        T value = _container[0];
        eraseAt( 0 );
        return value;
    }

    T first() const
    {
        if ( empty() )
            throw CommonError( CommonError::VECTOR_OUT_OF_BOUNDS );

        return get( 0 );
    }

    T last() const
    {
        if ( empty() )
            throw CommonError( CommonError::VECTOR_OUT_OF_BOUNDS );

        return get( size() - 1 );
    }

    T pop()
    {
        if ( size() == 0 )
            throw CommonError( CommonError::POPPING_FROM_EMPTY_VECTOR );

        T value = last();
        eraseAt( size() - 1 );
        return value;
    }

    bool operator==( const Vector<T> &other ) const
    {
        if ( size() != other.size() )
            return false;

        Vector<T> copyOfOther = other;

        for ( unsigned i = 0; i < size(); ++i )
        {
            if ( !copyOfOther.exists( get( i ) ) )
                return false;

            copyOfOther.erase( get( i ) );
        }

        return true;
    }

    bool operator!=( const Vector<T> &other ) const
    {
        return !( *this == other );
    }

    Vector &operator=( const Vector<T> &other )
    {
        _container = other._container;
        return *this;
    }

    void sort()
    {
        std::sort( _container.begin(), _container.end() );
    }

    Super getContainer() const
    {
        return _container;
    }

protected:
    Super _container;
};

#endif // __Vector_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
