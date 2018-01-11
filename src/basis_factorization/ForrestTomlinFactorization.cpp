/*********************                                                        */
/*! \file ForrestTomlinFactorization.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "BasisFactorizationError.h"
#include "EtaMatrix.h"
#include "FloatUtils.h"
#include "ForrestTomlinFactorization.h"
#include "MalformedBasisException.h"
#include <cstdlib>
#include <cstring>

ForrestTomlinFactorization::ForrestTomlinFactorization( unsigned m )
    : _m( m )
    , _B( NULL )
    , _Q( m )
    , _R( m )
    , _workMatrix( NULL )
    , _workColumn( NULL )
{
    _B = new double[m * m];
    if ( !_B )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED,
                                       "ForrestTomlinFactorization::B" );

    _workMatrix = new double[m * m];
    if ( !_workMatrix )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED,
                                       "ForrestTomlinFactorization::workMatrix" );

    _workColumn = new double[m];
    if ( !_workColumn )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED,
                                       "ForrestTomlinFactorization::workColumn" );
}

ForrestTomlinFactorization::~ForrestTomlinFactorization()
{
	if ( _B )
	{
		delete[] _B;
		_B = NULL;
	}

	if ( _workMatrix )
	{
		delete[] _workMatrix;
		_workMatrix = NULL;
	}

    if ( _workColumn )
    {
        delete[] _workColumn;
        _workColumn = NULL;
    }

    List<EtaMatrix *>::iterator uIt;
    for ( uIt = _U.begin(); uIt != _U.end(); ++uIt )
        delete *uIt;
    _U.clear();

    List<AlmostDiagonalMatrix *>::iterator aIt;
    for ( aIt = _A.begin(); aIt != _A.end(); ++aIt )
        delete *aIt;
    _A.clear();

    List<LPElement *>::iterator lpIt;
    for ( lpIt = _LP.begin(); lpIt != _LP.end(); ++lpIt )
        delete *lpIt;
	_LP.clear();
}

void ForrestTomlinFactorization::pushEtaMatrix( unsigned /* columnIndex */, double */* column */ )
{
}

void ForrestTomlinFactorization::forwardTransformation( const double */* y */, double */* x */ ) const
{
}

void ForrestTomlinFactorization::backwardTransformation( const double */* y */, double */* x */ ) const
{
}

void ForrestTomlinFactorization::storeFactorization( IBasisFactorization */* other */ )
{
}

void ForrestTomlinFactorization::restoreFactorization( const IBasisFactorization */* other */ )
{
}

void ForrestTomlinFactorization::setBasis( const double *B )
{
    memcpy( _B, B, sizeof(double) * _m * _m );
	initialLUFactorization();
}

void ForrestTomlinFactorization::clearFactorization()
{
    List<EtaMatrix *>::iterator uIt;
    for ( uIt = _U.begin(); uIt != _U.end(); ++uIt )
        delete *uIt;
    _U.clear();

    List<AlmostDiagonalMatrix *>::iterator aIt;
    for ( aIt = _A.begin(); aIt != _A.end(); ++aIt )
        delete *aIt;
    _A.clear();

    List<LPElement *>::iterator lpIt;
    for ( lpIt = _LP.begin(); lpIt != _LP.end(); ++lpIt )
        delete *lpIt;
	_LP.clear();

    _Q.resetToIdentity();
    _R.resetToIdentity();
}

void ForrestTomlinFactorization::initialLUFactorization()
{
    // First clear any existing factorization
    clearFactorization();

    // Copy the new matrix to the work memory
	memcpy( _workMatrix, _B, sizeof(double) * _m * _m );

    // Perform Gaussian eliminiation to obtain the LP matrices
	for ( unsigned i = 0; i < _m; ++i )
    {
        // Begin work for the i'th column.
        // Employ partial pivoting: find the row with the largest element and swap it to position i
        // (See discussion at http://www2.lawrence.edu/fast/GREGGJ/Math420/Section_6_2.pdf)
        double largestElement = FloatUtils::abs( _workMatrix[i * _m + i] );
        unsigned bestRowIndex = i;

        for ( unsigned j = i + 1; j < _m; ++j )
        {
            double contender = FloatUtils::abs( _workMatrix[j * _m + i] );
            if ( FloatUtils::gt( contender, largestElement ) )
            {
                largestElement = contender;
                bestRowIndex = j;
            }
        }

        // No non-zero pivot has been found, matrix cannot be factorized
        if ( FloatUtils::isZero( largestElement ) )
            throw MalformedBasisException();

        // Swap rows i and bestRow (if needed), and store this permutation
        if ( bestRowIndex != i )
        {
            rowSwap( i, bestRowIndex, _workMatrix );
            std::pair<unsigned, unsigned> *P = new std::pair<unsigned, unsigned>( i, bestRowIndex );
            _LP.appendHead( new LPElement( NULL, P ) );
        }

        // The matrix now has a non-zero value at entry (i,i), so we can perform
        // Gaussian elimination for the subsequent rows
        std::fill_n( _workColumn, _m, 0 );
        double div = _workMatrix[i * _m + i];
        _workColumn[i] = 1 / div;
		for ( unsigned j = i + 1; j < _m; ++j )
            _workColumn[j] = -_workMatrix[i + j * _m] / div;

        // Store the resulting lower-triangular eta matrix
        EtaMatrix *L = new EtaMatrix( _m, i, _workColumn );

        _LP.appendHead( new LPElement( L, NULL ) );

        // Perform the actual elimination step on the matrix
        // First, perform in-place multiplication for all rows below the pivot row
        for ( unsigned row = i + 1; row < _m; ++row )
        {
            _workMatrix[row * _m + i] = 0.0;
            for ( unsigned col = i + 1; col < _m; ++col )
                _workMatrix[row * _m + col] += L->_column[row] * _workMatrix[i * _m + col];
        }

        // Finally, perform the multiplication for the pivot row
        // itself. We change this row last because it is required for all
        // previous multiplication operations.
        for ( unsigned j = i + 1; j < _m; ++j )
            _workMatrix[i * _m + j] *= L->_column[i];

        _workMatrix[i * _m + i] = 1.0;
	}

    // Extract the U matrices
    for ( unsigned i = 1; i < _m; ++i )
    {
        // Copy the i'th column (column 0 is an identity column, can skip)
        std::fill_n( _workColumn, _m, 0 );
        for ( unsigned j = 0; j <= i; ++j )
            _workColumn[j] = _workMatrix[j * _m + i];
        _U.appendHead( new EtaMatrix( _m, i, _workColumn ) );
    }
}

bool ForrestTomlinFactorization::explicitBasisAvailable() const
{
    return true;
}

void ForrestTomlinFactorization::makeExplicitBasisAvailable()
{
}

const double *ForrestTomlinFactorization::getBasis() const
{
    return NULL;
}

void ForrestTomlinFactorization::invertBasis( double */* result */ )
{
}

const PermutationMatrix *ForrestTomlinFactorization::getQ() const
{
    return &_Q;
}

const PermutationMatrix *ForrestTomlinFactorization::getR() const
{
    return &_R;
}

const List<EtaMatrix *> *ForrestTomlinFactorization::getU() const
{
    return &_U;
}

const List<LPElement *> *ForrestTomlinFactorization::getLP() const
{
    return &_LP;
}

const List<AlmostDiagonalMatrix *> *ForrestTomlinFactorization::getA() const
{
    return &_A;
}

void ForrestTomlinFactorization::rowSwap( unsigned rowOne, unsigned rowTwo, double *matrix )
{
    memcpy( _workColumn, matrix + (rowOne * _m), sizeof(double) * _m );
    memcpy( matrix + (rowOne * _m), matrix + (rowTwo * _m), sizeof(double) * _m );
    memcpy( matrix + (rowTwo * _m), _workColumn,  sizeof(double) * _m );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
