/*********************                                                        */
/*! \file BasisFactorization.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Derek Huang
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "BasisFactorization.h"
#include "Debug.h"
#include "EtaMatrix.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "LPElement.h"
#include "ReluplexError.h"

BasisFactorization::BasisFactorization( unsigned m )
    : _B0( NULL )
	, _m( m )
    , _U( NULL )
    , _factorizationEnabled( true )
{
    _B0 = new double[m*m];
    if ( !_B0 )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "BasisFactorization::B0" );

	_U = new double[m*m];
	if ( !_U )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "BasisFactorization::U" );
}

BasisFactorization::~BasisFactorization()
{
    freeIfNeeded();
}

void BasisFactorization::freeIfNeeded()
{
    if ( _U )
    {
        delete[] _U;
        _U = NULL;
    }

	if ( _B0 )
	{
		delete[] _B0;
		_B0 = NULL;
	}

    List<EtaMatrix *>::iterator it;
    for ( it = _etas.begin(); it != _etas.end(); ++it )
        delete *it;

    _etas.clear();

    List<LPElement *>::iterator element;
    for ( element = _LP.begin(); element != _LP.end(); ++element )
        delete *element;

	_LP.clear();
}

const double *BasisFactorization::getU() const
{
	return _U;
}

const List<LPElement *> BasisFactorization::getLP() const
{
	return _LP;
}

const double *BasisFactorization::getB0() const
{
	return _B0;
}

const List<EtaMatrix *> BasisFactorization::getEtas() const
{
	return _etas;
}

void BasisFactorization::pushEtaMatrix( unsigned columnIndex, double *column )
{
    EtaMatrix *matrix = new EtaMatrix( _m, columnIndex, column );
    _etas.append( matrix );

	if ( ( _etas.size() > GlobalConfiguration::REFACTORIZATION_THRESHOLD ) && _factorizationEnabled )
	{
		condenseEtas();
		factorizeMatrix( _B0 );
	}
}

void BasisFactorization::LMultiplyRight( const EtaMatrix *L, double *X )
{
	double sum = 0;
	for ( unsigned i = 0; i < _m; ++i )
		sum += L->_column[i] * X[i];
	X[L->_columnIndex] = sum;
}

void BasisFactorization::LMultiplyLeft( const EtaMatrix *L, double *X )
{
    unsigned col = L->_columnIndex;
    double xCol = X[col];
    for ( unsigned i = 0; i < _m; ++i )
    {
        if ( i == col )
            X[i] *= L->_column[col];
        else
            X[i] += xCol * L->_column[i];
    }
}

void BasisFactorization::setB0( const double *B0 )
{
	memcpy( _B0, B0, sizeof(double) * _m * _m);
	factorizeMatrix( _B0 );
}

void BasisFactorization::condenseEtas()
{
    // If there are no Eta matrices stored, B0 becomes the identity matrix
    if ( _LP.empty() )
    {
        std::fill_n( _B0, _m * _m, 0.0 );
        for ( unsigned row = 0; row < _m; ++row )
            _B0[row * _m + row] = 1.0;
    }

    // Multiplication by an eta matrix on the right only changes one
    // column of B0. The new column is a linear combination of the
    // existing columns of B0, according to the eta column. We perform
    // the computation in place.
    for ( const auto &eta : _etas )
    {
		unsigned col = eta->_columnIndex;

        // Iterate over the rows of B0
		for ( unsigned i = 0; i < _m; ++i )
        {
            // Compute the linear combinations of the columns
			double sum = 0.0;
			for ( unsigned j = 0; j < _m; ++j )
				sum += _B0[i * _m + j] * eta->_column[j];
            _B0[i * _m + col] = sum;
		}

		delete eta;
	}

	_etas.clear();
	clearLPU();
}

void BasisFactorization::forwardTransformation( const double *y, double *x )
{
    // If there's no LP factorization, it is implied that B0 = I.
    // Then, because there are no etas, x = y.
    if ( _etas.empty() && _LP.empty() )
    {
        memcpy( x, y, sizeof(double) * _m );
        return;
    }

    double *tempY = new double[_m];
    memcpy( tempY, y, sizeof(double) * _m );
    std::fill( x, x + _m, 0.0 );

    // We are solving Bx = y, where B = B0 * E1 ... *En and
    // B0 = inv( LmPm ... L1P1 ) * U. The first step is thus to
    // multiply both hands of the equation by P1,L1,...,Pm,Lm on the left
    // to get rid of the L's and P's.

    // Todo: Guy needs to be convinced that we shouldn't be multiplying by LP's in non-reverse order
    for ( auto element = _LP.rbegin(); element != _LP.rend(); ++element )
    {
        if ( (*element)->_pair )
        {
			double temp = x[(*element)->_pair->first];
			x[(*element)->_pair->first] = x[(*element)->_pair->second];
			x[(*element)->_pair->second] = temp;
		}
        else
			LMultiplyLeft( (*element)->_eta , tempY );
	}

    // We are now left with U * E1 ... * En * x = y. Eliminate U.
	if ( !_LP.empty() )
    {
		x[_m-1] = tempY[_m-1];
		for ( int i = _m - 2; i >= 0; --i )
        {
			double sum = 0;
			for ( int j = _m - 1; j > i; --j )
				sum += _U[i * _m + j] * x[j];
			x[i] = tempY[i] - sum;
		}
		memcpy( tempY, x, sizeof(double) * _m );
	}

    // Finally, we are left with E1 * ... * En * x = y.
    // Eliminate etas one by one.
	for ( const auto &eta : _etas )
    {
        x[eta->_columnIndex] = tempY[eta->_columnIndex] / eta->_column[eta->_columnIndex];

        // Solve the rows above columnIndex
        for ( unsigned i = eta->_columnIndex + 1; i < _m; ++i )
            x[i] = tempY[i] - ( x[eta->_columnIndex] * eta->_column[i] );

        // Solve the rows below columnIndex
        for ( int i = eta->_columnIndex - 1; i >= 0; i-- )
            x[i] = tempY[i] - ( x[eta->_columnIndex] * eta->_column[i] );

        // The x from this iteration becomes a for the next iteration
        memcpy( tempY, x, sizeof(double) *_m );
    }
}

void BasisFactorization::backwardTransformation( const double *y, double *x )
{
    // If there's no LP factorization, it is implied that B0 = I.
    // Then, because there are no etas, x = y.
    if ( _etas.empty() && _LP.empty() )
    {
        memcpy( x, y, sizeof(double) * _m );
        return;
    }

    double *tempY = new double[_m];
    memcpy( tempY, y, sizeof(double) * _m );
    std::fill( x, x + _m, 0.0 );

    // We are solving xB = y, where B = B0 * E1 ... * En.
    // The first step is to eliminate the eta matrices and adjust y
    // so that x*B0 = y.
    for ( auto eta = _etas.rbegin(); eta != _etas.rend(); ++eta )
    {
        // x is going to equal y in all entries except columnIndex
        memcpy( x, tempY, sizeof(double) * _m );

        // Compute the special column
        unsigned columnIndex = (*eta)->_columnIndex;
        x[columnIndex] = tempY[columnIndex];
        for ( unsigned i = 0; i < _m; ++i )
        {
            if ( i != columnIndex )
                x[columnIndex] -= (x[i] * (*eta)->_column[i]);
        }
        x[columnIndex] = x[columnIndex] / (*eta)->_column[columnIndex];

        // To handle -0.0
        if ( FloatUtils::isZero( x[columnIndex] ) )
            x[columnIndex] = 0.0;

        // The x from this iteration becomes y for the next iteration
        memcpy( tempY, x, sizeof(double) *_m );
    }

    // Now that the Etas are gone, we use the fact that
    // LmPm ... L1P1 * B0 = U. This can also be written as:
    // B0 = inv( LmPm ... L1P1 ) * U. Finally, this means that
    // x * inv( LmPm ... L1P1 ) * U = y.

    // Next step is to eliminate U by setting x'= x*inv(LP), solving x'*U = y,
    // and storing the solution for x' in x.
	if ( !_LP.empty() )
	{
		x[0] = tempY[0];
		for ( unsigned i = 1; i < _m; ++i )
        {
			double sum = 0;
			for ( unsigned j = 0; j < i; ++j )
				sum += _U[i + j * _m] * x[j];
			x[i] = tempY[i] - sum;
		}
	}

    // We have in x the value for x*inv(LP). We extract the final x by multiplying
    // by the L's and P's on the right.
    // x*inv(LP) = x * inv( LmPm ... L1P1 ) = x * inv(P1) * inv(L1) ... * inv(Pm) * inv(Lm),
    // so we undo that LPs in that way.

    // Todo: Guy needs to be convinced that we shouldn't be multiplying by LP's in reverse order
    for ( const auto *d : _LP )
	{
		if ( d->_pair )
        {
            double temp = x[d->_pair->first];
			x[d->_pair->first] = x[d->_pair->second];
			x[d->_pair->second] = temp;
		}
        else
			LMultiplyRight( d->_eta, x );
	}
}

void BasisFactorization::storeFactorization( BasisFactorization *other ) const
{
    ASSERT( _m == other->_m );
    ASSERT( other->_etas.size() == 0 );

    memcpy( other->_B0, _B0, sizeof(double) * _m * _m );

    for ( const auto &eta : _etas )
        other->pushEtaMatrix( eta->_columnIndex, eta->_column );
}

void BasisFactorization::restoreFactorization( const BasisFactorization *other )
{
    ASSERT( _m == other->_m );

    for ( const auto &it : _etas )
        delete it;

    _etas.clear();

    memcpy( _B0, other->_B0, sizeof(double) * _m * _m );

    for ( const auto &eta : other->_etas )
        _etas.append( new EtaMatrix( eta->_m, eta->_columnIndex, eta->_column ) );
}

void BasisFactorization::rowSwap( unsigned rowOne, unsigned rowTwo, double *matrix )
{
    double temp = 0;
    for ( unsigned i = 0; i < _m; i++ )
    {
	    temp = matrix[rowOne * _m + i];
	    matrix[rowOne * _m + i] = matrix[rowTwo * _m + i];
        matrix[rowTwo * _m + i] = temp;
    }
}

void BasisFactorization::clearLPU()
{
    List<LPElement *>::iterator element;
    for ( element = _LP.begin(); element != _LP.end(); ++element )
        delete *element;
	_LP.clear();

	std::fill_n( _U, _m*_m, 0 );
}

void BasisFactorization::factorizeMatrix( double *matrix )
{
    // Clear any previous factorization, initialize U
	clearLPU();
	memcpy( _U, matrix, sizeof(double) * _m * _m );

	for ( unsigned i = 0; i < _m; ++i )
    {
        // Begin work for the i'th column. Look for a row with a non-zero entry
		if ( _U[i*_m+i] == 0 )
        {
			unsigned goodRow = i;

			while ( ( _U[goodRow * _m + i] == 0 ) && ( goodRow < _m ) )
                ++goodRow;

            // No pivot has been found, matrix cannot be factorized
			if ( goodRow == _m )
                throw ReluplexError( ReluplexError::NO_AVAILABLE_CANDIDATES, "No Pivot" );

            // Swap rows i and goodRow, and store this permutation
			rowSwap( i, goodRow, _U );
            std::pair<int, int> *P = new std::pair<int, int>( i, goodRow );
            _LP.appendHead( new LPElement( NULL, P ) );
		}

        // The matrix now has a non-zero value at entry (i,i), so we can perform
        // Gaussian elimination for the subsequent rows
        double *LCol = new double[_m];
        std::fill_n( LCol, _m, 0 );
        double div = _U[i * _m + i];
        LCol[i] = 1 / div;
		for ( unsigned j = i + 1; j < _m; ++j )
            LCol[j] = -_U[ i + j * _m] / div;

        // Store the resulting lower-triangular eta matrix
		EtaMatrix *L = new EtaMatrix( _m, i, LCol );
        _LP.appendHead( new LPElement( L, NULL ) );
        delete[] LCol;

        // Perform the actual elimination step on U
        LFactorizationMultiply( L );
	}
}

void BasisFactorization::LFactorizationMultiply( const EtaMatrix *L )
{
    unsigned colIndex = L->_columnIndex;
    // First, perform in-place multiplication for all rows below the pivot row
    for ( unsigned col = colIndex; col < _m; ++col )
    {
        for ( unsigned row = colIndex + 1; row < _m; ++row )
            _U[row * _m + col] += L->_column[row] * _U[colIndex * _m + col];
    }

    // Finally, perform the multiplication for the pivot row
    // itself. We change this row last because it is required for all
    // previous multiplication operations.
	for ( unsigned i = colIndex; i < _m; ++i )
		_U[colIndex * _m + i] *= L->_column[colIndex];
}

void BasisFactorization::matrixMultiply( unsigned dimension, const double *left, const double *right, double *result )
{
    for ( unsigned leftRow = 0; leftRow < dimension; ++leftRow )
    {
        for ( unsigned rightCol = 0; rightCol < dimension; ++rightCol )
        {
            double sum = 0;
			for ( unsigned i = 0; i < dimension; ++i )
				sum += left[leftRow * dimension + i] * right[i * dimension + rightCol];

			result[leftRow * dimension + rightCol] = sum;
		}
	}
}

bool BasisFactorization::factorizationEnabled() const
{
    return _factorizationEnabled;
}

void BasisFactorization::toggleFactorization( bool value )
{
    _factorizationEnabled = value;
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
