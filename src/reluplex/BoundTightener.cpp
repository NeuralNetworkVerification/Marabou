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

void BoundTightener::deriveTightenings( ITableau &tableau, unsigned leavingVariable )
{
    if ( _statistics )
        _statistics->incNumRowsExaminedByRowTightener();

    // Extract the variable's row from the tableau
	unsigned numNonBasic = tableau.getN() - tableau.getM();
	TableauRow row( numNonBasic );
	tableau.getTableauRow( tableau.variableToIndex( leavingVariable ), &row );

    // const TableauRow &row = *tableau.getPivotRow();

	// Get right hand side
    double constCoef = row._scalar;

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

	// unsigned leavingVariable = tableau.getLeavingVariable();

    // Tighten lower bound if needed
	if ( FloatUtils::lt( tableau.getLowerBound( leavingVariable ), tightenedLowerBound ) )
    {
        enqueueTightening( Tightening( leavingVariable, tightenedLowerBound, Tightening::LB ) );
        if ( _statistics )
            _statistics->incNumBoundsProposedByRowTightener();
    }

    // Tighten upper bound if needed
	if ( FloatUtils::gt( tableau.getUpperBound( leavingVariable ), tightenedUpperBound ) )
    {
        enqueueTightening( Tightening( leavingVariable, tightenedUpperBound, Tightening::UB ) );
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
