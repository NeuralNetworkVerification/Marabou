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
#include "Debug.h"
#include "Statistics.h"

BoundTightener::BoundTightener()
    : _statistics( NULL )
{
}

void BoundTightener::deriveTightenings( ITableau &tableau )
{
	
	if ( _statistics )
        _statistics->incNumRowsExaminedByRowTightener();

    // Extract the variable's row from the tableau
	unsigned numNonBasic = tableau.getN() - tableau.getM();
	unsigned enteringIndex = tableau.getEnteringVariableIndex();

	// The entering/leaving assignments are reversed because these are called post-pivot.
	unsigned enteringVariable = tableau.getLeavingVariable();
	unsigned leavingVariable = tableau.getEnteringVariable();
	
	const TableauRow &row = *tableau.getPivotRow();
	
	double enteringCoef = row[enteringIndex];

	// pre pivot row says
	// leaving = enteringCoef * entering + sum ci xi + b
	// where sum runs over nonbasic vars (that are not entering).
	// rearrange to
	// entering = leaving / enteringCoef - sum ci/enteringCoef xi
	// - b / enteringCoef
	
	// Get right hand side
    double constCoef = -row._scalar / enteringCoef;

    // Compute the lower and upper bounds from this row
	double tightenedLowerBound = constCoef;
	double tightenedUpperBound = constCoef;

	for ( unsigned i = 0; i < numNonBasic; ++i )
	{
		const TableauRow::Entry &entry( row._row[i] );
		unsigned var = entry._var;
		double coef = -entry._coefficient / enteringCoef;
		// Reuse the pass of this loop on the entering index
		// to account for the leaving / enteringCoef term above.
		if ( i == enteringIndex )
		{
			var = leavingVariable;
			coef = 1.0 / enteringCoef;
		}

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
	if ( FloatUtils::lt( tableau.getLowerBound( enteringVariable ), tightenedLowerBound ) )
    {
        enqueueTightening( Tightening( enteringVariable, tightenedLowerBound, Tightening::LB ) );
        if ( _statistics )
            _statistics->incNumBoundsProposedByRowTightener();
    }

    // Tighten upper bound if needed
	if ( FloatUtils::gt( tableau.getUpperBound( enteringVariable ), tightenedUpperBound ) )
    {
        enqueueTightening( Tightening( enteringVariable, tightenedUpperBound, Tightening::UB ) );
        if ( _statistics )
            _statistics->incNumBoundsProposedByRowTightener();
    }
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

void BoundTightener::setStatistics( Statistics *statistics )
{
    _statistics = statistics;
}

void BoundTightener::clearStoredTightenings()
{
    _tighteningRequests.clear();
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
