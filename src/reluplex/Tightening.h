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
		UB,
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

    // TODO: the function below really belongs in ITableau,
    // but it shouldn't be pure virtual, since it
    // just calls the pure virtaul functions tightenLB/UB.
    // can we make ITableau not an interface?
    /*
	  Tighten this request in the given tableau.
	*/
	void tighten( ITableau &tableau ) const
    {
        switch ( _type )
        {
            case Tightening::BoundType::LB:
                tableau.tightenLowerBound( _variable, _value );
                break;
            case Tightening::BoundType::UB:
                tableau.tightenUpperBound( _variable, _value );
                break;
	    }
    }
};

#endif // __Tightening_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
