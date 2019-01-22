/*********************                                                        */
/*! \file Stack.h
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

#ifndef __Stack_h__
#define __Stack_h__

#include "CommonError.h"
#include <stack>

template<class T>
class Stack
{
    typedef std::stack<T> Super;
public:

    void push( T value )
    {
        _container.push( value );
    }

    bool empty() const
    {
        return _container.empty();
    }

    unsigned size() const
    {
        return _container.size();
    }

    void clear()
    {
        unsigned stackSize = size();
        for ( unsigned i = 0; i < stackSize; ++i )
            _container.pop();
    }

    void pop()
    {
        if ( empty() )
            throw CommonError( CommonError::STACK_IS_EMPTY );

        _container.pop();
    }

    T &top()
    {
        if ( empty() )
            throw CommonError( CommonError::STACK_IS_EMPTY );

        return _container.top();
    }

protected:
    Super _container;
};

#endif // __Stack_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
