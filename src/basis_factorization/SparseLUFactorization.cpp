/*********************                                                        */
/*! \file SparseLUFactorization.cpp
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
#include "CSRMatrix.h"
#include "Debug.h"
#include "EtaMatrix.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "LPElement.h"
#include "MStringf.h"
#include "MalformedBasisException.h"
#include "SparseLUFactorization.h"
#include "SparseVector.h"

SparseLUFactorization::SparseLUFactorization( unsigned m, const BasisColumnOracle &basisColumnOracle )
    : IBasisFactorization( basisColumnOracle )
    , _B( m )
	, _m( m )
    , _sparseLUFactors( m )
    , _sparseGaussianEliminator( m )
    , _z( NULL )
{
    _z = new double[m];
    if ( !_z )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseLUFactorization::z" );
}

SparseLUFactorization::~SparseLUFactorization()
{
    freeIfNeeded();
}

void SparseLUFactorization::freeIfNeeded()
{
    if ( _z )
    {
        delete[] _z;
        _z = NULL;
    }

    List<EtaMatrix *>::iterator it;
    for ( it = _etas.begin(); it != _etas.end(); ++it )
        delete *it;

    _etas.clear();
}

const double *SparseLUFactorization::getBasis() const
{
    throw BasisFactorizationError( BasisFactorizationError::FEATURE_NOT_YET_SUPPORTED,
                                   "SparseLUFactorization::getBasis" );
}

const SparseMatrix *SparseLUFactorization::getSparseBasis() const
{
    throw BasisFactorizationError( BasisFactorizationError::FEATURE_NOT_YET_SUPPORTED,
                                   "SparseLUFactorization::getSparseBasis" );
}

const List<EtaMatrix *> SparseLUFactorization::getEtas() const
{
	return _etas;
}

void SparseLUFactorization::updateToAdjacentBasis( unsigned columnIndex,
                                                   const double *changeColumn,
                                                   const double */* newColumn */ )
{
    ASSERT( !FloatUtils::isZero( changeColumn[columnIndex] ) );

    EtaMatrix *matrix = new EtaMatrix( _m, columnIndex, changeColumn );
    _etas.append( matrix );

	if ( _etas.size() > GlobalConfiguration::REFACTORIZATION_THRESHOLD )
	{
        log( "Number of etas exceeds threshold. Refactoring basis\n" );
        obtainFreshBasis();
	}
}

void SparseLUFactorization::forwardTransformation( const double *y, double *x ) const
{
    /*
      We are solving Bx = y, where B = B0 * E1 ... * En.
      First we solve B0 * z = y using a forward transformation.
    */
    _sparseLUFactors.forwardTransformation( y, x );

    /*
      Now we are left with E1 * ... * En * x = z (z is stored in x)
      Eliminate etas one by one.
    */
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

void SparseLUFactorization::backwardTransformation( const double *y, double *x ) const
{
    /*
      We are solving xB = y, where B = B0 * E1 ... * En.
      The first step is to eliminate the eta matrices.
    */
    memcpy( _z, y, sizeof(double) * _m );
    for ( auto eta = _etas.rbegin(); eta != _etas.rend(); ++eta )
    {
        // The only entry in y that changes is columnIndex
        unsigned columnIndex = (*eta)->_columnIndex;
        for ( unsigned i = 0; i < _m; ++i )
        {
            if ( i != columnIndex )
                _z[columnIndex] -= (_z[i] * (*eta)->_column[i]);
        }

        _z[columnIndex] = _z[columnIndex] / (*eta)->_column[columnIndex];

        if ( FloatUtils::isZero( _z[columnIndex] ) )
            _z[columnIndex] = 0.0;
    }

    /*
      We now need to solve xB0 = z. Use a backward transformation.
    */
    _sparseLUFactors.backwardTransformation( _z, x );
}

void SparseLUFactorization::clearFactorization()
{
	List<EtaMatrix *>::iterator it;
    for ( it = _etas.begin(); it != _etas.end(); ++it )
        delete *it;
    _etas.clear();
}

void SparseLUFactorization::factorizeBasis()
{
    clearFactorization();

    try
    {
        _sparseGaussianEliminator.run( &_B, &_sparseLUFactors );
    }
    catch ( const BasisFactorizationError &e )
    {
        if ( e.getCode() == BasisFactorizationError::GAUSSIAN_ELIMINATION_FAILED )
            throw MalformedBasisException();
        else
            throw e;
    }
}

void SparseLUFactorization::storeFactorization( IBasisFactorization *other )
{
    SparseLUFactorization *otherSparseLUFactorization = (SparseLUFactorization *)other;

    ASSERT( _m == otherSparseLUFactorization->_m );
    ASSERT( otherSparseLUFactorization->_etas.size() == 0 );

    obtainFreshBasis();

    // Store the new basis and factorization
    _sparseLUFactors.storeToOther( &otherSparseLUFactorization->_sparseLUFactors );
}

void SparseLUFactorization::restoreFactorization( const IBasisFactorization *other )
{
    const SparseLUFactorization *otherSparseLUFactorization = (const SparseLUFactorization *)other;

    ASSERT( _m == otherSparseLUFactorization->_m );
    ASSERT( otherSparseLUFactorization->_etas.size() == 0 );

    // Clear any existing data
    clearFactorization();

    // Store the new basis and factorization
    otherSparseLUFactorization->_sparseLUFactors.storeToOther( &_sparseLUFactors );
}

void SparseLUFactorization::invertBasis( double *result )
{
    if ( !_etas.empty() )
        throw BasisFactorizationError( BasisFactorizationError::CANT_INVERT_BASIS_BECAUSE_OF_ETAS );

    ASSERT( result );

    _sparseLUFactors.invertBasis( result );
}

void SparseLUFactorization::log( const String &message )
{
    if ( GlobalConfiguration::BASIS_FACTORIZATION_LOGGING )
        printf( "SparseLUFactorization: %s\n", message.ascii() );
}

bool SparseLUFactorization::explicitBasisAvailable() const
{
    return _etas.empty();
}

void SparseLUFactorization::makeExplicitBasisAvailable()
{
    obtainFreshBasis();
}

void SparseLUFactorization::dump() const
{
    printf( "*** Dumping LU factorization ***\n\n" );

    printf( "\nDumping LU factors:\n" );
    _sparseLUFactors.dump();
    printf( "\n\n" );

    printf( "Dumping etas:\n" );
    for ( const auto &eta : _etas )
    {
        eta->dump();
        printf( "\n" );
    }
    printf( "*** Done dumping LU factorization ***\n\n" );
}

void SparseLUFactorization::obtainFreshBasis()
{
    _basisColumnOracle->getSparseBasis( _B );
    factorizeBasis();
}

void SparseLUFactorization::dumpExplicitBasis() const
{
    // The basis is given by:   B = F * V * Etas

    double *result = new double[_m * _m];
    double *toMultiply = new double[_m * _m];
    double *temp = new double[_m * _m];

    // Start with F
    _sparseLUFactors._F->toDense( result );
    for ( unsigned i = 0; i < _m; ++i )
        result[i*_m + i] = 1;

    // Multiply by V
    _sparseLUFactors._V->toDense( toMultiply );

    for ( unsigned i = 0; i < _m; ++i )
    {
        for ( unsigned j = 0; j < _m; ++j )
        {
            temp[i*_m + j] = 0;
            for ( unsigned k = 0; k < _m; ++k )
            {
                temp[i*_m + j] += ( result[i*_m + k] * toMultiply[k*_m + j] );
            }
        }
    }
    memcpy( result, temp, sizeof(double) * _m * _m );

    // Go eta by eta
    for ( const auto &eta : _etas )
    {
        eta->toMatrix( toMultiply );

        for ( unsigned i = 0; i < _m; ++i )
        {
            for ( unsigned j = 0; j < _m; ++j )
            {
                temp[i*_m + j] = 0;
                for ( unsigned k = 0; k < _m; ++k )
                {
                    temp[i*_m + j] += ( result[i*_m + k] * toMultiply[k*_m + j] );
                }
            }
        }

        memcpy( result, temp, sizeof(double) * _m * _m );
    }

    // Print out the result
    printf( "SparseLUFactorization dumping explicit basis:\n" );
    for ( unsigned i = 0; i < _m; ++i )
    {
        printf( "\t" );
        for ( unsigned j = 0; j < _m; ++j )
        {
            printf( "%5.2lf ", result[i*_m + j] );
        }

        printf( "\n" );
    }
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
