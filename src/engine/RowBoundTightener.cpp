/*********************                                                        */
/*! \file RowBoundTightener.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Duligur Ibeling
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "Debug.h"
#include "InfeasibleQueryException.h"
#include "ReluplexError.h"
#include "RowBoundTightener.h"
#include "Statistics.h"

RowBoundTightener::RowBoundTightener()
    : _lowerBounds( NULL )
    , _upperBounds( NULL )
    , _tightenedLower( NULL )
    , _tightenedUpper( NULL )
    , _rows( NULL )
    , _z( NULL )
    , _ciTimesLb( NULL )
    , _ciTimesUb( NULL )
    , _ciSign( NULL )
    , _statistics( NULL )
{
}

void RowBoundTightener::initialize( const ITableau &tableau )
{
    freeMemoryIfNeeded();

    _n = tableau.getN();
    _m = tableau.getM();

    _lowerBounds = new double[_n];
    if ( !_lowerBounds )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "RowBoundTightener::lowerBounds" );

    _upperBounds = new double[_n];
    if ( !_upperBounds )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "RowBoundTightener::upperBounds" );

    _tightenedLower = new bool[_n];
    if ( !_tightenedLower )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "RowBoundTightener::tightenedLower" );

    _tightenedUpper = new bool[_n];
    if ( !_tightenedUpper )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "RowBoundTightener::tightenedUpper" );

    std::fill( _tightenedLower, _tightenedLower + _n, false );
    std::fill( _tightenedUpper, _tightenedUpper + _n, false );

    for ( unsigned i = 0; i < _n; ++i )
    {
        _lowerBounds[i] = tableau.getLowerBound( i );
        _upperBounds[i] = tableau.getUpperBound( i );
    }

    if ( GlobalConfiguration::EXPLICIT_BASIS_BOUND_TIGHTENING_TYPE ==
         GlobalConfiguration::COMPUTE_INVERTED_BASIS_MATRIX )
    {
        _rows = new TableauRow *[_m];
        for ( unsigned i = 0; i < _m; ++i )
            _rows[i] = new TableauRow( _n - _m );
    }
    else if ( GlobalConfiguration::EXPLICIT_BASIS_BOUND_TIGHTENING_TYPE ==
              GlobalConfiguration::USE_IMPLICIT_INVERTED_BASIS_MATRIX )
    {
        _rows = new TableauRow *[_m];
        for ( unsigned i = 0; i < _m; ++i )
            _rows[i] = new TableauRow( _n - _m );

        _z = new double[_m];
    }

    _ciTimesLb = new double[_n - _m];
    _ciTimesUb = new double[_n - _m];
    _ciSign = new char[_n - _m];
}

void RowBoundTightener::clear( const ITableau &tableau )
{
    std::fill( _tightenedLower, _tightenedLower + _n, false );
    std::fill( _tightenedUpper, _tightenedUpper + _n, false );

    for ( unsigned i = 0; i < _n; ++i )
    {
        _lowerBounds[i] = tableau.getLowerBound( i );
        _upperBounds[i] = tableau.getUpperBound( i );
    }
}

RowBoundTightener::~RowBoundTightener()
{
    freeMemoryIfNeeded();
}

void RowBoundTightener::freeMemoryIfNeeded()
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

    if ( _rows )
    {
        for ( unsigned i = 0; i < _m; ++i )
            delete _rows[i];
        delete[] _rows;
        _rows = NULL;
    }

    if ( _z )
    {
        delete[] _z;
        _z = NULL;
    }

    if ( _ciTimesLb )
    {
        delete[] _ciTimesLb;
        _ciTimesLb = NULL;
    }

    if ( _ciTimesUb )
    {
        delete[] _ciTimesUb;
        _ciTimesUb = NULL;
    }

    if ( _ciSign )
    {
        delete[] _ciSign;
        _ciSign = NULL;
    }
}

void RowBoundTightener::examineImplicitInvertedBasisMatrix( const ITableau &tableau, bool untilSaturation )
{
    /*
      Roughly (the dimensions don't add up):

         xB = inv(B)*b - inv(B)*An
    */

    // Find z = inv(B) * b, by solving the forward transformation Bz = b
    tableau.forwardTransformation( tableau.getRightHandSide(), _z );
    for ( unsigned i = 0; i < _m; ++i )
    {
        _rows[i]->_scalar = _z[i];
        _rows[i]->_lhs = tableau.basicIndexToVariable( i );
    }

    // Now, go over the columns of the constraint martrix, perform an FTRAN
    // for each of them, and populate the rows.
    for ( unsigned i = 0; i < _n - _m; ++i )
    {
        unsigned nonBasic = tableau.nonBasicIndexToVariable( i );
        const double *ANColumn = tableau.getAColumn( nonBasic );
        tableau.forwardTransformation( ANColumn, _z );

        for ( unsigned j = 0; j < _m; ++j )
        {
            _rows[j]->_row[i]._var = nonBasic;
            _rows[j]->_row[i]._coefficient = -_z[j];
        }
    }

    // We now have all the rows, can use them for tightening.
    // The tightening procedure may throw an exception, in which case we need
    // to release the rows.
    bool newBoundsLearned;
    do
    {
        newBoundsLearned = onePassOverInvertedBasisRows( tableau );
    }
    while ( untilSaturation && newBoundsLearned );
}

void RowBoundTightener::examineInvertedBasisMatrix( const ITableau &tableau, bool untilSaturation )
{
    /*
      Roughly (the dimensions don't add up):

         xB = inv(B)*b - inv(B)*An

      We compute one row at a time.
    */

    const double *b = tableau.getRightHandSide();
    const double *invB = tableau.getInverseBasisMatrix();

    try
    {
        for ( unsigned i = 0; i < _m; ++i )
        {
            TableauRow *row = _rows[i];
            // First, compute the scalar, using inv(B)*b
            row->_scalar = 0;
            for ( unsigned j = 0; j < _m; ++j )
                row->_scalar += ( invB[i * _m + j] * b[j] );

            // Now update the row's coefficients for basic variable i
            for ( unsigned j = 0; j < _n - _m; ++j )
            {
                row->_row[j]._var = tableau.nonBasicIndexToVariable( j );

                // Dot product of the i'th row of inv(B) with the appropriate
                // column of An
                const double *ANColumn = tableau.getAColumn( row->_row[j]._var );
                row->_row[j]._coefficient = 0;
                for ( unsigned k = 0; k < _m; ++k )
                    row->_row[j]._coefficient -= ( invB[i * _m + k] * ANColumn[k] );
            }

            // Store the lhs variable
            row->_lhs = tableau.basicIndexToVariable( i );
        }

        // We now have all the rows, can use them for tightening.
        // The tightening procedure may throw an exception, in which case we need
        // to release the rows.

        bool newBoundsLearned;
        do
        {
            newBoundsLearned = onePassOverInvertedBasisRows( tableau );
        }
        while ( untilSaturation && newBoundsLearned );
    }
    catch ( ... )
    {
        delete[] invB;
        throw;
    }

    delete[] invB;
}

bool RowBoundTightener::onePassOverInvertedBasisRows( const ITableau &tableau )
{
    bool result = false;

    for ( unsigned i = 0; i < _m; ++i )
        if ( tightenOnSingleInvertedBasisRow( tableau, *( _rows[i] ) ) )
            result = true;

    return result;
}

bool RowBoundTightener::tightenOnSingleInvertedBasisRow( const ITableau &tableau, TableauRow &row )
{
	/*
      A row is of the form

         y = sum ci xi + b

      We wish to tighten once for y, but also once for every x.
    */
    unsigned n = tableau.getN();
    unsigned m = tableau.getM();

    bool foundNewBound = false;

    // Compute ci * lb, ci * ub, flag signs for all entries
    enum {
        ZERO = 0,
        POSITIVE = 1,
        NEGATIVE = 2,
    };

    for ( unsigned i = 0; i < n - m; ++i )
    {
        double ci = row[i];

        if ( FloatUtils::isZero( ci ) )
        {
            _ciSign[i] = ZERO;
            _ciTimesLb[i] = 0;
            _ciTimesUb[i] = 0;
            continue;
        }

        _ciSign[i] = FloatUtils::isPositive( ci ) ? POSITIVE : NEGATIVE;

        unsigned xi = row._row[i]._var;
        _ciTimesLb[i] = ci * _lowerBounds[xi];
        _ciTimesUb[i] = ci * _upperBounds[xi];
    }

    // Start with a pass for y
    unsigned y = row._lhs;
    double upperBound = row._scalar;
    double lowerBound = row._scalar;

    unsigned xi;
    double ci;

    for ( unsigned i = 0; i < n - m; ++i )
    {
        if ( _ciSign[i] == POSITIVE )
        {
            lowerBound += _ciTimesLb[i];
            upperBound += _ciTimesUb[i];
        }
        else
        {
            lowerBound += _ciTimesUb[i];
            upperBound += _ciTimesLb[i];
        }
    }

    if ( FloatUtils::lt( _lowerBounds[y], lowerBound ) )
    {
        _lowerBounds[y] = lowerBound;
        _tightenedLower[y] = true;
        foundNewBound = true;
    }

    if ( FloatUtils::gt( _upperBounds[y], upperBound ) )
    {
        _upperBounds[y] = upperBound;
        _tightenedUpper[y] = true;
        foundNewBound = true;
    }

    if ( FloatUtils::gt( _lowerBounds[y], _upperBounds[y] ) )
        throw InfeasibleQueryException();

    // Next, do a pass for each of the rhs variables.
    // For this, we wish to logically transform the equation into:
    //
    //     xi = 1/ci * ( y - sum cj xj - b )
    //
    // And then compute the upper/lower bounds for xi.
    //
    // However, for efficiency, we compute the lower and upper
    // bounds of the expression:
    //
    //         y - sum ci xi - b
    //
    // Then, when we consider xi we adjust the computed lower and upper
    // boudns accordingly.

    double auxLb = _lowerBounds[y] - row._scalar;
    double auxUb = _upperBounds[y] - row._scalar;

    // Now add ALL xi's
    for ( unsigned i = 0; i < n - m; ++i )
    {
        if ( _ciSign[i] == NEGATIVE )
        {
            auxLb -= _ciTimesLb[i];
            auxUb -= _ciTimesUb[i];
        }
        else
        {
            auxLb -= _ciTimesUb[i];
            auxUb -= _ciTimesLb[i];
        }
    }

    // Now consider each individual xi
    for ( unsigned i = 0; i < n - m; ++i )
    {
        // If ci = 0, nothing to do.
        if ( _ciSign[i] == ZERO )
            continue;

        lowerBound = auxLb;
        upperBound = auxUb;

        // Adjust the aux bounds to remove xi
        if ( _ciSign[i] == NEGATIVE )
        {
            lowerBound += _ciTimesLb[i];
            upperBound += _ciTimesUb[i];
        }
        else
        {
            lowerBound += _ciTimesUb[i];
            upperBound += _ciTimesLb[i];
        }

        // Now divide everything by ci, switching signs if needed.
        ci = row[i];
        lowerBound = lowerBound / ci;
        upperBound = upperBound / ci;

        if ( _ciSign[i] == NEGATIVE )
        {
            double temp = upperBound;
            upperBound = lowerBound;
            lowerBound = temp;
        }

        // If a tighter bound is found, store it
        xi = row._row[i]._var;
        if ( FloatUtils::lt( _lowerBounds[xi], lowerBound ) )
        {
            _lowerBounds[xi] = lowerBound;
            _tightenedLower[xi] = true;
            foundNewBound = true;
        }

        if ( FloatUtils::gt( _upperBounds[xi], upperBound ) )
        {
            _upperBounds[xi] = upperBound;
            _tightenedUpper[xi] = true;
            foundNewBound = true;
        }

        if ( FloatUtils::gt( _lowerBounds[xi], _upperBounds[xi] ) )
            throw InfeasibleQueryException();
    }

    return foundNewBound;
}

void RowBoundTightener::examineBasisMatrix( const ITableau &tableau, bool untilSaturation )
{
    bool newBoundsLearned;

    /*
      If working until saturation, do single passes over the matrix until no new bounds
      are learned. Otherwise, just do a single pass.
    */
    do
    {
        newBoundsLearned = onePassOverBasisMatrix( tableau );
    }
    while ( untilSaturation && newBoundsLearned );
}

bool RowBoundTightener::onePassOverBasisMatrix( const ITableau &tableau )
{
    bool result = false;

    List<Equation *> basisEquations;
    tableau.getBasisEquations( basisEquations );
    for ( const auto &equation : basisEquations )
        for ( const auto &addend : equation->_addends )
            if ( tightenOnSingleEquation( *equation, addend ) )
                result = true;

    for ( const auto &equation : basisEquations )
        delete equation;

    return result;
}

bool RowBoundTightener::tightenOnSingleEquation( Equation &equation,
                                                 Equation::Addend varBeingTightened )
{
    ASSERT( !FloatUtils::isZero( varBeingTightened._coefficient ) );
    bool foundNewBound = false;

    // The equation is of the form a * varBeingTightened + sum (bi * xi) = c,
    // or: a * varBeingTightened = c - sum (bi * xi)

    // We first compute the lower and upper bounds for the expression c - sum (bi * xi)
    double upperBound = equation._scalar;
    double lowerBound = equation._scalar;

    for ( auto addend : equation._addends )
    {
        if ( varBeingTightened._variable == addend._variable )
            continue;

        double addendLB = _lowerBounds[addend._variable];
        double addendUB = _upperBounds[addend._variable];

        if ( FloatUtils::isNegative( addend._coefficient ) )
        {
            lowerBound -= addend._coefficient * addendLB;
            upperBound -= addend._coefficient * addendUB;
        }

        if ( FloatUtils::isPositive( addend._coefficient ) )
        {
            lowerBound -= addend._coefficient * addendUB;
            upperBound -= addend._coefficient * addendLB;
        }
    }

    // We know that lb < a * varBeingTightened < ub.
    // We want to divide by a, but we care about the sign:
    //    If a is positive: lb/a < x < ub/a
    //    If a is negative: lb/a > x > ub/a
    lowerBound = lowerBound / varBeingTightened._coefficient;
    upperBound = upperBound / varBeingTightened._coefficient;

    if ( FloatUtils::isNegative( varBeingTightened._coefficient ) )
    {
        double temp = upperBound;
        upperBound = lowerBound;
        lowerBound = temp;
    }

    // Tighten lower bound if needed
    if ( FloatUtils::lt( _lowerBounds[varBeingTightened._variable], lowerBound ) )
    {
        _lowerBounds[varBeingTightened._variable] = lowerBound;
        _tightenedLower[varBeingTightened._variable] = true;
        foundNewBound = true;
    }

    // Tighten upper bound if needed
    if ( FloatUtils::gt( _upperBounds[varBeingTightened._variable], upperBound ) )
    {
        _upperBounds[varBeingTightened._variable] = upperBound;
        _tightenedUpper[varBeingTightened._variable] = true;
        foundNewBound = true;
    }

    if ( FloatUtils::gt( _lowerBounds[varBeingTightened._variable],
                         _upperBounds[varBeingTightened._variable] ) )
        throw InfeasibleQueryException();

    return foundNewBound;
}

void RowBoundTightener::examineConstraintMatrix( const ITableau &tableau, bool untilSaturation )
{
    bool newBoundsLearned;

    /*
      If working until saturation, do single passes over the matrix until no new bounds
      are learned. Otherwise, just do a single pass.
    */
    do
    {
        newBoundsLearned = onePassOverConstraintMatrix( tableau );
    }
    while ( untilSaturation && newBoundsLearned );
}

bool RowBoundTightener::onePassOverConstraintMatrix( const ITableau &tableau )
{
    bool result = false;

    unsigned n = tableau.getN();
    unsigned m = tableau.getM();

    for ( unsigned i = 0; i < m; ++i )
        for ( unsigned j = 0; j < n; ++j )
            if ( tightenOnSingleConstraintRow( tableau, i, j ) )
                result = true;

    return result;
}

bool RowBoundTightener::tightenOnSingleConstraintRow( const ITableau &tableau,
                                                      unsigned row,
                                                      unsigned varBeingTightened )
{
    const double *A = tableau.getA();
    const double *b = tableau.getRightHandSide();
    unsigned n = tableau.getN();
    unsigned m = tableau.getM();

    double tightenedCoefficient = A[varBeingTightened * m + row];
    if ( FloatUtils::isZero( tightenedCoefficient ) )
        return false;

    bool foundNewBound = false;

    // Initialize both bounds using the right hand side of the row
    double tightenedLowerBound = b[row] / tightenedCoefficient;
	double tightenedUpperBound = tightenedLowerBound;

	for ( unsigned i = 0; i < n; ++i )
	{
        if ( i == varBeingTightened )
            continue;

		double coefficient = -A[i * m + row] / tightenedCoefficient;

		if ( FloatUtils::isPositive( coefficient ) )
		{
			tightenedLowerBound += coefficient * _lowerBounds[i];
			tightenedUpperBound += coefficient * _upperBounds[i];
		}
		else if ( FloatUtils::isNegative( coefficient ) )
		{
			tightenedLowerBound += coefficient * _upperBounds[i];
			tightenedUpperBound += coefficient * _lowerBounds[i];
		}
    }

    // Tighten lower bound if needed
	if ( FloatUtils::lt( _lowerBounds[varBeingTightened], tightenedLowerBound ) )
    {
        _lowerBounds[varBeingTightened] = tightenedLowerBound;
        _tightenedLower[varBeingTightened] = true;
        foundNewBound = true;
    }

    // Tighten upper bound if needed
	if ( FloatUtils::gt( _upperBounds[varBeingTightened], tightenedUpperBound ) )
    {
        _upperBounds[varBeingTightened] = tightenedUpperBound;
        _tightenedUpper[varBeingTightened] = true;
        foundNewBound = true;
    }

    if ( FloatUtils::gt( _lowerBounds[varBeingTightened], _upperBounds[varBeingTightened] ) )
        throw InfeasibleQueryException();

    return foundNewBound;
}

void RowBoundTightener::examinePivotRow( ITableau &tableau )
{
	if ( _statistics )
        _statistics->incNumRowsExaminedByRowTightener();

	// The entering/leaving assignments are reversed because these are called post-pivot.
	unsigned enteringVariable = tableau.getLeavingVariable();
	unsigned leavingVariable = tableau.getEnteringVariable();

	const TableauRow &row = *tableau.getPivotRow();

    unsigned enteringIndex = tableau.getEnteringVariableIndex();
	double enteringCoef = row[enteringIndex];

	/*
      The pre-pivot row says:

         leaving = enteringCoef * entering + sum ci xi + b

      where sum runs over nonbasic vars (that are not entering).
      Rearrange to

         entering = leaving / enteringCoef - sum ci/enteringCoef xi - b / enteringCoef
    */

	// Get right hand side
    double constCoef = -row._scalar / enteringCoef;

    // Compute the lower and upper bounds from this row
	double tightenedLowerBound = constCoef;
	double tightenedUpperBound = constCoef;

    unsigned numNonBasic = tableau.getN() - tableau.getM();
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
	if ( FloatUtils::lt( _lowerBounds[enteringVariable], tightenedLowerBound ) )
    {
        _lowerBounds[enteringVariable] = tightenedLowerBound;
        _tightenedLower[enteringVariable] = true;
    }

    // Tighten upper bound if needed
	if ( FloatUtils::gt( _upperBounds[enteringVariable], tightenedUpperBound ) )
    {
        _upperBounds[enteringVariable] = tightenedUpperBound;
        _tightenedUpper[enteringVariable] = true;
    }

    if ( FloatUtils::gt( _lowerBounds[enteringVariable], _upperBounds[enteringVariable] ) )
        throw InfeasibleQueryException();
}

void RowBoundTightener::getRowTightenings( List<Tightening> &tightenings ) const
{
    for ( unsigned i = 0; i < _n; ++i )
    {
        if ( _tightenedLower[i] )
        {
            tightenings.append( Tightening( i, _lowerBounds[i], Tightening::LB ) );
            _tightenedLower[i] = false;
        }

        if ( _tightenedUpper[i] )
        {
            tightenings.append( Tightening( i, _upperBounds[i], Tightening::UB ) );
            _tightenedUpper[i] = false;
        }
    }
}

void RowBoundTightener::setStatistics( Statistics *statistics )
{
    _statistics = statistics;
}

void RowBoundTightener::notifyLowerBound( unsigned variable, double bound )
{
    if ( FloatUtils::gt( bound, _lowerBounds[variable] ) )
    {
        _lowerBounds[variable] = bound;
        _tightenedLower[variable] = false;
    }
}

void RowBoundTightener::notifyUpperBound( unsigned variable, double bound )
{
    if ( FloatUtils::lt( bound, _upperBounds[variable] ) )
    {
        _upperBounds[variable] = bound;
        _tightenedUpper[variable] = false;
    }
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
