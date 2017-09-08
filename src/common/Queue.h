/*********************                                                        */
/*! \file Queue.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __Queue_h__
#define __Queue_h__

#include "CommonError.h"
#include <queue>

template<class T>
class Queue
{
    typedef std::queue<T> Super;

public:
    virtual void push( T value )
    {
        _container.push( value );
    }

    bool empty() const
    {
        return _container.empty();
    }

    void clear()
    {
        while ( !empty() )
            pop();
    }

    void pop()
    {
        if ( empty() )
            throw CommonError( CommonError::QUEUE_IS_EMPTY );

        _container.pop();
    }

    T &peak()
    {
        if ( empty() )
            throw CommonError( CommonError::QUEUE_IS_EMPTY );

        return _container.front();
    }

protected:
    Super _container;
};

#endif // __Queue_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
