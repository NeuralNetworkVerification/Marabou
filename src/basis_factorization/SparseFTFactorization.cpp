/*********************                                                        */
/*! \file SparseFTFactorization.cpp
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
#include "Debug.h"
#include "GlobalConfiguration.h"
#include "MalformedBasisException.h"
#include "SparseFTFactorization.h"

SparseFTFactorization::SparseFTFactorization( unsigned m, const BasisColumnOracle &basisColumnOracle )
    : IBasisFactorization( basisColumnOracle )
	, _m( m )
    , _sparseLUFactors( m )
    , _sparseGaussianEliminator( m )
    , _z( NULL )
{
    _B = new CSRMatrix;
    if ( !_B )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseFTFactorization::B" );
    _B->initializeToEmpty( m, m );

    _H = new CSRMatrix;
    if ( !_H )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseFTFactorization::H" );
    _H->initializeToEmpty( m, m ); // TODO: initialize to identity?

    _z = new double[m];
    if ( !_z )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseFTFactorization::z" );
}

SparseFTFactorization::~SparseFTFactorization()
{
    freeIfNeeded();
}

void SparseFTFactorization::freeIfNeeded()
{
	if ( _B )
	{
		delete _B;
		_B = NULL;
	}

	if ( _H )
	{
		delete _H;
		_H = NULL;
	}

    if ( _z )
    {
        delete[] _z;
        _z = NULL;
    }

    // TODO: H?
}

const double *SparseFTFactorization::getBasis() const
{
    printf( "Error! dense getBasis() not supported for SparseFTFactorization!\n" );
    exit( 1 );
}

const SparseMatrix *SparseFTFactorization::getSparseBasis() const
{
	return _B;
}

void SparseFTFactorization::updateToAdjacentBasis( unsigned columnIndex,
                                                   const double */* changeColumn */,
                                                   const SparseVector *newColumn )
{
    --columnIndex;
    if ( newColumn )
        printf( "BLA" );
}

void SparseFTFactorization::setBasis( const double *B )
{
    _B->initialize( B, _m, _m );
	factorizeBasis();

    // DO SOMETHING WITH H?
}

void SparseFTFactorization::forwardTransformation( const double *y, double *x ) const
{
    // /*
    //   We are solving Bx = y, where B = B0 * E1 ... * En.
    //   First we solve B0 * z = y using a forward transformation.
    // */
    // _sparseFTFactors.forwardTransformation( y, x );

    // /*
    //   Now we are left with E1 * ... * En * x = z (z is stored in x)
    //   Eliminate etas one by one.
    // */
    // for ( const auto &eta : _etas )
    // {
    //     double inverseDiagonal = 1 / eta->_column[eta->_columnIndex];
    //     double factor = x[eta->_columnIndex] * inverseDiagonal;

    //     // Solve all non-diagonal rows
    //     for ( unsigned i = 0; i < _m; ++i )
    //     {
    //         if ( i == eta->_columnIndex )
    //             continue;

    //         x[i] -= ( factor * eta->_column[i] );
    //         if ( FloatUtils::isZero( x[i] ) )
    //             x[i] = 0.0;
    //     }

    //     // Handle the digonal element
    //     x[eta->_columnIndex] *= inverseDiagonal;
    //     if ( FloatUtils::isZero( x[eta->_columnIndex] ) )
    //         x[eta->_columnIndex] = 0.0;
    // }

    memcpy( x, y, sizeof(double) * _m );
}

void SparseFTFactorization::backwardTransformation( const double *y, double *x ) const
{
    // /*
    //   We are solving xB = y, where B = B0 * E1 ... * En.
    //   The first step is to eliminate the eta matrices.
    // */
    // memcpy( _z, y, sizeof(double) * _m );
    // for ( auto eta = _etas.rbegin(); eta != _etas.rend(); ++eta )
    // {
    //     // The only entry in y that changes is columnIndex
    //     unsigned columnIndex = (*eta)->_columnIndex;
    //     for ( unsigned i = 0; i < _m; ++i )
    //     {
    //         if ( i != columnIndex )
    //             _z[columnIndex] -= (_z[i] * (*eta)->_column[i]);
    //     }

    //     _z[columnIndex] = _z[columnIndex] / (*eta)->_column[columnIndex];

    //     if ( FloatUtils::isZero( _z[columnIndex] ) )
    //         _z[columnIndex] = 0.0;
    // }

    // /*
    //   We now need to solve xB0 = z. Use a backward transformation.
    // */
    // _sparseFTFactors.backwardTransformation( _z, x );

    memcpy( x, y, sizeof(double) * _m );
}

void SparseFTFactorization::clearFactorization()
{
	// List<EtaMatrix *>::iterator it;
    // for ( it = _etas.begin(); it != _etas.end(); ++it )
    //     delete *it;
    // _etas.clear();

    // TODO: H?
}

void SparseFTFactorization::factorizeBasis()
{
    clearFactorization();

    try
    {
        _sparseGaussianEliminator.run( _B, &_sparseLUFactors );
    }
    catch ( const BasisFactorizationError &e )
    {
        if ( e.getCode() == BasisFactorizationError::GAUSSIAN_ELIMINATION_FAILED )
            throw MalformedBasisException();
        else
            throw e;
    }
}

void SparseFTFactorization::storeFactorization( IBasisFactorization *other )
{
    SparseFTFactorization *otherSparseFTFactorization = (SparseFTFactorization *)other;

    ASSERT( _m == otherSparseFTFactorization->_m );
    // ASSERT( otherSparseFTFactorization->_etas.size() == 0 );

    // TODO: H?

    obtainFreshBasis();

    // Store the new basis and factorization
    _B->storeIntoOther( otherSparseFTFactorization->_B );
    _sparseLUFactors.storeToOther( &otherSparseFTFactorization->_sparseLUFactors );
}

void SparseFTFactorization::restoreFactorization( const IBasisFactorization *other )
{
    const SparseFTFactorization *otherSparseFTFactorization = (const SparseFTFactorization *)other;

    ASSERT( _m == otherSparseFTFactorization->_m );
    // ASSERT( otherSparseFTFactorization->_etas.size() == 0 );

    // TODO: H?

    // Clear any existing data
    clearFactorization();

    // Store the new basis and factorization
    otherSparseFTFactorization->_B->storeIntoOther( _B );
    otherSparseFTFactorization->_sparseLUFactors.storeToOther( &_sparseLUFactors );
}

void SparseFTFactorization::invertBasis( double *result )
{
    // if ( !_etas.empty() )
    //     throw BasisFactorizationError( BasisFactorizationError::CANT_INVERT_BASIS_BECAUSE_OF_ETAS );

    // TODO: H?

    ASSERT( result );

    _sparseLUFactors.invertBasis( result );
}

void SparseFTFactorization::log( const String &message )
{
    if ( GlobalConfiguration::BASIS_FACTORIZATION_LOGGING )
        printf( "SparseFTFactorization: %s\n", message.ascii() );
}

bool SparseFTFactorization::explicitBasisAvailable() const
{
    // return _etas.empty();
    return false;
    // TODO: H?
}

void SparseFTFactorization::makeExplicitBasisAvailable()
{
    obtainFreshBasis();
}

void SparseFTFactorization::dump() const
{
    printf( "*** Dumping FT factorization ***\n\n" );

    printf( "\nDumping FT factors:\n" );
    _sparseLUFactors.dump();
    printf( "\n\n" );

    printf( "*** Done dumping FT factorization ***\n\n" );
}

void SparseFTFactorization::obtainFreshBasis()
{
    _B->initializeToEmpty( _m, _m );

    SparseVector column;
    for ( unsigned columnIndex = 0; columnIndex < _m; ++columnIndex )
    {
        _basisColumnOracle->getColumnOfBasis( columnIndex, &column );

        for ( unsigned entry = 0; entry < column.getNnz(); ++entry )
            _B->commitChange( column.getIndexOfEntry( entry ), columnIndex, column.getValueOfEntry( entry ) );
    }
    _B->executeChanges();

    factorizeBasis();

    // TOOD: H?
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
