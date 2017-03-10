/*********************                                                        */
/*! \file Pair.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Reluplex project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __Pair_h__
#define __Pair_h__

#include <iostream>
#include <utility>

template<class L, class R>
class Pair
{
    typedef std::pair<L,R> Super;
public:
    Pair()
    {
    }

    Pair( const L &first, const R &second ) : _container( first, second )
    {
    }

    L &first()
    {
        return _container.first;
    }

    const L &first() const
    {
        return _container.first;
    }

    R &second()
    {
        return _container.second;
    }

    const R &second() const
    {
        return _container.second;
    }

	Pair<L, R> &operator=( const Pair<L, R> &other )
	{
		_container = other._container;
		return *this;
	}

    bool operator==( const Pair<L, R> &other ) const
	{
		return _container == other._container;
	}

    bool operator!=( const Pair<L, R> &other ) const
	{
		return _container != other._container;
	}

    bool operator<( const Pair<L, R> &other ) const
    {
        return _container < other._container;
    }

protected:
    Super _container;
};

template<class L, class R>
std::ostream &operator<<( std::ostream &stream, const Pair<L, R> &pair )
{
    return stream << pair.first() << "," << pair.second();
}

#endif // __Pair_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
