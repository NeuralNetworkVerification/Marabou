/*********************                                                        */
/*! \file Tightening.h
** \verbatim
** Top contributors (to current version):
**   Duligur Ibeling
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __Tightening_h__
#define __Tightening_h__

#include "ITableau.h"

class Tightening
{
public:
	enum BoundType {
		LB = 0,
		UB = 1,
    };

    Tightening( unsigned variable, double value, BoundType type )
        : _variable( variable )
        , _value ( value )
        , _type( type )
    {
    }

	/*
	  The variable to tighten.
	*/
	unsigned _variable;

	/*
	  Its new value.
	*/
	double _value;

	/*
	  Whether the tightening tightens the
	  lower bound or the upper bound.
	*/
    BoundType _type;

    /*
      Equality operator.
    */
    bool operator==( const Tightening &other ) const
    {
        return
            ( _variable == other._variable ) &&
            ( _value == other._value ) &&
            ( _type == other._type );
    }
};

#endif // __Tightening_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
