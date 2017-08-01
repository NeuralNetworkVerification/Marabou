/*********************                                                        */
/*! \file BoundTightener.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Duligur Ibeling
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "BoundTightener.h"

void BoundTightener::deriveTightenings( ITableau &tableau, unsigned variable )
{
    // Extract the variable's row from the tableau
	unsigned numNonBasic = tableau.getN() - tableau.getM();
	TableauRow row( numNonBasic );
	tableau.getTableauRow( tableau.variableToIndex( variable ), &row );

	// Get right hand side
	tableau.computeRightHandSide();
	double constCoef = tableau.getRightHandSide()[tableau.variableToIndex( variable )];

    // Compute the lower and upper bounds from this row
	double tightenedLowerBound = constCoef;
	double tightenedUpperBound = constCoef;
	for ( unsigned i = 0; i < numNonBasic; ++i )
	{
		const TableauRow::Entry &entry( row._row[i] );
		unsigned var = entry._var;
		double coef = entry._coefficient;
		double currentLowerBound = tableau.getLowerBound( var );
		double currentUpperBound = tableau.getUpperBound( var );

		if ( FloatUtils::isPositive( coef ) )
		{
			tightenedLowerBound += coef * currentLowerBound;
			tightenedUpperBound += coef * currentUpperBound;
		}
		else if ( FloatUtils::isNegative( coef ) )
		{
			tightenedLowerBound += coef * currentUpperBound;
			tightenedUpperBound += coef * currentLowerBound;
		}
	}

    // Tighten lower bound if needed
	if ( FloatUtils::lt( tableau.getLowerBound( variable ), tightenedLowerBound ) )
		enqueueTightening( Tightening( variable, tightenedLowerBound, Tightening::LB ) );

    // Tighten upper bound if needed
	if ( FloatUtils::gt( tableau.getUpperBound( variable ), tightenedUpperBound ) )
		enqueueTightening( Tightening( variable, tightenedUpperBound, Tightening::UB ) );
}

void BoundTightener::enqueueTightening( const Tightening& tightening )
{
	_tighteningRequests.push( tightening );
}

void BoundTightener::tighten( ITableau &tableau )
{
	while ( !_tighteningRequests.empty() )
    {
		_tighteningRequests.peak().tighten( tableau );
		_tighteningRequests.pop();
	}
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
