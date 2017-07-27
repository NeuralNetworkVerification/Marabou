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

bool Tightening::tighten( ITableau& tableau ) const
{
	switch (type) {
		case Tightening::BoundType::LB:
			tableau.tightenLowerBound( variable, value );
			break;
		case Tightening::BoundType::UB:
			tableau.tightenUpperBound( variable, value );
			break;
	}
	return tableau.boundsValid( variable );
}

void BoundTightener::deriveTightenings( ITableau& tableau, unsigned variable )
{
	unsigned numNonBasic = tableau.getN() - tableau.getM();
	TableauRow row( numNonBasic );
	tableau.getTableauRow( variable, &row ); // ???
	double tightenedLowerBound = 0.0;
	double tightenedUpperBound = 0.0;
	for ( unsigned i = 0; i < numNonBasic; ++i )
	{
		const TableauRow::Entry& entry = row._row[ i ];
		unsigned var = entry._var;
		double coef = entry._coefficient;
		double currentLowerBound = tableau.getLowerBound( var );
		double currentUpperBound = tableau.getUpperBound( var );
		
		if( FloatUtils::isPositive( coef ) )
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

	if ( FloatUtils::lt( tableau.getLowerBound( variable ), tightenedLowerBound ) )
	{
		Tightening tightening;
		tightening.variable = variable;
		tightening.value = tightenedLowerBound;
		tightening.type = Tightening::BoundType::LB;
		enqueueTightening( tightening );
	}
	if ( FloatUtils::gt( tableau.getUpperBound( variable ), tightenedUpperBound ) )
	{
		Tightening tightening;
		tightening.variable = variable;
		tightening.value = tightenedUpperBound;
		tightening.type = Tightening::BoundType::UB;
		enqueueTightening( tightening );
	}
}

void BoundTightener::enqueueTightening( const Tightening& tightening )
{
	_tighteningRequests.push( tightening );
}

bool BoundTightener::tighten( ITableau& tableau )
{
	while(!_tighteningRequests.empty()) {
		const Tightening& request = _tighteningRequests.peak();
		bool valid = request.tighten( tableau );
		_tighteningRequests.pop();
		if ( !valid )
		{
			_tighteningRequests.clear();
			return false;
		}
	}
	return true;
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
