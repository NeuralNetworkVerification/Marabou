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
    , _invQ( m )
    , _U( NULL )
    , _explicitBasisAvailable( false )
    , _workMatrix( NULL )
    , _workVector( NULL )
    , _workW( NULL )
    , _workQ( m )
    , _invWorkQ( m )
    , _workOrdering( NULL )
{
    _B = new double[m * m];
    if ( !_B )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED,
                                       "ForrestTomlinFactorization::B" );

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

    _workOrdering = new unsigned[m];
    if ( !_workOrdering )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED,
                                       "ForrestTomlinFactorization::workOrdering" );
}

ForrestTomlinFactorization::~ForrestTomlinFactorization()
{
	if ( _B )
	{
		delete[] _B;
		_B = NULL;
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

    if ( _workOrdering )
    {
        delete[] _workOrdering;
        _workOrdering = NULL;
    }

    List<LPElement *>::iterator lpIt;
    for ( lpIt = _LP.begin(); lpIt != _LP.end(); ++lpIt )
        delete *lpIt;
    _LP.clear();

    List<AlmostIdentityMatrix *>::iterator aIt;
    for ( aIt = _A.begin(); aIt != _A.end(); ++aIt )
        delete *aIt;
    _A.clear();
}

void ForrestTomlinFactorization::pushEtaMatrix( unsigned columnIndex, const double *column )
{
    // Pushing an eta matrix invalidates the explicit basis
    _explicitBasisAvailable = false;

    /*
      Let V = U * invQ * E * Q.
      V differs from U in just one column. The index of this column
      determines the new permutation matrix workQ
    */
    unsigned indexOfChangedUColumn = _invQ.findIndexOfRow( columnIndex );

    _workQ.resetToIdentity();
    for ( unsigned i = indexOfChangedUColumn; i < _m - 1; ++i )
        _workQ._ordering[i] = i + 1;
    _workQ._ordering[_m - 1] = indexOfChangedUColumn;
    _workQ.invert( _invWorkQ );

    /*
      The next step is to compute the A matrices. For this, we need V,
      which is obtained by replacing the column of U with
      w = Um...U1 * invQ * d, where d is the change column.

      We use the U array to temporarily store V for the computation, although
      it is not upper triangular.
    */
    for ( unsigned i = 0; i < _m; ++i )
        _workVector[i] = column[_invQ._ordering[i]];

    for ( unsigned i = 0; i < _m; ++i )
    {
        // Multiply _workVector by Ui
        ASSERT( _U[i]->_column[_U[i]->_columnIndex] == 1 );
        for ( unsigned j = 0; j < _U[i]->_columnIndex; ++j )
            _workVector[j] += ( _workVector[_U[i]->_columnIndex] * _U[i]->_column[j] );
    }

    memcpy( _U[indexOfChangedUColumn]->_column, _workVector, sizeof(double) * _m );

    /*
      Now we have workQ * V * invWorkQ that is upper triangular except the last row.
      We construct a new sequence of AlmostIdentityMatrices that make it upper
      triangular.
    */

    List<AlmostIdentityMatrix *> newAs;

    // Now, go adjust these matrices according to the last row
    // of workQ * V * workR, which is of course not stored explicitly.
    for ( unsigned i = indexOfChangedUColumn; i < _m; ++i )
    {
        // Initialize to the non-modified bump value, according to the
        // permutations. This is cell (m,i).
        unsigned originalRow = _workQ._ordering[_m - 1];
        unsigned originalCol = _workQ._ordering[i];
        double bumpValue = _U[originalCol]->_column[originalRow];

        for ( const auto &a : newAs )
        {
            /*
              We adjust for each previous column, j.
              For each such column we need to add A[j]'s value, times
              entry (_A[j]._column, i) of V.
              To find this entry we go through the permutations.
            */

            bumpValue += a->_value *
                _U[_workQ._ordering[i]]->_column[_workQ._ordering[a->_column]];
        }

        // If the bump value is zero, nothing needs to be done
        if ( FloatUtils::isZero( bumpValue ) && ( i < _m - 1 ) )
            continue;

        // If the bump value is 1 and this is the diagonal entry
        // already, nothing needs to be done
        if ( FloatUtils::areEqual( bumpValue, 1 ) && ( i == _m - 1  ) )
            continue;

        // Now we want to zero out the bump value using an A matrix.
        // We need entry (i,i) of V, and again we got through the permutations.
        AlmostIdentityMatrix *newA = new AlmostIdentityMatrix;
        newA->_row = _m - 1;
        newA->_column = i;

        if ( i < _m - 1 )
        {
            double diagonalValue = _U[_workQ._ordering[i]]->_column[_workQ._ordering[i]];
            ASSERT( !FloatUtils::isZero( diagonalValue ) );
            newA->_value = - bumpValue / diagonalValue;
        }
        else
        {
            ASSERT( !FloatUtils::isZero( bumpValue ) );
            newA->_value = 1 / bumpValue;
        }

        newAs.append( newA );
    }

    /*
      At this point we have that Ak...A1 * workQ * V * invWorkQ is upper triangular.
      We now do the textbook update to the stored elements.
    */

    // Update the Us to their new upper triangular form, according to the
    // permutation matrices.

    // Column permutations: the column that was changed becomes the last column.
    EtaMatrix *temp = _U[indexOfChangedUColumn];
    for ( unsigned i = indexOfChangedUColumn; i < _m - 1; ++i )
    {
        _U[i] = _U[i + 1];
        _U[i]->_columnIndex = i;
    }

    _U[_m - 1] = temp;
    _U[_m - 1]->_columnIndex = _m - 1;

    // Row permutations: the row with the change index is erased, other rows move up,
    // and we add the identity row in the end
    for ( unsigned i = indexOfChangedUColumn; i < _m - 1; ++i )
    {
        for ( unsigned j = indexOfChangedUColumn; j < i + 1; ++j )
            _U[i]->_column[j] = _U[i]->_column[j + 1];

        _U[i]->_column[i + 1] = 0;
    }

    for ( unsigned j = indexOfChangedUColumn; j < _m - 1; ++j )
        _U[_m - 1]->_column[j] = _U[_m - 1]->_column[j + 1];
    _U[_m - 1]->_column[_m - 1] = 1;

    // Update _Q to _Q * _invWorkQ. All permutation matrices are stored
    // row-wise, so this means permuting invQ's rows.

    for ( unsigned i = 0; i < _m; ++i )
        _workOrdering[i] = _invWorkQ._ordering[_Q._ordering[i]];
    for ( unsigned i = 0; i < _m; ++i )
        _Q._ordering[i] = _workOrdering[i];

    // Recompute invQ
    _Q.invert( _invQ );

    // Ai = _Q * Ai * _invQ, for all new A matrices
    for ( const auto &a : newAs )
    {
        a->_row = _invQ._ordering[a->_row];
        a->_column = _invQ._ordering[a->_column];
    }

    // Finally, append the new As to the list
    _A.append( newAs );

    // If the number of A matrices is too great, condense them.
    if ( _A.size() > GlobalConfiguration::REFACTORIZATION_THRESHOLD )
        makeExplicitBasisAvailable();
}

void ForrestTomlinFactorization::forwardTransformation( const double *y, double *x ) const
{
    /*
      The goal is to find x such that Bx = y.
      We know that the following equation holds:

           Ak...A1 * LsPs...L1P1 * B = Q * Um...U1 * invQ

      We find x in 2 steps:

      1. Find w such that

             w = inv(Q) * Ak...A1 * LsPs...L1P1 * y

      2. Find x such that

             Um....U1 * invQ * x = w
    */

    /****
    Step 1: Find w such that:  w = inv(Q) * Ak...A1 * LsPs...L1P1 * y
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
            double diagonalEntry = _workVector[col];

            _workVector[col] *= (*lp)->_eta->_column[col];
            for ( unsigned i = col + 1; i < _m; ++i )
            {
                _workVector[i] += diagonalEntry * (*lp)->_eta->_column[i];

                if ( FloatUtils::isZero( _workVector[i] ) )
                    _workVector[i] = 0.0;
            }
        }
    }

    // Mutiply by the As
    for ( const auto &a : _A )
    {
        if ( a->_column == a->_row )
        {
            _workVector[a->_column] *= a->_value;
        }
        else
        {
            _workVector[a->_row] += ( a->_value * _workVector[a->_column] );
            if ( FloatUtils::isZero( _workVector[a->_row] ) )
                _workVector[a->_row] = 0.0;
        }
    }

    // Multiply by inv(Q)
    for ( unsigned i = 0; i < _m; ++i )
        _workW[i] = _workVector[_invQ._ordering[i]];

    /****
    Step 2: Find x such that: Um....U1 * invQ * x = w
    ****/

    // Eliminate the U's
    for ( int i = _m - 1; i >= 0; --i )
    {
        double diagonalEntry = _workW[_U[i]->_columnIndex];
        for ( unsigned j = 0; j < _U[i]->_columnIndex; ++j )
        {
            _workW[j] -= _U[i]->_column[j] * diagonalEntry;

            if ( FloatUtils::isZero( _workW[j] ) )
                _workW[j] = 0.0;
        }
    }

    // We are now left with invQ x = w (for our modified w). Multiply by Q and be done.
    for ( unsigned i = 0; i < _m; ++i )
        x[i] = _workW[_Q._ordering[i]];
}

void ForrestTomlinFactorization::backwardTransformation( const double *y, double *x ) const
{
    /*
      The goal is to find x such that xB = y.
      We know that the following equation holds:

      Ak...A1 * LsPs...L1P1 * B = Q * Um...U1 * invQ

      We find x in 2 steps:

      1. Find v such that

      v * Um...U1 = y * Q

      2. Find x such that

      x = v * invQ * Ak...A1 * LsPs...L1P1
    */

    unsigned columnIndex;

    /****
         Step 1: Find v such that:  v * Um...U1 = y * Q
    ****/

    // Multiply y by Q, store in _workVector.
    // Note: this is easier to do with a column-wise representation of Q,
    // which is just the row-wise representation of invQ.
    for ( unsigned i = 0; i < _m; ++i )
        _workVector[i] = y[_invQ._ordering[i]];

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

        ASSERT( FloatUtils::areEqual( _U[i]->_column[columnIndex], 1.0 ) );

        if ( FloatUtils::isZero( _workVector[columnIndex] ) )
            _workVector[columnIndex] = 0.0;
    }

    /****
    Step 2: x = v * invQ * Ak...A1 * LsPs...L1P1
    ****/

    // Multiply by invQ
    // Note: this is easier to do with a column-wise representation of invQ,
    // which is just the row-wise representation of Q.
    for ( unsigned i = 0; i < _m; ++i )
        x[i] = _workVector[_Q._ordering[i]];

    // Mutiply by the As
    for ( auto a = _A.rbegin(); a != _A.rend(); ++a )
    {
        columnIndex = (*a)->_column;
        if ( columnIndex == (*a)->_row )
        {
            x[columnIndex] *= (*a)->_value;
        }
        else
        {
            x[columnIndex] += ( (*a)->_value * x[(*a)->_row] );
            if ( FloatUtils::isZero( x[columnIndex] ) )
                x[columnIndex] = 0.0;
        }
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

void ForrestTomlinFactorization::storeFactorization( IBasisFactorization *other )
{
    ForrestTomlinFactorization *otherFTFactorization = (ForrestTomlinFactorization *)other;

    ASSERT( _m == otherFTFactorization->_m );

    // Copy the non-pointer elements
    otherFTFactorization->_Q = _Q;
    otherFTFactorization->_invQ = _invQ;
    otherFTFactorization->_explicitBasisAvailable = _explicitBasisAvailable;

    // Copy the basis matrix and its factorization
    memcpy( otherFTFactorization->_B, _B, sizeof(double) * _m * _m );

    for ( unsigned i = 0; i < _m; ++i )
        *(otherFTFactorization->_U[i]) = *_U[i];

    List<LPElement *>::iterator lpIt;
    for ( lpIt = otherFTFactorization->_LP.begin(); lpIt != otherFTFactorization->_LP.end(); ++lpIt )
        delete *lpIt;
    otherFTFactorization->_LP.clear();

    for ( const auto &lp : _LP )
        otherFTFactorization->_LP.append( lp->duplicate() );

    List<AlmostIdentityMatrix *>::iterator aIt;
    for ( aIt = otherFTFactorization->_A.begin(); aIt != otherFTFactorization->_A.end(); ++aIt )
        delete *aIt;
    otherFTFactorization->_A.clear();

    for ( const auto &a : _A )
        otherFTFactorization->_A.append( new AlmostIdentityMatrix( *a ) );

    // We assume that this function is not invoked in the middle of, e.g., a pivot operation,
    // so we don't store the temporary data structures.
}

void ForrestTomlinFactorization::restoreFactorization( const IBasisFactorization *other )
{
    ForrestTomlinFactorization *otherFTFactorization = (ForrestTomlinFactorization *)other;

    ASSERT( _m == otherFTFactorization->_m );

    // Copy the non-pointer elements
    _Q = otherFTFactorization->_Q;
    _invQ = otherFTFactorization->_invQ;
    _explicitBasisAvailable = otherFTFactorization->_explicitBasisAvailable;

    // Copy the basis matrix and its factorization
    memcpy( _B, otherFTFactorization->_B, sizeof(double) * _m * _m );

    for ( unsigned i = 0; i < _m; ++i )
        *_U[i] = *(otherFTFactorization->_U[i]);

    List<LPElement *>::iterator lpIt;
    for ( lpIt = _LP.begin(); lpIt != _LP.end(); ++lpIt )
        delete *lpIt;
    _LP.clear();

    for ( const auto &lp : otherFTFactorization->_LP )
        _LP.append( lp->duplicate() );

    List<AlmostIdentityMatrix *>::iterator aIt;
    for ( aIt = _A.begin(); aIt != _A.end(); ++aIt )
        delete *aIt;
    _A.clear();

    for ( const auto &a : otherFTFactorization->_A )
        _A.append( new AlmostIdentityMatrix( *a ) );
}

void ForrestTomlinFactorization::setBasis( const double *B )
{
    clearFactorization();
    memcpy( _B, B, sizeof(double) * _m * _m );
	initialLUFactorization();
    _explicitBasisAvailable = true;
}

void ForrestTomlinFactorization::clearFactorization()
{
    List<LPElement *>::iterator lpIt;
    for ( lpIt = _LP.begin(); lpIt != _LP.end(); ++lpIt )
        delete *lpIt;
    _LP.clear();

    List<AlmostIdentityMatrix *>::iterator aIt;
    for ( aIt = _A.begin(); aIt != _A.end(); ++aIt )
        delete *aIt;
    _A.clear();

    _Q.resetToIdentity();
    _invQ.resetToIdentity();

    for ( unsigned i = 0; i < _m; ++i )
        _U[i]->resetToIdentity();
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
    return _explicitBasisAvailable;
}

void ForrestTomlinFactorization::makeExplicitBasisAvailable()
{
    if ( explicitBasisAvailable() )
        return;
    /*
      We know that the following equation holds:

      Ak...A1 * LsPs...L1P1 * B = Q * Um...U1 * invQ

      Or:

      B = inv( Ak...A1 * LsPs...L1P1 ) * Q * Um...U1 * invQ
        = inv(P1)inv(L1) ... inv(Ps)inv(LS) * inv(A1)...inv(Ak)
          * Q * Um...U1 * invQ
    */

    // Start by computing: Um...U1 * invQ.
    std::fill_n( _workMatrix, _m * _m, 0.0 );
    for ( unsigned i = 0; i < _m; ++i )
        _workMatrix[i * _m + _invQ._ordering[i]] = 1.0;

    for ( unsigned i = 0; i < _m; ++i )
    {
        for ( unsigned row = 0; row < _U[i]->_columnIndex; ++row )
            for ( unsigned col = 0; col < _m; ++col )
                _workMatrix[row * _m + col] += _U[i]->_column[row] * _workMatrix[_U[i]->_columnIndex * _m + col];
    }

    // Permute the rows according to Q
    for ( unsigned i = 0; i < _m; ++i )
        memcpy( _B + ( i * _m ), _workMatrix + ( _Q._ordering[i] * _m ), sizeof(double) * _m );

    // Multiply by the inverse As. An inverse of an almost identity matrix
    // is just that matrix with its special value negated (unless that
    // matrix is diagonal.
    for ( auto a = _A.rbegin(); a != _A.rend(); ++a )
    {
        if ( (*a)->_row == (*a)->_column )
        {
            for ( unsigned j = 0; j < _m; ++j )
                _B[(*a)->_row * _m + j] *= ( 1 / (*a)->_value );
        }
        else
        {
            for ( unsigned j = 0; j < _m; ++j )
                _B[(*a)->_row * _m + j] -= _B[(*a)->_column * _m + j] * (*a)->_value;
        }
    }

    // Multiply by inverted Ps and Ls
    for ( const auto &lp : _LP )
    {
        // The inverse of a general permutation matrix is its transpose.
        // However, since these are permutations of just two rows, the
        // transpose is equal to the original - so no need to invert.
        if ( lp->_pair )
        {
            memcpy( _workVector, _B + (lp->_pair->first * _m), sizeof(double) * _m );
            memcpy( _B + (lp->_pair->first * _m), _B + (lp->_pair->second * _m), sizeof(double) * _m );
            memcpy( _B + (lp->_pair->second * _m), _workVector,  sizeof(double) * _m );
		}
        else
        {
            // The inverse of an eta matrix is an eta matrix with the special
            // column negated and divided by the diagonal entry. The only
            // exception is the diagonal entry itself, which is just the
            // inverse of the original diagonal entry.
            EtaMatrix *eta = lp->_eta;
            double etaDiagonalEntry = 1 / eta->_column[eta->_columnIndex];

            for ( unsigned row = eta->_columnIndex + 1; row < _m; ++row )
            {
                for ( unsigned col = 0; col < _m; ++col )
                {
                    _B[row * _m + col] -=
                        eta->_column[row] * etaDiagonalEntry * _B[eta->_columnIndex * _m + col];
                }
            }

            for ( unsigned col = 0; col < _m; ++col )
                _B[eta->_columnIndex * _m + col] *= etaDiagonalEntry;
        }
    }

    clearFactorization();
	initialLUFactorization();
    _explicitBasisAvailable = true;
}

const double *ForrestTomlinFactorization::getBasis() const
{
    ASSERT( _explicitBasisAvailable );
    return _B;
}

void ForrestTomlinFactorization::invertBasis( double *result )
{
    if ( !_explicitBasisAvailable )
        throw BasisFactorizationError( BasisFactorizationError::CANT_INVERT_BASIS_BECAUSE_BASIS_ISNT_AVAILABLE );

    /*
      We have a simple LU factorization of the basis, and we can use
      it to compute the inverse.
    */

    ASSERT( result );

    // Initialize the result to the identity matrix
    for ( unsigned i = 0; i < _m; ++i )
        for ( unsigned j = 0; j < _m; ++j )
            result[i * _m + j] = ( i == j ) ? 1.0 : 0.0;

    if ( _LP.empty() )
    {
        DEBUG({
                // Assert B is the identity matrix.
                for ( unsigned i = 0; i < _m; ++i )
                    for ( unsigned j = 0; j < _m; ++j )
                        ASSERT( _B[i * _m + j] == ( i == j ) ? 1.0 : 0.0 );
            });

        return;
    }

    // Apply the LP operations to the result
    for ( auto it = _LP.rbegin(); it != _LP.rend(); ++it )
    {
        const LPElement *element = *it;

        if ( element->_pair )
        {
            rowSwap( element->_pair->first, element->_pair->second, result );
        }
        else
        {
            unsigned colIndex = element->_eta->_columnIndex;

            // First, perform in-place multiplication for all rows below the pivot row
            for ( unsigned row = colIndex + 1; row < _m; ++row )
            {
                for ( unsigned col = 0; col < _m; ++col )
                    result[row * _m + col] += element->_eta->_column[row] * result[colIndex * _m + col];
            }

            // Finally, perform the multiplication for the pivot row
            // itself. We change this row last because it is required for all
            // previous multiplication operations.
            for ( unsigned i = 0; i < _m; ++i )
                result[colIndex * _m + i] *= element->_eta->_column[colIndex];
        }
    }

    // Apply the U operations to the result
    for ( int col = _m - 1; col > 0; --col )
    {
        for ( int row = col - 1; row >= 0; --row )
        {
            double uElement = _U[col]->_column[row];
            if ( FloatUtils::isZero( uElement ) )
                continue;

            for ( unsigned k = 0; k < _m; ++k )
                result[row * _m + k] += -uElement * result[col * _m + k];
        }
    }
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

const List<AlmostIdentityMatrix *> *ForrestTomlinFactorization::getA() const
{
    return &_A;
}

void ForrestTomlinFactorization::rowSwap( unsigned rowOne, unsigned rowTwo, double *matrix )
{
    memcpy( _workVector, matrix + (rowOne * _m), sizeof(double) * _m );
    memcpy( matrix + (rowOne * _m), matrix + (rowTwo * _m), sizeof(double) * _m );
    memcpy( matrix + (rowTwo * _m), _workVector,  sizeof(double) * _m );
}

void ForrestTomlinFactorization::pushA( const AlmostIdentityMatrix &matrix )
{
    _A.append( new AlmostIdentityMatrix( matrix ) );
}

void ForrestTomlinFactorization::setQ( const PermutationMatrix &Q )
{
    _Q = Q;
    _Q.invert( _invQ );
}

void ForrestTomlinFactorization::dumpU() const
{
    printf( "Dumping U:\n" );
    for ( unsigned i = 0; i < _m; ++i )
    {
        for ( unsigned j = 0; j < _m; ++j )
        {
            printf( "%6.2lf ", _U[j]->_column[i] );
        }
        printf( "\n" );
    }
}

void ForrestTomlinFactorization::dump() const
{
    printf( "*** Dumping FT factorization ***\n\n" );

    printf( "Dumping As:\n" );
    unsigned count = 0;
    for ( const auto &a : _A )
    {
        printf( "\tA%u = < %u, %u, %.5lf >\n", count, a->_row, a->_column, a->_value );
        ++count;
    }

    printf( "\nDumping LPs:\n" );
    count = 0;
    for ( const auto &lp : _LP )
    {
        printf( "LP[%i]:\n", _LP.size() - 1 - count );
        ++count;
        lp->dump();
    }
    printf( "\n\n" );

    printf( "Dumping Us:\n" );
    for ( unsigned i = 0; i < _m; ++i )
    {
        printf( "U[%u]:\n", i );
        _U[i]->dump();
        printf( "\n" );
    }

    printf( "\nDumping Q:\n" );
    _Q.dump();

    printf( "\nDumping invQ:\n" );
    _invQ.dump();

    printf( "*** Done dumping FT factorization ***\n\no" );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
