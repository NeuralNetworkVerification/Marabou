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
    , _workVector( NULL )
    , _workW( NULL )
{
    _B = new double[m * m];
    if ( !_B )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED,
                                       "ForrestTomlinFactorization::B" );

    _workMatrix = new double[m * m];
    if ( !_workMatrix )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED,
                                       "ForrestTomlinFactorization::workMatrix" );

    _workVector = new double[m];
    if ( !_workVector )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED,
                                       "ForrestTomlinFactorization::workVector" );

    _workW = new double[m];
    if ( !_workW )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED,
                                       "ForrestTomlinFactorization::workW" );
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

    if ( _workVector )
    {
        delete[] _workVector;
        _workVector = NULL;
    }

    if ( _workW )
    {
        delete[] _workW;
        _workW = NULL;
    }

    List<EtaMatrix *>::iterator uIt;
    for ( uIt = _U.begin(); uIt != _U.end(); ++uIt )
        delete *uIt;
    _U.clear();

    _A.clear();

    List<LPElement *>::iterator lpIt;
    for ( lpIt = _LP.begin(); lpIt != _LP.end(); ++lpIt )
        delete *lpIt;
	_LP.clear();
}

void ForrestTomlinFactorization::pushEtaMatrix( unsigned /* columnIndex */, double */* column */ )
{
}

void ForrestTomlinFactorization::forwardTransformation( const double *y, double *x ) const
{
    /*
      The goal is to find x such that Bx = y.
      We know that the following equation holds:

           Am...A1 * LsPs...L1P1 * B = Q * Um...U1 * R

      We find x in 2 steps:

      1. Find w such that

             w = inv(Q) * Am...A1 * LsPs...L1P1 * y

      2. Find x such that

             Um....U1 * R * x = w
    */

    /****
    Step 1: Find w such that:  w = inv(Q) * Am...A1 * LsPs...L1P1 * y
    ****/

    memcpy( _workVector, y, sizeof(double) * _m );

    // Multiply y by Ps and Ls
    for ( auto lp = _LP.rbegin(); lp != _LP.rend(); ++lp )
    {
        if ( (*lp)->_pair )
        {
			double temp = _workVector[(*lp)->_pair->first];
			_workVector[(*lp)->_pair->first] = _workVector[(*lp)->_pair->second];
			_workVector[(*lp)->_pair->second] = temp;
		}
        else
        {
            unsigned col = (*lp)->_eta->_columnIndex;
            double diagonEntry = _workVector[col];
            for ( unsigned i = 0; i < _m; ++i )
            {
                if ( i == col )
                    _workVector[i] *= (*lp)->_eta->_column[col];
                else
                    _workVector[i] += diagonEntry * (*lp)->_eta->_column[i];

                if ( FloatUtils::isZero( _workVector[i] ) )
                    _workVector[i] = 0.0;
            }
        }
    }

    // Mutiply by the As
    for ( auto a = _A.rbegin(); a != _A.rend(); ++ a )
    {
        _workVector[a->_row] += ( a->_value * _workVector[a->_column] );

        if ( FloatUtils::isZero( _workVector[a->_row] ) )
            _workVector[a->_row] = 0.0;
    }

    // Multiply by inv(Q)
    PermutationMatrix *invQ = _Q.invert();
    for ( unsigned i = 0; i < _m; ++i )
        _workW[invQ->_ordering[i]] = _workVector[i];

    /****
    Step 2: Find x such that:  Um....U1 * R * x = w
    ****/

    // Eliminate the U's
    for ( const auto &u : _U )
    {
        // Handle the special row of U first
        _workVector[u->_columnIndex] = _workW[u->_columnIndex] / u->_column[u->_columnIndex];

        // Handle the remaining rows
        for ( unsigned i = 0; i < _m; ++i )
        {
            if ( i == u->_columnIndex )
                continue;

            _workVector[i] = _workW[i] - ( u->_column[i] * _workVector[u->_columnIndex] );
            if ( FloatUtils::isZero( _workVector[i] ) )
                _workVector[i] = 0.0;
        }

        memcpy( _workW, _workVector, sizeof(double) * _m );
    }

    // We are now left with Rx = w (for our modified w). Multiply by inv(R) and be done.
    PermutationMatrix *invR = _R.invert();
    for ( unsigned i = 0; i < _m; ++i )
        x[invR->_ordering[i]] = _workW[i];
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
        std::fill_n( _workVector, _m, 0 );
        double div = _workMatrix[i * _m + i];
        _workVector[i] = 1 / div;
		for ( unsigned j = i + 1; j < _m; ++j )
            _workVector[j] = -_workMatrix[i + j * _m] / div;

        // Store the resulting lower-triangular eta matrix
        EtaMatrix *L = new EtaMatrix( _m, i, _workVector );

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
        std::fill_n( _workVector, _m, 0 );
        for ( unsigned j = 0; j <= i; ++j )
            _workVector[j] = _workMatrix[j * _m + i];
        _U.appendHead( new EtaMatrix( _m, i, _workVector ) );
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

const List<AlmostDiagonalMatrix> *ForrestTomlinFactorization::getA() const
{
    return &_A;
}

void ForrestTomlinFactorization::rowSwap( unsigned rowOne, unsigned rowTwo, double *matrix )
{
    memcpy( _workVector, matrix + (rowOne * _m), sizeof(double) * _m );
    memcpy( matrix + (rowOne * _m), matrix + (rowTwo * _m), sizeof(double) * _m );
    memcpy( matrix + (rowTwo * _m), _workVector,  sizeof(double) * _m );
}

void ForrestTomlinFactorization::pushA( const AlmostDiagonalMatrix &matrix )
{
    _A.appendHead( matrix );
}

void ForrestTomlinFactorization::setQ( const PermutationMatrix &Q )
{
    _Q = Q;
}

void ForrestTomlinFactorization::setR( const PermutationMatrix &R )
{
    _R = R;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
