/*********************                                                        */
/*! \file RowBoundTightener.cpp
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

#include "Debug.h"
#include "FactTracker.h"
#include "InfeasibleQueryException.h"
#include "ReluplexError.h"
#include "RowBoundTightener.h"
#include "SparseUnsortedList.h"
#include "Statistics.h"

RowBoundTightener::RowBoundTightener( const ITableau &tableau )
    : _tableau( tableau )
    , _lowerBounds( NULL )
    , _lowerBoundExplanationIDs( NULL )
    , _upperBounds( NULL )
    , _upperBoundExplanationIDs( NULL )
    , _tightenedLower( NULL )
    , _tightenedUpper( NULL )
    , _lowerBoundIsInternal( NULL )
    , _upperBoundIsInternal( NULL )
    , _rows( NULL )
    , _z( NULL )
    , _ciTimesLb( NULL )
    , _ciTimesUb( NULL )
    , _ciSign( NULL )
    , _factTracker( NULL )
    , _statistics( NULL )
    , _internalFactTracker( NULL )
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

    _lowerBoundExplanationIDs = new unsigned[_n];
    if ( !_lowerBoundExplanationIDs )
      throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "RowBoundTightener::lowerBoundExplanationIDs" );

    _upperBounds = new double[_n];
    if ( !_upperBounds )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "RowBoundTightener::upperBounds" );

    _upperBoundExplanationIDs = new unsigned[_n];
    if ( !_upperBoundExplanationIDs )
      throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "RowBoundTightener::upperBoundExplanationIDs" );

    _tightenedLower = new bool[_n];
    if ( !_tightenedLower )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "RowBoundTightener::tightenedLower" );

    _tightenedUpper = new bool[_n];
    if ( !_tightenedUpper )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "RowBoundTightener::tightenedUpper" );

    _lowerBoundIsInternal = new bool[_n];
    if ( !_lowerBoundIsInternal )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "RowBoundTightener::_lowerBoundIsInternal" );

    _upperBoundIsInternal = new bool[_n];
    if ( !_upperBoundIsInternal )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "RowBoundTightener::_upperBoundIsInternal" );

    _internalFactTracker = new FactTracker();

    resetBounds();

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
        _lowerBoundExplanationIDs[i] = 0;
        _lowerBoundIsInternal[i] = false;
        
        if ( _factTracker && _factTracker->hasFactAffectingBound( i, FactTracker::LB ) )
        {
            _lowerBoundExplanationIDs[i] = _factTracker->getFactIDAffectingBound( i, FactTracker::LB );
        }

        _upperBounds[i] = _tableau.getUpperBound( i );
        _upperBoundExplanationIDs[i] = 0;
        _upperBoundIsInternal[i] = false;

        if ( _factTracker && _factTracker->hasFactAffectingBound( i, FactTracker::UB ) )
        {
            _upperBoundExplanationIDs[i] = _factTracker->getFactIDAffectingBound( i, FactTracker::UB );
        }

        //clear bounds
    }

    delete _internalFactTracker;
    _internalFactTracker = new FactTracker();
}

void RowBoundTightener::clear()
{
    std::fill( _tightenedLower, _tightenedLower + _n, false );
    std::fill( _tightenedUpper, _tightenedUpper + _n, false );

    for ( unsigned i = 0; i < _n; ++i )
    {
        _lowerBounds[i] = _tableau.getLowerBound( i );
        _lowerBoundExplanationIDs[i] = 0;
        _lowerBoundIsInternal[i] = false;

        if ( _factTracker && _factTracker->hasFactAffectingBound( i, FactTracker::LB ) )
        {
            _lowerBoundExplanationIDs[i] = _factTracker->getFactIDAffectingBound( i, FactTracker::LB );
        }

        _upperBounds[i] = _tableau.getUpperBound( i );
        _upperBoundExplanationIDs[i] = 0;
        _upperBoundIsInternal[i] = false;

        if ( _factTracker && _factTracker->hasFactAffectingBound( i, FactTracker::UB ) )
        {
            _upperBoundExplanationIDs[i] = _factTracker->getFactIDAffectingBound( i, FactTracker::UB );
        }
    }

    delete _internalFactTracker;
    _internalFactTracker = new FactTracker();
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

    if ( _lowerBoundExplanationIDs )
    {
        delete[] _lowerBoundExplanationIDs;
        _lowerBoundExplanationIDs = NULL;
    }

    if ( _upperBounds )
    {
        delete[] _upperBounds;
        _upperBounds = NULL;
    }

    if ( _upperBoundExplanationIDs )
    {
        delete[] _upperBoundExplanationIDs;
        _upperBoundExplanationIDs = NULL;
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

    if ( _internalFactTracker )
    {
        delete _internalFactTracker;
        _internalFactTracker = NULL;
    }

    if( _lowerBoundIsInternal )
    {
        delete[] _lowerBoundIsInternal;
        _lowerBoundIsInternal = NULL;
    }

    if ( _upperBoundIsInternal )
    {
        delete[] _upperBoundIsInternal;
        _upperBoundIsInternal = NULL;
    }
}

List<unsigned> RowBoundTightener::findExternalExplanations( unsigned variable, RowBoundTightener::QueryType queryType ) const
{
    Set<unsigned> explanations;
    
    if( queryType == RowBoundTightener::BOTH || queryType == RowBoundTightener::LB )
    {
        if( !_lowerBoundIsInternal[variable] )
        {
            explanations.insert( _lowerBoundExplanationIDs[variable] );
        }
        else
        {
            if( _internalFactTracker && _internalFactTracker->hasFactAffectingBound( variable, FactTracker::LB ) )
            {
                unsigned explanationID = _internalFactTracker->getFactIDAffectingBound( variable, FactTracker::LB );
                explanations.insert( _internalFactTracker->getExternalFactsForBound( explanationID ) );
            }
        }
    }

    if( queryType == RowBoundTightener::BOTH || queryType == RowBoundTightener::UB )
    {
        if( !_upperBoundIsInternal[variable] )
        {
            explanations.insert( _upperBoundExplanationIDs[variable] );
        }
        else
        {
            if( _internalFactTracker && _internalFactTracker->hasFactAffectingBound( variable, FactTracker::UB ) )
            {
                unsigned explanationID = _internalFactTracker->getFactIDAffectingBound( variable, FactTracker::UB );
                explanations.insert( _internalFactTracker->getExternalFactsForBound( explanationID ) );
            }
        }
    }

    List<unsigned> explanationList;

    for ( unsigned explanationID : explanations )
    {
        explanationList.append( explanationID );
    }

    return explanationList;
}

void RowBoundTightener::examineImplicitInvertedBasisMatrix( bool untilSaturation )
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
    for ( unsigned i = 0; i < _n - _m; ++i )
    {
        unsigned nonBasic = _tableau.nonBasicIndexToVariable( i );
        const double *ANColumn = _tableau.getAColumn( nonBasic );
        _tableau.forwardTransformation( ANColumn, _z );

        for ( unsigned j = 0; j < _m; ++j )
        {
            _rows[j]->_row[i]._var = nonBasic;
            _rows[j]->_row[i]._coefficient = -_z[j];
        }
    }

    // We now have all the rows, can use them for tightening.
    // The tightening procedure may throw an exception, in which case we need
    // to release the rows.
    unsigned newBoundsLearned;
    unsigned maxNumberOfIterations = untilSaturation ?
        GlobalConfiguration::ROW_BOUND_TIGHTENER_SATURATION_ITERATIONS : 1;
    do
    {
        newBoundsLearned = onePassOverInvertedBasisRows();

        if ( _statistics && ( newBoundsLearned > 0 ) )
            _statistics->incNumTighteningsFromExplicitBasis( newBoundsLearned );

        --maxNumberOfIterations;
    }
    while ( ( maxNumberOfIterations != 0 ) && ( newBoundsLearned > 0 ) );
}

void RowBoundTightener::examineInvertedBasisMatrix( bool untilSaturation )
{
    /*
      Roughly (the dimensions don't add up):

         xB = inv(B)*b - inv(B)*An

      We compute one row at a time.
    */

    const double *b = _tableau.getRightHandSide();
    const double *invB = _tableau.getInverseBasisMatrix();

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
                row->_row[j]._var = _tableau.nonBasicIndexToVariable( j );

                // Dot product of the i'th row of inv(B) with the appropriate
                // column of An

                const SparseUnsortedList *column = _tableau.getSparseAColumn( row->_row[j]._var );
                row->_row[j]._coefficient = 0;

                for ( const auto &entry : *column )
                    row->_row[j]._coefficient -= invB[i*_m + entry._index] * entry._value;
            }

            // Store the lhs variable
            row->_lhs = _tableau.basicIndexToVariable( i );
        }

        // We now have all the rows, can use them for tightening.
        // The tightening procedure may throw an exception, in which case we need
        // to release the rows.

        unsigned newBoundsLearned;
        unsigned maxNumberOfIterations = untilSaturation ?
            GlobalConfiguration::ROW_BOUND_TIGHTENER_SATURATION_ITERATIONS : 1;
        do
        {
            newBoundsLearned = onePassOverInvertedBasisRows();

            if ( _statistics && ( newBoundsLearned > 0 ) )
                _statistics->incNumTighteningsFromExplicitBasis( newBoundsLearned );

            --maxNumberOfIterations;
        }
        while ( ( maxNumberOfIterations != 0 ) && ( newBoundsLearned > 0 ) );
    }
    catch ( ... )
    {
        delete[] invB;
        throw;
    }

    delete[] invB;
}

unsigned RowBoundTightener::onePassOverInvertedBasisRows()
{
    unsigned newBounds = 0;

    for ( unsigned i = 0; i < _m; ++i )
        newBounds += tightenOnSingleInvertedBasisRow( *( _rows[i] ), i );

    return newBounds;
}

unsigned RowBoundTightener::tightenOnSingleInvertedBasisRow( const TableauRow &row, unsigned equIndex )
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

    // Guy: what's the motivation for using sets over lists? The removal of elements later?

    Map<unsigned, bool> yLowerBoundExplanations;
    Map<unsigned, bool> yUpperBoundExplanations;

    // Guy: this is for the fact that added the actual equation, right?
    if ( _factTracker && _factTracker->hasFactAffectingEquation( equIndex ) )
    {
        yLowerBoundExplanations.insert( _factTracker->getFactIDAffectingEquation( equIndex ), false );
        yUpperBoundExplanations.insert( _factTracker->getFactIDAffectingEquation( equIndex ), false );
    }

    for ( unsigned i = 0; i < n - m; ++i )
    {
        unsigned xi = row._row[i]._var;
        if ( _ciSign[i] == POSITIVE )
        {
            lowerBound += _ciTimesLb[i];

            // Guy: as I stated elsewhere, if I understand correctly, we are saying to the fact tracker that
            // the current bound for xi contributed to the new bound we are computing. However, maybe this is not
            // the same bound as the one we are using? I.e., th efact trackers knows a newer or an older bound?

            yLowerBoundExplanations.insert( _lowerBoundExplanationIDs[xi], _lowerBoundIsInternal[xi] );
            
            upperBound += _ciTimesUb[i];

            yUpperBoundExplanations.insert( _upperBoundExplanationIDs[xi], _upperBoundIsInternal[xi] );
        }
        else if ( _ciSign[i] == NEGATIVE )
        {
            lowerBound += _ciTimesUb[i];

            yLowerBoundExplanations.insert( _upperBoundExplanationIDs[xi], _upperBoundIsInternal[xi] );
            
            upperBound += _ciTimesLb[i];

            yUpperBoundExplanations.insert( _lowerBoundExplanationIDs[xi], _lowerBoundIsInternal[xi] );
        }
    }

    if ( FloatUtils::lt( _lowerBounds[y], lowerBound ) )
    {
        _lowerBounds[y] = lowerBound;
        _lowerBoundIsInternal[y] = true;
        Tightening newBound( y, _lowerBounds[y], Tightening::LB );
        
        for ( std::pair<unsigned, bool> explanationPair: yLowerBoundExplanations )
        {
            newBound.addExplanation( explanationPair.first, explanationPair.second );
        }

        if( _internalFactTracker )
        {
            _internalFactTracker->addBoundFact( newBound._variable, newBound );
            _lowerBoundExplanationIDs[y] = _internalFactTracker->getFactIDAffectingBound( y, FactTracker::LB );
        }

        _tightenedLower[y] = true;
        ++result;
    }
    
    if ( FloatUtils::gt( _upperBounds[y], upperBound ) )
    {
        _upperBounds[y] = upperBound;
        _upperBoundIsInternal[y] = true;
        Tightening newBound( y, _upperBounds[y], Tightening::UB );

        for ( std::pair<unsigned, bool> explanationPair: yUpperBoundExplanations )
        {
            newBound.addExplanation( explanationPair.first, explanationPair.second );
        }

        if( _internalFactTracker )
        {
            _internalFactTracker->addBoundFact( newBound._variable, newBound );
            _upperBoundExplanationIDs[y] = _internalFactTracker->getFactIDAffectingBound( y, FactTracker::UB );
        }
        _tightenedUpper[y] = true;
        ++result;
    }

    if ( FloatUtils::gt( _lowerBounds[y], _upperBounds[y] ) )
    {
        List<unsigned> failureExplanations = findExternalExplanations( y );
        throw InfeasibleQueryException( failureExplanations );
    }

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
        if ( _ciSign[i] == POSITIVE )
        {
            lowerBound += _ciTimesUb[i];
            upperBound += _ciTimesLb[i];
        }

        // Now divide everything by ci, switching signs if needed.
        ci = row[i];
        lowerBound = lowerBound / ci;
        upperBound = upperBound / ci;

        // Guy: For symmetry with auxLb and auxUb, please compute once the explanations for
        // all variables including y, and then remove as needed. Follow the same naming convention.
        Map<unsigned, bool> xiLowerBoundExplanations;
        Map<unsigned, bool> xiUpperBoundExplanations;

        if ( _ciSign[i] == POSITIVE )
        {
            xiLowerBoundExplanations = yLowerBoundExplanations;
            xiUpperBoundExplanations = yUpperBoundExplanations;
        }

        if ( _ciSign[i] == NEGATIVE )
        {
            double temp = upperBound;
            upperBound = lowerBound;
            lowerBound = temp;

            // Guy: copying lists/sets is expensive. Better do the if() first and copy just once.
            xiLowerBoundExplanations = yUpperBoundExplanations;
            xiUpperBoundExplanations = yLowerBoundExplanations;
        }

        // If a tighter bound is found, store it
        xi = row._row[i]._var;

        // Guy: erasing from lists is expensive, move this to appear inside the following if()

        if ( FloatUtils::lt( _lowerBounds[xi], lowerBound ) )
        {
            if( xiLowerBoundExplanations.exists( xi ) )
            {
                xiLowerBoundExplanations.erase( xi );
            }

            if( _ciSign[i] == POSITIVE )
            {
                xiLowerBoundExplanations.insert( _lowerBoundExplanationIDs[y], _lowerBoundIsInternal[y] );
            }
            else if( _ciSign[i] == NEGATIVE )
            {
                xiLowerBoundExplanations.insert( _upperBoundExplanationIDs[y], _upperBoundIsInternal[y] );
            }

            _lowerBounds[xi] = lowerBound;
            _lowerBoundIsInternal[xi] = true;
            Tightening newBound( xi, lowerBound, Tightening::LB );

            for ( std::pair<unsigned, bool> explanationPair: xiLowerBoundExplanations )
            {
                newBound.addExplanation( explanationPair.first, explanationPair.second );
            }

            if( _internalFactTracker )
            {
                _internalFactTracker->addBoundFact( newBound._variable, newBound );
                _lowerBoundExplanationIDs[xi] = _internalFactTracker->getFactIDAffectingBound( xi, FactTracker::LB );
            }
            
            _tightenedLower[xi] = true;
            ++result;
        }

        if ( FloatUtils::gt( _upperBounds[xi], upperBound ) )
        {
            if( xiUpperBoundExplanations.exists( xi ) )
            {
                xiUpperBoundExplanations.erase( xi );
            }
            
            if( _ciSign[i] == POSITIVE )
            {
                xiUpperBoundExplanations.insert( _upperBoundExplanationIDs[y], _upperBoundIsInternal[y] );
            }
            else if( _ciSign[i] == NEGATIVE )
            {
                xiUpperBoundExplanations.insert( _lowerBoundExplanationIDs[y], _lowerBoundIsInternal[y] );
            }

            _upperBounds[xi] = upperBound;
            _upperBoundIsInternal[xi] = true;
            Tightening newBound( xi, upperBound, Tightening::UB );

            for( std::pair<unsigned, bool> explanationPair: xiUpperBoundExplanations )
            {
                newBound.addExplanation( explanationPair.first, explanationPair.second );
            }

            if( _internalFactTracker )
            {
                _internalFactTracker->addBoundFact( newBound._variable, newBound );
                _upperBoundExplanationIDs[xi] = _internalFactTracker->getFactIDAffectingBound( xi, FactTracker::UB );
            }

            _tightenedUpper[xi] = true;
            ++result;
        }

        if ( FloatUtils::gt( _lowerBounds[xi], _upperBounds[xi] ) )
        {
            List<unsigned> failureExplanations = findExternalExplanations( xi );
            throw InfeasibleQueryException( failureExplanations );
        }
    }

    return result;
}

void RowBoundTightener::examineConstraintMatrix( bool untilSaturation )
{
    unsigned newBoundsLearned;

    /*
      If working until saturation, do single passes over the matrix until no new bounds
      are learned. Otherwise, just do a single pass.
    */
    unsigned maxNumberOfIterations = untilSaturation ?
        GlobalConfiguration::ROW_BOUND_TIGHTENER_SATURATION_ITERATIONS : 1;
    do
    {
        newBoundsLearned = onePassOverConstraintMatrix();

        if ( _statistics && ( newBoundsLearned > 0 ) )
            _statistics->incNumTighteningsFromConstraintMatrix( newBoundsLearned );

        --maxNumberOfIterations;
    }
    while ( ( maxNumberOfIterations != 0 ) && ( newBoundsLearned > 0 ) );
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
    // TODO
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

    Map<unsigned, bool> tempLowerBoundExplanations;
    Map<unsigned, bool> tempUpperBoundExplanations;

    if ( _factTracker && _factTracker->hasFactAffectingEquation( row ) )
    {
        tempLowerBoundExplanations.insert( _factTracker->getFactIDAffectingEquation( row ), true );
        tempUpperBoundExplanations.insert( _factTracker->getFactIDAffectingEquation( row ), true );
    }

    // Now add ALL xi's
    for ( unsigned i = 0; i < n; ++i )
    {
        if ( _ciSign[i] == NEGATIVE )
        {
            auxLb -= _ciTimesLb[i];
            tempLowerBoundExplanations.insert( _lowerBoundExplanationIDs[i], _lowerBoundIsInternal[i] );

            auxUb -= _ciTimesUb[i];
            tempUpperBoundExplanations.insert( _upperBoundExplanationIDs[i], _upperBoundIsInternal[i] );
        }
        else if ( _ciSign[i] == POSITIVE )
        {
            auxLb -= _ciTimesUb[i];
            tempLowerBoundExplanations.insert( _upperBoundExplanationIDs[i], _upperBoundIsInternal[i] );

            auxUb -= _ciTimesLb[i];
            tempUpperBoundExplanations.insert( _lowerBoundExplanationIDs[i], _lowerBoundIsInternal[i] );
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
        if ( _ciSign[index] == POSITIVE )
        {
            lowerBound += _ciTimesUb[index];
            upperBound += _ciTimesLb[index];
        }

        // Now divide everything by ci, switching signs if needed.
        ci = entry._value;

        lowerBound = lowerBound / ci;
        upperBound = upperBound / ci;

        Map<unsigned, bool> iLowerBoundExplanations;
        Map<unsigned, bool> iUpperBoundExplanations;
        if ( _ciSign[index] == POSITIVE )
        {
            iLowerBoundExplanations = tempLowerBoundExplanations;
            iUpperBoundExplanations = tempUpperBoundExplanations;
        }
        if ( _ciSign[index] == NEGATIVE )
        {
            double temp = upperBound;
            upperBound = lowerBound;
            lowerBound = temp;
            // Guy: Copying sets is expensive...
            iLowerBoundExplanations = tempUpperBoundExplanations;
            iUpperBoundExplanations = tempLowerBoundExplanations;
        }

        // If a tighter bound is found, store it
        if ( FloatUtils::lt( _lowerBounds[index], lowerBound ) )
        {
            if ( iLowerBoundExplanations.exists( index ) )
            {
                iLowerBoundExplanations.erase( index );
            }

            _lowerBounds[index] = lowerBound;
            _lowerBoundIsInternal[index] = true;
            Tightening newBound( index, lowerBound, Tightening::LB );

            for ( std::pair<unsigned, bool> explanationPair: iLowerBoundExplanations )
            {
                newBound.addExplanation( explanationPair.first, explanationPair.second );
            }
            
            if( _internalFactTracker )
            {
                _internalFactTracker->addBoundFact( newBound._variable, newBound );
                _lowerBoundExplanationIDs[index] = _internalFactTracker->getFactIDAffectingBound( index, FactTracker::LB );
            }

            _tightenedLower[index] = true;
            ++result;
        }

        if ( FloatUtils::gt( _upperBounds[index], upperBound ) )
        {
            if ( iUpperBoundExplanations.exists( index ) )
            {
                iUpperBoundExplanations.erase( index );
            }

            _upperBounds[index] = upperBound;
            _upperBoundIsInternal[index] = true;
            Tightening newBound( index, upperBound, Tightening::UB );

            for ( std::pair<unsigned, bool> explanationPair: iUpperBoundExplanations )
            {
                newBound.addExplanation( explanationPair.first, explanationPair.second );
            }

            if( _internalFactTracker )
            {
                _internalFactTracker->addBoundFact( newBound._variable, newBound );
                _upperBoundExplanationIDs[index] = _internalFactTracker->getFactIDAffectingBound( index, FactTracker::UB );
            }

            _tightenedUpper[index] = true;
            ++result;
        }

        if ( FloatUtils::gt( _lowerBounds[index], _upperBounds[index] ) )
        {
            List<unsigned> failureExplanations =  findExternalExplanations( index );
            throw InfeasibleQueryException( failureExplanations );
        }
    }

    return result;
}

void RowBoundTightener::examinePivotRow()
{
	if ( _statistics )
        _statistics->incNumRowsExaminedByRowTightener();

    const TableauRow &row( *_tableau.getPivotRow() );
    unsigned newBoundsLearned = tightenOnSingleInvertedBasisRow( row, _tableau.getLeavingVariableIndex() );

    if ( _statistics && ( newBoundsLearned > 0 ) )
        _statistics->incNumTighteningsFromRows( newBoundsLearned );
}

void RowBoundTightener::getRowTightenings( List<Tightening> &tightenings ) const
{
    for ( unsigned i = 0; i < _n; ++i )
    {
        if ( _tightenedLower[i] )
        {
            Tightening newBound( i, _lowerBounds[i], Tightening::LB );
            List<unsigned> externalExplanations = findExternalExplanations( i, RowBoundTightener::LB );

            for ( unsigned explanationID: externalExplanations )
            {
                newBound.addExplanation( explanationID, false );
            }

            tightenings.append( newBound );
            _tightenedLower[i] = false;
        }

        if ( _tightenedUpper[i] )
        {
            Tightening newBound( i, _upperBounds[i], Tightening::UB );
            List<unsigned> externalExplanations = findExternalExplanations( i, RowBoundTightener::UB );

            for ( unsigned explanationID: externalExplanations )
            {
                newBound.addExplanation( explanationID, false );
            }

            tightenings.append( newBound );
            _tightenedUpper[i] = false;
        }
    }
}

void RowBoundTightener::setFactTracker( FactTracker *factTracker )
{
    _factTracker = factTracker;
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
        _lowerBoundIsInternal[variable] = false;

        if ( _factTracker && _factTracker->hasFactAffectingBound( variable, FactTracker::LB ) )
        {
            _lowerBoundExplanationIDs[variable] = _factTracker->getFactIDAffectingBound( variable, FactTracker::LB );
        }
        
        _tightenedLower[variable] = false;
    }
}

void RowBoundTightener::notifyUpperBound( unsigned variable, double bound )
{
    if ( FloatUtils::lt( bound, _upperBounds[variable] ) )
    {
        _upperBounds[variable] = bound;
        _upperBoundIsInternal[variable] = false;

        if ( _factTracker && _factTracker->hasFactAffectingBound( variable, FactTracker::UB ) )
        {
            _upperBoundExplanationIDs[variable] = _factTracker->getFactIDAffectingBound( variable, FactTracker::UB );
        }

        _tightenedUpper[variable] = false;
    }
}

void RowBoundTightener::notifyDimensionChange( unsigned /* m */ , unsigned /* n */ )
{
    setDimensions();
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
