/*********************                                                        */
/*! \file ConstraintBoundTightener.cpp
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

#include "ConstraintBoundTightener.h"
#include "FloatUtils.h"
#include "MarabouError.h"
#include "Statistics.h"
#include "InfeasibleQueryException.h"


ConstraintBoundTightener::ConstraintBoundTightener( ITableau &tableau, IEngine &engine )
    : _tableau( tableau )
    , _lowerBounds( NULL )
    , _upperBounds( NULL )
    , _tightenedLower( NULL )
    , _tightenedUpper( NULL )
    , _statistics( NULL )
    , _engine (engine)
{
}

void ConstraintBoundTightener::setDimensions()
{
    freeMemoryIfNeeded();

    _n = _tableau.getN();
    _m = _tableau.getM();

    _lowerBounds = new double[_n];
    if ( !_lowerBounds )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "ConstraintBoundTightener::lowerBounds" );

    _upperBounds = new double[_n];
    if ( !_upperBounds )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "ConstraintBoundTightener::upperBounds" );

    _tightenedLower = new bool[_n];
    if ( !_tightenedLower )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "ConstraintBoundTightener::tightenedLower" );

    _tightenedUpper = new bool[_n];
    if ( !_tightenedUpper )
		throw MarabouError( MarabouError::ALLOCATION_FAILED, "ConstraintBoundTightener::tightenedUpper" );

	resetBounds();
}

void ConstraintBoundTightener::resetBounds()
{
    std::fill( _tightenedLower, _tightenedLower + _n, false );
    std::fill( _tightenedUpper, _tightenedUpper + _n, false );

    for ( unsigned i = 0; i < _n; ++i )
    {
        _lowerBounds[i] = _tableau.getLowerBound( i );
        _upperBounds[i] = _tableau.getUpperBound( i );
    }
}

ConstraintBoundTightener::~ConstraintBoundTightener()
{
    freeMemoryIfNeeded();
}

void ConstraintBoundTightener::freeMemoryIfNeeded()
{
    if ( _lowerBounds )
    {
        delete[] _lowerBounds;
        _lowerBounds = NULL;
    }

    if ( _upperBounds )
    {
        delete[] _upperBounds;
        _upperBounds = NULL;
    }

    if ( _tightenedLower )
    {
        delete[] _tightenedLower;
        _tightenedLower = NULL;
    }

    if ( _tightenedUpper )
    {
        delete[] _tightenedUpper;
        _tightenedUpper = NULL;
    }

	clearEngineUpdates();
}

void ConstraintBoundTightener::setStatistics( Statistics *statistics )
{
    _statistics = statistics;
}

void ConstraintBoundTightener::notifyLowerBound( unsigned variable, double bound )
{
    if ( bound > _lowerBounds[variable] )
    {
        _lowerBounds[variable] = bound;
        _tightenedLower[variable] = false;
    }
}

void ConstraintBoundTightener::notifyUpperBound( unsigned variable, double bound )
{
    if ( bound < _upperBounds[variable] )
    {
        _upperBounds[variable] = bound;
        _tightenedUpper[variable] = false;
    }
}

void ConstraintBoundTightener::notifyDimensionChange( unsigned /* m */ , unsigned /* n */ )
{
    setDimensions();
}

void ConstraintBoundTightener::registerTighterLowerBound( unsigned variable, double bound )
{
    if ( bound > _lowerBounds[variable] )
    {
        _lowerBounds[variable] = bound;
        _tightenedLower[variable] = true;
    }
}

void ConstraintBoundTightener::registerTighterUpperBound( unsigned variable, double bound )
{
    if ( bound < _upperBounds[variable] )
    {
        _upperBounds[variable] = bound;
        _tightenedUpper[variable] = true;
    }
}

void ConstraintBoundTightener::registerTighterLowerBound( unsigned variable, double bound, const SparseUnsortedList& row )
{
	if ( bound > _lowerBounds[variable] )
	{
		if ( GlobalConfiguration::PROOF_CERTIFICATE )
			_tableau.updateExplanation( row, false, variable );

		registerTighterLowerBound( variable, bound );
	}
}

void ConstraintBoundTightener::registerTighterUpperBound( unsigned variable, double bound, const SparseUnsortedList& row )
{
	if ( bound < _upperBounds[variable] )
	{
		if ( GlobalConfiguration::PROOF_CERTIFICATE )
			_tableau.updateExplanation( row, true, variable );

		registerTighterUpperBound( variable, bound );
	}
}

void ConstraintBoundTightener::ConstraintBoundTightener::getConstraintTightenings( List<Tightening> &tightenings ) const
{
    for ( unsigned i = 0; i < _n; ++i )
    {
        if ( _tightenedLower[i] )
            tightenings.append( Tightening( i, _lowerBounds[i], Tightening::LB ) );

        if ( _tightenedUpper[i] )
            tightenings.append( Tightening( i, _upperBounds[i], Tightening::UB ) );
    }
}

std::list<double> ConstraintBoundTightener::getUGBUpdates() const
{
	return _upperGBUpdates;
}

std::list<double> ConstraintBoundTightener::getLGBUpdates() const
{
	return _lowerGBUpdates;
}

std::list<std::vector<double>> ConstraintBoundTightener::getTableauUpdates() const
{
	return _initialTableauUpdates;
}

void ConstraintBoundTightener::clearEngineUpdates()
{
	_lowerGBUpdates.clear();
	_upperGBUpdates.clear();
	_initialTableauUpdates.clear();
}

void ConstraintBoundTightener::replaceEquationAndAdd( unsigned var, const Equation& eq )
{
	_engine.applyAllConstraintTightenings(); // TODO consider design. Needed since adding rows deletes all info

	// Assuming equation is EQ type
	unsigned newAux = _tableau.addEquation( eq ), n = _tableau.getN(), m = _tableau.getM();
	// Register engine updates
	std::vector<double> newITRow ( n + 1, 0 );
	for (auto addend : eq._addends )
		newITRow[addend._variable] = addend._coefficient;
	newITRow[newAux] = 1;
	newITRow[n] = eq._scalar;

	// Make all previous updates of same length
	for ( auto &row : _initialTableauUpdates )
	{
		row.insert(row.end() - 1, 0);
		assert( row.size() == newITRow.size() );
	}

	_initialTableauUpdates.push_back( newITRow );


	// Enforcing the new aux var to be zero
	_lowerGBUpdates.push_back( 0 );
	_upperGBUpdates.push_back( 0 );

	registerTighterUpperBound( newAux, 0 );
	registerTighterLowerBound( newAux, 0 );

	// Assuming equation is of type var = scalar
	// For generalization, can possibly compute bound imposed by row
	registerTighterUpperBound( var, eq._scalar );
	registerTighterLowerBound( var, eq._scalar );

	// Injects the new explanation - the last row
	// No need for a tightening row, and avoid possible bug of adding a basic variable as non-basic
	SingleVarBoundsExplanator expl ( m );
	expl.injectEntry( m - 1, 1, true );
	_tableau.injectExplanation(var, expl);
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
