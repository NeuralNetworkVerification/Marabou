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
    , _A( NULL )
    , _Q( m )
    , _invQ( m )
    , _U( NULL )
    , _workMatrix( NULL )
    , _workVector( NULL )
    , _workW( NULL )
    , _workQ( m )
    , _invWorkQ( m )
    , _storedW( NULL )
{
    _B = new double[m * m];
    if ( !_B )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED,
                                       "ForrestTomlinFactorization::B" );

    _A = new AlmostIdentityMatrix[m];
    if ( !_A )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED,
                                       "ForrestTomlinFactorization::A" );

    _U = new EtaMatrix *[m];
    if ( !_U )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED,
                                       "ForrestTomlinFactorization::U" );

    for ( unsigned i = 0; i < _m; ++i )
    {
        _U[i] = new EtaMatrix( _m, i );
        if ( !_U[i] )
            throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED,
                                           "ForrestTomlinFactorization::U[i]" );
    }

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
    _storedW = new double[m];
    if ( !_storedW )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED,
                                       "ForrestTomlinFactorization::storedW" );
}

ForrestTomlinFactorization::~ForrestTomlinFactorization()
{
	if ( _B )
	{
		delete[] _B;
		_B = NULL;
	}

	if ( _A )
	{
		delete[] _A;
		_A = NULL;
	}

	if ( _U )
	{
        for ( unsigned i = 0; i < _m; ++i )
        {
            if ( _U[i] )
            {
                delete _U[i];
                _U[i] = NULL;
            }
        }

		delete[] _U;
		_U = NULL;
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

    if ( _storedW )
    {
        delete[] _storedW;
        _storedW = NULL;
    }

    List<LPElement *>::iterator lpIt;
    for ( lpIt = _LP.begin(); lpIt != _LP.end(); ++lpIt )
        delete *lpIt;
	_LP.clear();
}

void ForrestTomlinFactorization::pushEtaMatrix( unsigned columnIndex, const double */* column */ )
{
    /*
      The first step is to compute V = U * invQ * E * Q. V differs from U in just
      one column. This column should have already been found during the previous
      forward transformation, in which case it is stored as _storedW.
      The index of this changed column, depends on Q.
    */

    // Extract the index of the changed U column, and update it to _storedW.
    unsigned indexOfChangedUColumn = _Q.findIndexOfRow( columnIndex );
    memcpy( _U[indexOfChangedUColumn - 1]->_column, _storedW, sizeof(double) * _m );

    /*
      V is upper triangular except for one column. We pick workQ
      so that workQ * V * invWorkQ is upper triangular except for the last row.
    */
    _workQ.resetToIdentity();
    for ( unsigned i = indexOfChangedUColumn; i < _m - 1; ++i )
        _workQ._ordering[i] = i + 1;
    _workQ._ordering[_m - 1] = indexOfChangedUColumn;

    _workQ.invert( _invWorkQ );

    /*
      Now we have workQ * V * invWorkQ that is upper triangular except the last row.
      We construct a new sequence of AlmostIdentityMatrices that make it upper
      triangular.
    */

    // Start by resetting the old matrices
    for ( unsigned i = 0; i < _m; ++i )
        _A[i]._identity = true;

    // Now, go adjust these matrices according to the last row
    // of workQ * V * workR, which is of course not stored explicitly.
    for ( unsigned i = indexOfChangedUColumn; i < _m; ++i )
    {
        // Initialize to the non-modified bump value, according to the
        // permutations. This is cell (m,i).
        unsigned originalRow = _workQ._ordering[_m - 1];
        unsigned originalCol = _workQ._ordering[i];

        double bumpValue = _U[originalCol]->_column[originalRow];

        for ( unsigned j = indexOfChangedUColumn; j < i; ++i )
        {
            if ( !_A[j]._identity )
                bumpValue += _A[j]._value * _U[_workQ._ordering[_A[j]._column]]->_column[_workQ._ordering[_A[j]._column]];
        }

        // If the bump value is zero, nothing needs to be done
        if ( FloatUtils::isZero( bumpValue ) )
            continue;

        // Now we want to zero out the bump value using an A matrix
        double diagonalValue = _U[_workQ._ordering[i]]->_column[_workQ._ordering[i]];

        _A[i]._row = _m - 1;
        _A[i]._column = i;
        _A[i]._value = bumpValue / diagonalValue;
        _A[i]._identity = false;
    }

    /*
      At this point we have that Am...A1 * workQ * V * invWorkQ is upper triangular.
      We now do the textbook update to the store elements.
    */

    // Update _Q to _Q * _invWorkQ. All permutation matrices are stored
    // row-wise, so this means permuting invQ's rows.

    unsigned *ordering = new unsigned[_m];
    for ( unsigned i = 0; i < _m; ++i )
        ordering[i] = _invWorkQ._ordering[_Q._ordering[i]];
    for ( unsigned i = 0; i < _m; ++i )
        _Q._ordering[i] = ordering[i];
    delete[] ordering;

    // Recompute invQ
    _Q.invert( _invQ );

    // Ai = _Q * Ai * _invQ, for all i
    for ( unsigned i = 0; i < _m; ++i )
    {
        if ( !_A[i]._identity )
        {
            _A[i]._row = _Q._ordering[_A[i]._row];
            _A[i]._column = _invQ._ordering[_A[i]._column];
        }
    }
}

void ForrestTomlinFactorization::forwardTransformation( const double *y, double *x ) const
{
    /*
      The goal is to find x such that Bx = y.
      We know that the following equation holds:

           Am...A1 * LsPs...L1P1 * B = Q * Um...U1 * invQ

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
    for ( unsigned i = 0; i < _m; ++i )
    {
        if ( _A[i]._identity )
            continue;

        _workVector[_A[i]._row] += ( _A[i]._value * _workVector[_A[i]._column] );

        if ( FloatUtils::isZero( _workVector[_A[i]._row] ) )
            _workVector[_A[i]._row] = 0.0;
    }

    // Multiply by inv(Q)
    for ( unsigned i = 0; i < _m; ++i )
        _workW[_invQ._ordering[i]] = _workVector[i];

    /****
    Intermediate step: store w for later use
    ****/
    memcpy( _storedW, _workW, sizeof(double) * _m );

    /****
    Step 2: Find x such that:  Um....U1 * invQ * x = w
    ****/

    // Eliminate the U's
    for ( int i = _m - 1; i >= 0; --i )
    {
        // Handle the special row of U first
        _workVector[_U[i]->_columnIndex] =
            _workW[_U[i]->_columnIndex] / _U[i]->_column[_U[i]->_columnIndex];

        // Handle the remaining rows
        for ( unsigned j = 0; j < _m; ++j )
        {
            if ( j == _U[i]->_columnIndex )
                continue;

            _workVector[j] = _workW[j] - ( _U[i]->_column[j] * _workVector[_U[i]->_columnIndex] );
            if ( FloatUtils::isZero( _workVector[j] ) )
                _workVector[j] = 0.0;
        }

        memcpy( _workW, _workVector, sizeof(double) * _m );
    }


    // We are now left with invQ x = w (for our modified w). Multiply by Q and be done.
    for ( unsigned i = 0; i < _m; ++i )
        x[_Q._ordering[i]] = _workW[i];
}

void ForrestTomlinFactorization::backwardTransformation( const double *y, double *x ) const
{
    /*
      The goal is to find x such that xB = y.
      We know that the following equation holds:

      Am...A1 * LsPs...L1P1 * B = Q * Um...U1 * invQ

      We find x in 2 steps:

      1. Find v such that

      v * Um...U1 = y * Q

      2. Find x such that

      x = v * invQ * Am...A1 * LsPs...L1P1
    */

    unsigned columnIndex;

    /****
         Step 1: Find v such that:  v * Um...U1 = y * Q
    ****/

    // Multiply y by Q, store in _workVector
    for ( unsigned i = 0; i < _m; ++i )
        _workVector[_Q._ordering[i]] = y[i];

    // Eliminate the U's
    for ( unsigned i = 0; i < _m; ++i )
    {
        columnIndex = _U[i]->_columnIndex;

        // Only one entry of _workVector is changed by the undoing of each u
        for ( unsigned j = 0; j < _m; ++j )
        {
            if ( j == columnIndex )
                continue;

            _workVector[columnIndex] -= ( _U[i]->_column[j] * _workVector[j] );
        }

        _workVector[columnIndex] /= _U[i]->_column[columnIndex];

        if ( FloatUtils::isZero( _workVector[columnIndex] ) )
            _workVector[columnIndex] = 0.0;
    }

    /****
    Step 2: x = v * invQ * Am...A1 * LsPs...L1P1
    ****/

    // Multiply by invQ
    for ( unsigned i = 0; i < _m; ++i )
        x[_invQ._ordering[i]] = _workVector[i];

    // Mutiply by the As
    for ( int i = _m - 1; i >= 0; --i )
    {
        if ( _A[i]._identity )
            continue;

        columnIndex = _A[i]._column;
        x[columnIndex] += ( _A[i]._value * x[_A[i]._row] );

        if ( FloatUtils::isZero( x[columnIndex] ) )
            x[columnIndex] = 0.0;
    }

    // Multiply y by Ps and Ls
    for ( const auto &lp : _LP )
    {
        if ( lp->_pair )
        {
			double temp = x[lp->_pair->first];
			x[lp->_pair->first] = x[lp->_pair->second];
			x[lp->_pair->second] = temp;
		}
        else
        {
            columnIndex = lp->_eta->_columnIndex;

            x[columnIndex] *= lp->_eta->_column[columnIndex];
            for ( unsigned i = 0; i < _m; ++i )
            {
                if ( i == columnIndex )
                    continue;

                x[columnIndex] += ( lp->_eta->_column[i] * x[i] );
            }

            if ( FloatUtils::isZero( x[columnIndex] ) )
                x[columnIndex] = 0.0;
        }
    }
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
    List<LPElement *>::iterator lpIt;
    for ( lpIt = _LP.begin(); lpIt != _LP.end(); ++lpIt )
        delete *lpIt;
	_LP.clear();

    _Q.resetToIdentity();
    _invQ.resetToIdentity();

    for ( unsigned i = 0; i < _m; ++i )
    {
        _A[i]._identity = true;
        _U[i]->resetToIdentity();
    }
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
        EtaMatrix *u = _U[i];
        // Copy the i'th column (column 0 is an identity column, can skip)
        // U is always upper triangular, so don't need to clear below
        // the diagonal.

        for ( unsigned j = 0; j <= i; ++j )
            u->_column[j] = _workMatrix[j * _m + i];
    }
}

bool ForrestTomlinFactorization::explicitBasisAvailable() const
{
    return false;
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

const PermutationMatrix *ForrestTomlinFactorization::getInvQ() const
{
    return &_invQ;
}

const EtaMatrix **ForrestTomlinFactorization::getU() const
{
    return (const EtaMatrix **)_U;
}

const List<LPElement *> *ForrestTomlinFactorization::getLP() const
{
    return &_LP;
}

const AlmostIdentityMatrix *ForrestTomlinFactorization::getA() const
{
    return _A;
}

void ForrestTomlinFactorization::rowSwap( unsigned rowOne, unsigned rowTwo, double *matrix )
{
    memcpy( _workVector, matrix + (rowOne * _m), sizeof(double) * _m );
    memcpy( matrix + (rowOne * _m), matrix + (rowTwo * _m), sizeof(double) * _m );
    memcpy( matrix + (rowTwo * _m), _workVector,  sizeof(double) * _m );
}

void ForrestTomlinFactorization::setA( unsigned index, const AlmostIdentityMatrix &matrix )
{
    _A[index] = matrix;
}

void ForrestTomlinFactorization::setQ( const PermutationMatrix &Q )
{
    _Q = Q;
    _Q.invert( _invQ );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
