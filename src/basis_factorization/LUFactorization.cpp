/*********************                                                        */
/*! \file LUFactorization.cpp
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

#include "BasisFactorizationError.h"
#include "Debug.h"
#include "EtaMatrix.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "LPElement.h"
#include "LUFactorization.h"
#include "MStringf.h"
#include "MalformedBasisException.h"

LUFactorization::LUFactorization( unsigned m, const BasisColumnOracle &basisColumnOracle )
    : IBasisFactorization( basisColumnOracle )
    , _B0( NULL )
	, _m( m )
    , _U( NULL )
    , _LCol( NULL )
{
    _B0 = new double[m*m];
    if ( !_B0 )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "LUFactorization::B0" );

    // Initialize B0 to the identity matrix
    std::fill_n( _B0, _m * _m, 0.0 );
    for ( unsigned row = 0; row < _m; ++row )
        _B0[row * _m + row] = 1.0;

    _U = new double[m*m];
	if ( !_U )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "LUFactorization::U" );

    _LCol = new double[m];
    if ( !_LCol )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "LUFactorization::LCol" );
}

LUFactorization::~LUFactorization()
{
    freeIfNeeded();
}

void LUFactorization::freeIfNeeded()
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

    if ( _LCol )
    {
        delete[] _LCol;
        _LCol = NULL;
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

const double *LUFactorization::getU() const
{
	return _U;
}

const List<LPElement *> LUFactorization::getLP() const
{
	return _LP;
}

const double *LUFactorization::getBasis() const
{
	return _B0;
}

const List<EtaMatrix *> LUFactorization::getEtas() const
{
	return _etas;
}

void LUFactorization::pushEtaMatrix( unsigned columnIndex, const double *column )
{
    EtaMatrix *matrix = new EtaMatrix( _m, columnIndex, column );
    _etas.append( matrix );

	if ( ( _etas.size() > GlobalConfiguration::REFACTORIZATION_THRESHOLD ) && factorizationEnabled() )
	{
        log( "Number of etas exceeds threshold. Refactoring basis\n" );
        refactorizeBasis();
	}
}

void LUFactorization::LMultiplyRight( const EtaMatrix *L, double *x ) const
{
    unsigned columnIndex = L->_columnIndex;
    x[columnIndex] *= L->_column[columnIndex];

	for ( unsigned i = columnIndex + 1; i < _m; ++i )
		x[columnIndex] += L->_column[i] * x[i];

    if ( FloatUtils::isZero( x[columnIndex] ) )
        x[columnIndex] = 0.0;
}

void LUFactorization::LMultiplyLeft( const EtaMatrix *L, double *x ) const
{
    unsigned col = L->_columnIndex;
    double xCol = x[col];

    for ( unsigned i = col + 1; i < _m; ++i )
    {
        x[i] += xCol * L->_column[i];
        if ( FloatUtils::isZero( x[i] ) )
            x[i] = 0.0;
    }

    x[col] *= L->_column[col];
}

void LUFactorization::setBasis( const double *B )
{
	memcpy( _B0, B, sizeof(double) * _m * _m );
    clearFactorization();
	factorizeMatrix( _B0 );
}

void LUFactorization::forwardTransformation( const double *y, double *x ) const
{
    memcpy( x, y, sizeof(double) * _m );

    // If there's no LP factorization, it is implied that B0 = I.
    // Then, because there are no etas, x = y.
    if ( _etas.empty() && _LP.empty() )
    {
        return;
    }

    // We are solving Bx = y, where B = B0 * E1 ... *En and
    // B0 = inv( LmPm ... L1P1 ) * U. The equation is thus:
    // inv(P1) * inv(L1) ... * inv(Pm) * inv(Lm) * U * E1 ... * En * x = y.
    // The first step is to multiply both hands of the equation
    // by P1,L1,...,Pm,Lm on the left to get rid of the Ls and Ps.
    for ( auto element = _LP.rbegin(); element != _LP.rend(); ++element )
    {
        if ( (*element)->_pair )
        {
			double temp = x[(*element)->_pair->first];
			x[(*element)->_pair->first] = x[(*element)->_pair->second];
			x[(*element)->_pair->second] = temp;
		}
        else
			LMultiplyLeft( (*element)->_eta , x );
    }

    // We are now left with U * E1 ... * En * x = y. Eliminate U.
    // U can be regarded as Um * Um-1 ... * U1, where Ui indicates
    // an eta matrix whose ith column is the same as U's.
    // The inverse of each Ui is the same matrix with its non-diagonal
    // elements negated.
    if ( !_LP.empty() )
    {
        for ( int i = _m - 2; i >= 0; --i )
        {
            for ( int j = _m - 1; j > i; --j )
                x[i] -= _U[i * _m + j] * x[j];

            if ( FloatUtils::isZero( x[i] ) )
                x[i] = 0.0;
        }
    }

    // Finally, we are left with E1 * ... * En * x = y.
    // Eliminate etas one by one.
    for ( const auto &eta : _etas )
    {
        double inverseDiagonal = 1 / eta->_column[eta->_columnIndex];
        double factor = x[eta->_columnIndex] * inverseDiagonal;

        // Solve all non-diagonal rows
        for ( unsigned i = 0; i < _m; ++i )
        {
            if ( i == eta->_columnIndex )
                continue;

            x[i] -= ( factor * eta->_column[i] );
            if ( FloatUtils::isZero( x[i] ) )
                x[i] = 0.0;
        }

        // Handle the digonal element
        x[eta->_columnIndex] *= inverseDiagonal;
        if ( FloatUtils::isZero( x[eta->_columnIndex] ) )
            x[eta->_columnIndex] = 0.0;
    }
}

void LUFactorization::backwardTransformation( const double *y, double *x ) const
{
    memcpy( x, y, sizeof(double) * _m );

    // If there's no LP factorization, it is implied that B0 = I.
    // Then, because there are no etas, x = y.
    if ( _etas.empty() && _LP.empty() )
    {
        return;
    }

    // We are solving xB = y, where B = B0 * E1 ... * En.
    // The first step is to eliminate the eta matrices and adjust y
    // so that x*B0 = y.
    for ( auto eta = _etas.rbegin(); eta != _etas.rend(); ++eta )
    {
        // The only entry in y that changes is columnIndex
        unsigned columnIndex = (*eta)->_columnIndex;
        for ( unsigned i = 0; i < _m; ++i )
        {
            if ( i != columnIndex )
                x[columnIndex] -= (x[i] * (*eta)->_column[i]);
        }

        x[columnIndex] = x[columnIndex] / (*eta)->_column[columnIndex];

        if ( FloatUtils::isZero( x[columnIndex] ) )
            x[columnIndex] = 0.0;
    }

    // Now that the Etas are gone, we use the fact that
    // LmPm ... L1P1 * B0 = U. This can also be written as:
    // B0 = inv( LmPm ... L1P1 ) * U. Finally, this means that
    // x * inv( LmPm ... L1P1 ) * U = y.

    // Next step is to eliminate U by setting x'= x*inv(LP), solving x'*U = y,
    // and storing the solution for x' in x.
	if ( !_LP.empty() )
	{
		for ( unsigned i = 1; i < _m; ++i )
        {
			for ( unsigned j = 0; j < i; ++j )
                x[i] -= _U[i + j * _m] * x[j];

            if ( FloatUtils::isZero( x[i] ) )
                x[i] = 0.0;
		}
	}

    // We have in x the value for x*inv(LP). We extract the final x by multiplying
    // by the L's and P's on the right.
    // x*inv(LP) = x * inv( LmPm ... L1P1 ) = x * inv(P1) * inv(L1) ... * inv(Pm) * inv(Lm),
    // so we undo that LPs in that order.
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

void LUFactorization::rowSwap( unsigned rowOne, unsigned rowTwo, double *matrix )
{
    double temp = 0;
    for ( unsigned i = 0; i < _m; i++ )
    {
	    temp = matrix[rowOne * _m + i];
	    matrix[rowOne * _m + i] = matrix[rowTwo * _m + i];
        matrix[rowTwo * _m + i] = temp;
    }
}

void LUFactorization::clearFactorization()
{
    List<LPElement *>::iterator element;
    for ( element = _LP.begin(); element != _LP.end(); ++element )
        delete *element;
    _LP.clear();

	std::fill_n( _U, _m*_m, 0 );

    _etas.clear();
}

void LUFactorization::factorizeMatrix( double *matrix )
{
    // Initialize U
	memcpy( _U, matrix, sizeof(double) * _m * _m );

	for ( unsigned i = 0; i < _m; ++i )
    {
        // Begin work for the i'th column.
        // Employ partial pivoting: find the row with the largest element and swap it to position i
        // (See discussion at http://www2.lawrence.edu/fast/GREGGJ/Math420/Section_6_2.pdf)

        double largestElement = FloatUtils::abs( _U[i * _m + i] );
        unsigned bestRowIndex = i;

        for ( unsigned j = i + 1; j < _m; ++j )
        {
            double contender = FloatUtils::abs( _U[j * _m + i] );
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
            rowSwap( i, bestRowIndex, _U );
            std::pair<unsigned, unsigned> *P = new std::pair<unsigned, unsigned>( i, bestRowIndex );
            _LP.appendHead( new LPElement( NULL, P ) );
        }

        // The matrix now has a non-zero value at entry (i,i), so we can perform
        // Gaussian elimination for the subsequent rows
        std::fill_n( _LCol, _m, 0 );
        double div = _U[i * _m + i];
        _LCol[i] = 1 / div;
		for ( unsigned j = i + 1; j < _m; ++j )
            _LCol[j] = -_U[i + j * _m] / div;

        // Store the resulting lower-triangular eta matrix
        EtaMatrix *L = new EtaMatrix( _m, i, _LCol );
        _LP.appendHead( new LPElement( L, NULL ) );

        // Perform the actual elimination step on U
        LFactorizationMultiply( L );
	}
}

void LUFactorization::LFactorizationMultiply( const EtaMatrix *L )
{
    unsigned colIndex = L->_columnIndex;
    // First, perform in-place multiplication for all rows below the pivot row
    for ( unsigned row = colIndex + 1; row < _m; ++row )
    {
        _U[row * _m + colIndex] = 0.0;
        for ( unsigned col = colIndex + 1; col < _m; ++col )
            _U[row * _m + col] += L->_column[row] * _U[colIndex * _m + col];
    }

    // Finally, perform the multiplication for the pivot row
    // itself. We change this row last because it is required for all
    // previous multiplication operations.
	for ( unsigned i = colIndex + 1; i < _m; ++i )
		_U[colIndex * _m + i] *= L->_column[colIndex];

    _U[colIndex * _m + colIndex] = 1.0;
}

void LUFactorization::storeFactorization( IBasisFactorization *other )
{
    LUFactorization *otherLUFactorization = (LUFactorization *)other;

    ASSERT( _m == otherLUFactorization->_m );
    ASSERT( otherLUFactorization->_etas.size() == 0 );

    // In order to reduce space requirements (for the etas), compute a fresh refactorization
    if ( !_etas.empty() )
        refactorizeBasis();

    // Store the new basis and factorization
    memcpy( otherLUFactorization->_B0, _B0, sizeof(double) * _m * _m );
    memcpy( otherLUFactorization->_U, _U, sizeof(double) * _m * _m );
    for ( const auto &it : _LP )
        otherLUFactorization->_LP.append( it->duplicate() );
}

void LUFactorization::restoreFactorization( const IBasisFactorization *other )
{
    const LUFactorization *otherLUFactorization = (const LUFactorization *)other;

    ASSERT( _m == otherLUFactorization->_m );
    ASSERT( otherLUFactorization->_etas.size() == 0 );

    // Clear any existing data
    clearFactorization();

    // Store the new basis and factorization
    memcpy( _B0, otherLUFactorization->_B0, sizeof(double) * _m * _m );
    memcpy( _U, otherLUFactorization->_U, sizeof(double) * _m * _m );
    for ( const auto &it : otherLUFactorization->_LP )
        _LP.append( it->duplicate() );
}

void LUFactorization::invertBasis( double *result )
{
    if ( !_etas.empty() )
        throw BasisFactorizationError( BasisFactorizationError::CANT_INVERT_BASIS_BECAUSE_OF_ETAS );

    ASSERT( result );

    // Initialize the result to the identity matrix
    for ( unsigned i = 0; i < _m; ++i )
        for ( unsigned j = 0; j < _m; ++j )
            result[i * _m + j] = ( i == j ) ? 1.0 : 0.0;

    if ( _LP.empty() )
    {
        DEBUG({
                // Assert B0 is the identity matrix.
                for ( unsigned i = 0; i < _m; ++i )
                    for ( unsigned j = 0; j < _m; ++j )
                        ASSERT( _B0[i * _m + j] == ( i == j ) ? 1.0 : 0.0 );
            });

        return;
    }

    // Apply the LP operations to the result
    for ( List<LPElement *>::const_reverse_iterator it = _LP.rbegin(); it != _LP.rend(); ++it )
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
            double uElement = _U[row * _m + col];
            if ( FloatUtils::isZero( uElement ) )
                continue;

            for ( unsigned k = 0; k < _m; ++k )
                result[row * _m + k] += -uElement * result[col * _m + k];
        }
    }
}

void LUFactorization::log( const String &message )
{
    if ( GlobalConfiguration::BASIS_FACTORIZATION_LOGGING )
        printf( "LUFactorization: %s\n", message.ascii() );
}

bool LUFactorization::explicitBasisAvailable() const
{
    return _etas.empty();
}

void LUFactorization::makeExplicitBasisAvailable()
{
    refactorizeBasis();
}

void LUFactorization::dump() const
{
    printf( "*** Dumping LU factorization ***\n\n" );

    printf( "\nDumping LPs:\n" );
    unsigned count = 0;
    for ( const auto &lp : _LP )
    {
        printf( "LP[%i]:\n", _LP.size() - 1 - count );
        ++count;
        lp->dump();
    }
    printf( "\n\n" );

    printf( "Dumping Us:\n" );
    for ( unsigned col = 0; col < _m; ++col )
    {
        printf( "U[%u]:\n", col );
        for ( unsigned i = 0; i < _m; ++i )
        {
            printf( "\t%lf\n", _U[i * _m + col] );
        }
        printf( "\n" );
    }

    printf( "Dumping etas:\n" );
    for ( const auto &eta : _etas )
    {
        eta->dump();
        printf( "\n" );
    }

    printf( "*** Done dumping LU factorization ***\n\n" );
}

void LUFactorization::refactorizeBasis()
{
    clearFactorization();

    for ( unsigned column = 0; column < _m; ++column )
    {
        const double *basisColumn = _basisColumnOracle->getColumnOfBasis( column );
        for ( unsigned row = 0; row < _m; ++row )
            _B0[row * _m + column] = basisColumn[row];
    }

    factorizeMatrix( _B0 );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
