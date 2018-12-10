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
#include "SparseUnsortedList.h"
#include "Statistics.h"

RowBoundTightener::RowBoundTightener( const ITableau &tableau )
    : _tableau( tableau )
    , _lowerBounds( NULL )
    , _upperBounds( NULL )
    , _tightenedLower( NULL )
    , _tightenedUpper( NULL )
    , _rows( NULL )
    , _z( NULL )
    , _ciTimesLb( NULL )
    , _ciTimesUb( NULL )
    , _ciSign( NULL )
    , _lastPivotRows( NULL )
    , _haveStoredPivotRow( NULL )
    , _pivotRowAges( NULL )
    , _iteration( 0 )
    // , _pivotRowIndex( 0 )
    // , _numStoredPivotRows( 0 )
    , _statistics( NULL )
{
}

void RowBoundTightener::setDimensions()
{
    freeMemoryIfNeeded();

    _n = _tableau.getN();
    _m = _tableau.getM();

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

    _lastPivotRows = new SparseTableauRow *[_m];
    for ( unsigned i = 0; i < _m; ++i )
        _lastPivotRows[i] = new SparseTableauRow( _n - _m );

    _haveStoredPivotRow = new bool[_m];
    if ( !_haveStoredPivotRow )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "RowBoundTightener::haveStoredPivotRow" );

    _pivotRowAges = new unsigned long long[_m];
    if ( !_pivotRowAges )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "RowBoundTightener::pivotRowAges" );

    // _pivotRowIndex = 0;
    // _numStoredPivotRows = 0;

    resetBounds();

    if ( GlobalConfiguration::EXPLICIT_BASIS_BOUND_TIGHTENING_TYPE ==
         GlobalConfiguration::COMPUTE_INVERTED_BASIS_MATRIX )
    {
        _rows = new SparseTableauRow *[_m];
        for ( unsigned i = 0; i < _m; ++i )
            _rows[i] = new SparseTableauRow( _n - _m );
    }
    else if ( GlobalConfiguration::EXPLICIT_BASIS_BOUND_TIGHTENING_TYPE ==
              GlobalConfiguration::USE_IMPLICIT_INVERTED_BASIS_MATRIX )
    {
        _rows = new SparseTableauRow *[_m];
        for ( unsigned i = 0; i < _m; ++i )
            _rows[i] = new SparseTableauRow( _n - _m );

        _z = new double[_m];
    }

    _ciTimesLb = new double[_n];
    _ciTimesUb = new double[_n];
    _ciSign = new char[_n];
}

void RowBoundTightener::resetBounds()
{
    std::fill( _tightenedLower, _tightenedLower + _n, false );
    std::fill( _tightenedUpper, _tightenedUpper + _n, false );

    for ( unsigned i = 0; i < _n; ++i )
    {
        _lowerBounds[i] = _tableau.getLowerBound( i );
        _upperBounds[i] = _tableau.getUpperBound( i );
    }

    std::fill_n( _haveStoredPivotRow, _m, false );
    std::fill_n( _pivotRowAges, _m, 0 );
}

void RowBoundTightener::clear()
{
    std::fill( _tightenedLower, _tightenedLower + _n, false );
    std::fill( _tightenedUpper, _tightenedUpper + _n, false );

    for ( unsigned i = 0; i < _n; ++i )
    {
        _lowerBounds[i] = _tableau.getLowerBound( i );
        _upperBounds[i] = _tableau.getUpperBound( i );
    }

    std::fill_n( _haveStoredPivotRow, _m, false );
    std::fill_n( _pivotRowAges, _m, false );
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

    if ( _lastPivotRows )
    {
        for ( unsigned i = 0; i < _m; ++i )
            delete _lastPivotRows[i];
        delete[] _lastPivotRows;
        _lastPivotRows = NULL;
    }

    if ( _haveStoredPivotRow )
    {
        delete[] _haveStoredPivotRow;
        _haveStoredPivotRow = NULL;
    }

    if ( _pivotRowAges )
    {
        delete[] _pivotRowAges;
        _pivotRowAges = NULL;
    }

    // _pivotRowIndex = 0;
    // _numStoredPivotRows = 0;

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

void RowBoundTightener::examineImplicitInvertedBasisMatrix( Saturation saturation )
{
    /*
      Roughly (the dimensions don't add up):

         xB = inv(B)*b - inv(B)*An
    */

    // Find z = inv(B) * b, by solving the forward transformation Bz = b
    _tableau.forwardTransformation( _tableau.getRightHandSide(), _z );
    for ( unsigned i = 0; i < _m; ++i )
    {
        _rows[i]->_scalar = _z[i];
        _rows[i]->_lhs = _tableau.basicIndexToVariable( i );
    }

    // Now, go over the columns of the constraint martrix, perform an FTRAN
    // for each of them, and populate the rows.
    for ( unsigned j = 0; j < _m; ++j )
        _rows[j]->clear();

    for ( unsigned i = 0; i < _n - _m; ++i )
    {
        unsigned nonBasic = _tableau.nonBasicIndexToVariable( i );
        const double *ANColumn = _tableau.getAColumn( nonBasic );
        _tableau.forwardTransformation( ANColumn, _z );

        for ( unsigned j = 0; j < _m; ++j )
            _rows[j]->append( i, nonBasic, -_z[j] );
    }

    // We now have all the rows, can use them for tightening.
    // The tightening procedure may throw an exception, in which case we need
    // to release the rows.
    unsigned newBoundsLearned;
    do
    {
        newBoundsLearned = onePassOverInvertedBasisRows();

        if ( _statistics && ( newBoundsLearned > 0 ) )
            _statistics->incNumTighteningsFromExplicitBasis( newBoundsLearned );
    }
    while ( ( saturation == UNTIL_SATURATION ) && ( newBoundsLearned > 0 ) );
}

void RowBoundTightener::examineInvertedBasisMatrix( const double *invertedBasis,
                                                    const double *rhs,
                                                    Saturation saturation )
{
    extractRowsFromInvertedBasisMatrix( invertedBasis, rhs );
    examineInvertedBasisMatrix( saturation );
}

void RowBoundTightener::extractRowsFromInvertedBasisMatrix( const double *invertedBasis, const double *rhs )
{
    /*
      Roughly (the dimensions don't add up):

         xB = inv(B)*b - inv(B)*An

      We compute one row at a time.
    */

    const double *b = rhs;
    const double *invB = invertedBasis;

    unsigned count = 0;

    // printf( "Dumping row ages...\n" );
    // for ( unsigned i = 0; i < _m; ++i )
    // {
    //     printf( "\trow %u: %llu\n", i, _pivotRowAges[i] );
    // }
    // printf( "\n" );

    for ( unsigned i = 0; i < _m; ++i )
    {
        if ( GlobalConfiguration::USE_STORED_PIVOT_ROWS && _haveStoredPivotRow[i] && ( _iteration <= _pivotRowAges[i] + 100  ) ) // + 300
            continue;

        // if ( _haveStoredPivotRow[i] && !( _iteration  <= _pivotRowAges[i] + 300  ) )
        // {
        //     printf( "Refreshing row %u, %llu\n", i, _iteration - _pivotRowAges[i] );

        // }

        ++count;

        _haveStoredPivotRow[i] = true;
        _pivotRowAges[i] = _iteration;

        SparseTableauRow *row = _rows[i];
        row->clear();

        // First, compute the scalar, using inv(B)*b
        row->_scalar = 0;
        for ( unsigned j = 0; j < _m; ++j )
            row->_scalar += ( invB[i * _m + j] * b[j] );

        // Now update the row's coefficients for basic variable i
        for ( unsigned j = 0; j < _n - _m; ++j )
        {
            // Dot product of the i'th row of inv(B) with the appropriate
            // column of An
            unsigned nonBasic = _tableau.nonBasicIndexToVariable( j );
            double coefficient = 0;
            const SparseUnsortedList *column = _tableau.getSparseAColumn( nonBasic );

            for ( const auto &entry : *column )
                coefficient -= invB[i*_m + entry._index] * entry._value;

            if ( !FloatUtils::isZero( coefficient ) )
                row->append( j, nonBasic, coefficient );
        }

        // Store the lhs variable
        row->_lhs = _tableau.basicIndexToVariable( i );
    }

    // printf( "RBT: conputed %u rows from scratch\n", count );

}

void RowBoundTightener::examineInvertedBasisMatrix( Saturation saturation )
{
    // The _rows have all been extracted, and now we use them for tightening.
    unsigned newBoundsLearned;
    do
    {
        newBoundsLearned = onePassOverInvertedBasisRows();

        if ( _statistics && ( newBoundsLearned > 0 ) )
            _statistics->incNumTighteningsFromExplicitBasis( newBoundsLearned );
    }
    while ( ( saturation == UNTIL_SATURATION ) && ( newBoundsLearned > 0 ) );
}


unsigned RowBoundTightener::onePassOverInvertedBasisRows()
{
    unsigned newBounds = 0;

    for ( unsigned i = 0; i < _m; ++i )
        newBounds += tightenOnSingleInvertedBasisRow( *( _rows[i] ) );

    return newBounds;
}

unsigned RowBoundTightener::tightenOnSingleInvertedStoredBasisRow( unsigned rowIndex )
{
    ASSERT( rowIndex < _m );

    if ( !GlobalConfiguration::USE_STORED_PIVOT_ROWS )
        return tightenOnSingleInvertedBasisRow( *( _rows[rowIndex] ) );
    else
    {
        if ( !_haveStoredPivotRow[rowIndex] )
            return 0;

        return tightenOnSingleInvertedBasisRow( *( _rows[rowIndex] ) );
    }
}

unsigned RowBoundTightener::tightenOnSingleInvertedBasisRow( const SparseTableauRow &row )
{
	/*
      A row is of the form

         y = sum ci xi + b

      We wish to tighten once for y, but also once for every x.
    */
    unsigned n = _tableau.getN();
    unsigned m = _tableau.getM();

    unsigned result = 0;

    // Compute ci * lb, ci * ub, flag signs for all entries
    enum {
        ZERO = 0,
        POSITIVE = 1,
        NEGATIVE = 2,
    };

    for ( unsigned i = 0; i < n - m; ++i )
    {
        _ciSign[i] = ZERO;
        _ciTimesLb[i] = 0;
        _ciTimesUb[i] = 0;
    }

    for ( auto entry = row.begin(); entry != row.end(); ++entry )
    {
        unsigned i = entry->_index;
        double ci = entry->_coefficient;
        unsigned xi = entry->_variable;

        _ciSign[i] = FloatUtils::isPositive( ci ) ? POSITIVE : NEGATIVE;
        _ciTimesLb[i] = ci * _lowerBounds[xi];
        _ciTimesUb[i] = ci * _upperBounds[xi];
    }

    // Start with a pass for y
    unsigned y = row._lhs;
    double upperBound = row._scalar;
    double lowerBound = row._scalar;

    unsigned xi;
    double ci;

    for ( auto entry = row.begin(); entry != row.end(); ++entry )
    {
        unsigned i = entry->_index;

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
        ++result;
    }

    if ( FloatUtils::gt( _upperBounds[y], upperBound ) )
    {
        _upperBounds[y] = upperBound;
        _tightenedUpper[y] = true;
        ++result;
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
    for ( auto entry = row.begin(); entry != row.end(); ++entry )
    {
        unsigned i = entry->_index;

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
    for ( auto entry = row.begin(); entry != row.end(); ++entry )
    {
        unsigned i = entry->_index;

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
        ci = entry->_coefficient;
        lowerBound = lowerBound / ci;
        upperBound = upperBound / ci;

        if ( _ciSign[i] == NEGATIVE )
        {
            double temp = upperBound;
            upperBound = lowerBound;
            lowerBound = temp;
        }

        // If a tighter bound is found, store it
        xi = entry->_variable;
        if ( FloatUtils::lt( _lowerBounds[xi], lowerBound ) )
        {
            _lowerBounds[xi] = lowerBound;
            _tightenedLower[xi] = true;
            ++result;
        }

        if ( FloatUtils::gt( _upperBounds[xi], upperBound ) )
        {
            _upperBounds[xi] = upperBound;
            _tightenedUpper[xi] = true;
            ++result;
        }

        if ( FloatUtils::gt( _lowerBounds[xi], _upperBounds[xi] ) )
            throw InfeasibleQueryException();
    }

    return result;
}

void RowBoundTightener::examineConstraintMatrix( Saturation saturation )
{
    unsigned newBoundsLearned;

    /*
      If working until saturation, do single passes over the matrix until no new bounds
      are learned. Otherwise, just do a single pass.
    */
    do
    {
        newBoundsLearned = onePassOverConstraintMatrix();

        if ( _statistics && ( newBoundsLearned > 0 ) )
            _statistics->incNumTighteningsFromConstraintMatrix( newBoundsLearned );
    }
    while ( ( saturation == UNTIL_SATURATION ) && ( newBoundsLearned > 0 ) );
}

unsigned RowBoundTightener::onePassOverConstraintMatrix()
{
    unsigned result = 0;

    unsigned m = _tableau.getM();

    for ( unsigned i = 0; i < m; ++i )
        result += tightenOnSingleConstraintRow( i );

    return result;
}

unsigned RowBoundTightener::tightenOnSingleConstraintRow( unsigned row )
{
    /*
      The cosntraint matrix A satisfies Ax = b.
      Each row is of the form:

          sum ci xi - b = 0

      We first compute the lower and upper bounds for the expression

          sum ci xi - b
    */
    unsigned n = _tableau.getN();

    unsigned result = 0;

    const SparseUnsortedList *sparseRow = _tableau.getSparseARow( row );
    const double *b = _tableau.getRightHandSide();

    double ci;
    unsigned index;

    // Compute ci * lb, ci * ub, flag signs for all entries
    enum {
        ZERO = 0,
        POSITIVE = 1,
        NEGATIVE = 2,
    };

    std::fill_n( _ciSign, n, ZERO );
    std::fill_n( _ciTimesLb, n, 0 );
    std::fill_n( _ciTimesUb, n, 0 );

    for ( const auto &entry : *sparseRow )
    {
        index = entry._index;
        ci = entry._value;

        _ciSign[index] = FloatUtils::isPositive( ci ) ? POSITIVE : NEGATIVE;
        _ciTimesLb[index] = ci * _lowerBounds[index];
        _ciTimesUb[index] = ci * _upperBounds[index];
    }

    /*
      Do a pass for each of the rhs variables.
      For this, we wish to logically transform the equation into:

          xi = 1/ci * ( b - sum cj xj )

      And then compute the upper/lower bounds for xi.

      However, for efficiency, we compute the lower and upper
      bounds of the expression:

              b - sum ci xi

      Then, when we consider xi we adjust the computed lower and upper
      boudns accordingly.
    */

    double auxLb = b[row];
    double auxUb = b[row];

    // Now add ALL xi's
    for ( unsigned i = 0; i < n; ++i )
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

    double lowerBound;
    double upperBound;

    // Now consider each individual xi with non zero coefficient
    for ( const auto &entry : *sparseRow )
    {
        index = entry._index;

        lowerBound = auxLb;
        upperBound = auxUb;

        // Adjust the aux bounds to remove xi
        if ( _ciSign[index] == NEGATIVE )
        {
            lowerBound += _ciTimesLb[index];
            upperBound += _ciTimesUb[index];
        }
        else
        {
            lowerBound += _ciTimesUb[index];
            upperBound += _ciTimesLb[index];
        }

        // Now divide everything by ci, switching signs if needed.
        ci = entry._value;

        lowerBound = lowerBound / ci;
        upperBound = upperBound / ci;

        if ( _ciSign[index] == NEGATIVE )
        {
            double temp = upperBound;
            upperBound = lowerBound;
            lowerBound = temp;
        }

        // If a tighter bound is found, store it
        if ( FloatUtils::lt( _lowerBounds[index], lowerBound ) )
        {
            _lowerBounds[index] = lowerBound;
            _tightenedLower[index] = true;
            ++result;
        }

        if ( FloatUtils::gt( _upperBounds[index], upperBound ) )
        {
            _upperBounds[index] = upperBound;
            _tightenedUpper[index] = true;
            ++result;
        }

        if ( FloatUtils::gt( _lowerBounds[index], _upperBounds[index] ) )
            throw InfeasibleQueryException();
    }

    return result;
}

void RowBoundTightener::examinePivotRow()
{

	if ( _statistics )
        _statistics->incNumRowsExaminedByRowTightener();

    const SparseTableauRow &row( *_tableau.getSparsePivotRow() );
    unsigned newBoundsLearned = tightenOnSingleInvertedBasisRow( row );

    unsigned basicIndex = _tableau.getLeavingVariableIndex();

    ASSERT( basicIndex <= _m );
    // *_rows[basicIndex] = row;
    _haveStoredPivotRow[basicIndex] = true;
    _pivotRowAges[basicIndex] = _iteration;

    // *_lastPivotRows[_pivotRowIndex] = row;
    // ++_pivotRowIndex;
    // if ( _pivotRowIndex >= _m )
    //     _pivotRowIndex = 0;
    // if ( _numStoredPivotRows < _m )
    //     ++_numStoredPivotRows;

    if ( _statistics && ( newBoundsLearned > 0 ) )
        _statistics->incNumTighteningsFromRows( newBoundsLearned );

    ++_iteration;
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

void RowBoundTightener::notifyDimensionChange( unsigned /* m */ , unsigned /* n */ )
{
    setDimensions();
}

void RowBoundTightener::debug()
{
    unsigned count = 0;

    for ( unsigned i = 0; i < _m; ++i )
    {
        if ( _haveStoredPivotRow[i] )
            ++count;
    }

    printf( "\nRBW: Stored rows: %u / %u \n", count, _m );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
