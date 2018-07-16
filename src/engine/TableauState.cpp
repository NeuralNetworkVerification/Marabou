/*********************                                                        */
/*! \file TableauState.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "BasisFactorizationFactory.h"
#include "CSRMatrix.h"
#include "ReluplexError.h"
#include "TableauState.h"

TableauState::TableauState()
    : _A( NULL )
    , _sparseColumnsOfA( NULL )
    , _denseA( NULL )
    , _b( NULL )
    , _lowerBounds( NULL )
    , _upperBounds( NULL )
    , _basicAssignment( NULL )
    , _nonBasicAssignment( NULL )
    , _basicIndexToVariable( NULL )
    , _nonBasicIndexToVariable( NULL )
    , _variableToIndex( NULL )
    , _basisFactorization( NULL )
{
}

TableauState::~TableauState()
{
    if ( _A )
    {
        delete _A;
        _A = NULL;
    }

    if ( _sparseColumnsOfA )
    {
        for ( unsigned i = 0; i < _n; ++i )
        {
            if ( _sparseColumnsOfA[i] )
            {
                delete _sparseColumnsOfA[i];
                _sparseColumnsOfA[i] = NULL;
            }
        }

        delete[] _sparseColumnsOfA;
        _sparseColumnsOfA = NULL;
    }

    if ( _denseA )
    {
        delete[] _denseA;
        _denseA = NULL;
    }

    if ( _b )
    {
        delete[] _b;
        _b = NULL;
    }

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

    if ( _basicAssignment )
    {
        delete[] _basicAssignment;
        _basicAssignment = NULL;
    }

    if ( _nonBasicAssignment )
    {
        delete[] _nonBasicAssignment;
        _nonBasicAssignment = NULL;
    }

    if ( _basicIndexToVariable )
    {
        delete[] _basicIndexToVariable;
        _basicIndexToVariable = NULL;
    }

    if ( _nonBasicIndexToVariable )
    {
        delete[] _nonBasicIndexToVariable;
        _nonBasicIndexToVariable = NULL;
    }

    if ( _variableToIndex )
    {
        delete[] _variableToIndex;
        _variableToIndex = NULL;
    }

    if ( _basisFactorization )
    {
        delete _basisFactorization;
        _basisFactorization = NULL;
    }
}

void TableauState::setDimensions( unsigned m, unsigned n, const IBasisFactorization::BasisColumnOracle &oracle )
{
    _m = m;
    _n = n;

    _A = new CSRMatrix();
    if ( !_A )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "TableauState::A" );

    _sparseColumnsOfA = new SparseVector *[n];
    if ( !_sparseColumnsOfA )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "TableauState::sparseColumnsOfA" );

    for ( unsigned i = 0; i < n; ++i )
    {
        _sparseColumnsOfA[i] = new SparseVector;
        if ( !_sparseColumnsOfA[i] )
            throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "TableauState::sparseColumnsOfA[i]" );
    }

    _denseA = new double[m*n];
    if ( !_denseA )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "TableauState::denseA" );

    _b = new double[m];
    if ( !_b )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "TableauState::b" );

    _lowerBounds = new double[n];
    if ( !_lowerBounds )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "TableauState::lowerBounds" );

    _upperBounds = new double[n];
    if ( !_upperBounds )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "TableauState::upperBounds" );

    _basicAssignment = new double[m];
    if ( !_basicAssignment )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "TableauState::assignment" );

    _nonBasicAssignment = new double[n-m];
    if ( !_nonBasicAssignment )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "TableauState::nonBasicAssignment" );

    _basicIndexToVariable = new unsigned[m];
    if ( !_basicIndexToVariable )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "TableauState::basicIndexToVariable" );

    _nonBasicIndexToVariable = new unsigned[n-m];
    if ( !_nonBasicIndexToVariable )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "TableauState::nonBasicIndexToVariable" );

    _variableToIndex = new unsigned[n];
    if ( !_variableToIndex )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "TableauState::variableToIndex" );

    _basisFactorization = BasisFactorizationFactory::createBasisFactorization( m, oracle );
    if ( !_basisFactorization )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "TableauState::basisFactorization" );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
