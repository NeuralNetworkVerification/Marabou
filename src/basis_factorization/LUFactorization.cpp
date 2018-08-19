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
    , _B( NULL )
	, _m( m )
    , _luFactors( m )
    , _gaussianEliminator( m )
    , _z( NULL )
{
    _B = new double[m*m];
    if ( !_B )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "LUFactorization::B" );

    _z = new double[m];
    if ( !_z )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "LUFactorization::z" );
}

LUFactorization::~LUFactorization()
{
    freeIfNeeded();
}

void LUFactorization::freeIfNeeded()
{
	if ( _B )
	{
		delete[] _B;
		_B = NULL;
	}

    List<EtaMatrix *>::iterator it;
    for ( it = _etas.begin(); it != _etas.end(); ++it )
        delete *it;

    _etas.clear();
}

const double *LUFactorization::getBasis() const
{
	return _B;
}

const SparseMatrix *LUFactorization::getSparseBasis() const
{
    printf( "Error! sparse getBasis() not supported for LUFactorization!\n" );
    exit( 1 );
}

const List<EtaMatrix *> LUFactorization::getEtas() const
{
	return _etas;
}

void LUFactorization::updateToAdjacentBasis( unsigned columnIndex,
                                             const double *changeColumn,
                                             const SparseVector */* newColumn */ )
{
    EtaMatrix *matrix = new EtaMatrix( _m, columnIndex, changeColumn );
    _etas.append( matrix );

	if ( _etas.size() > GlobalConfiguration::REFACTORIZATION_THRESHOLD )
	{
        log( "Number of etas exceeds threshold. Refactoring basis\n" );
        obtainFreshBasis();
	}
}

void LUFactorization::setBasis( const double *B )
{
	memcpy( _B, B, sizeof(double) * _m * _m );
	factorizeBasis();
}

void LUFactorization::forwardTransformation( const double *y, double *x ) const
{
    /*
      We are solving Bx = y, where B = B0 * E1 ... * En.
      First we solve B0 * z = y using a forward transformation.
    */
    _luFactors.forwardTransformation( y, x );

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

void LUFactorization::backwardTransformation( const double *y, double *x ) const
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
    _luFactors.backwardTransformation( _z, x );
}

void LUFactorization::clearFactorization()
{
	List<EtaMatrix *>::iterator it;
    for ( it = _etas.begin(); it != _etas.end(); ++it )
        delete *it;
    _etas.clear();
}

void LUFactorization::factorizeBasis()
{
    clearFactorization();

    try
    {
        _gaussianEliminator.run( _B, &_luFactors );
    }
    catch ( const BasisFactorizationError &e )
    {
        if ( e.getCode() == BasisFactorizationError::GAUSSIAN_ELIMINATION_FAILED )
            throw MalformedBasisException();
        else
            throw e;
    }
}

void LUFactorization::storeFactorization( IBasisFactorization *other )
{
    LUFactorization *otherLUFactorization = (LUFactorization *)other;

    ASSERT( _m == otherLUFactorization->_m );
    ASSERT( otherLUFactorization->_etas.size() == 0 );

    obtainFreshBasis();

    // Store the new basis and factorization
    memcpy( otherLUFactorization->_B, _B, sizeof(double) * _m * _m );
    _luFactors.storeToOther( &otherLUFactorization->_luFactors );
}

void LUFactorization::restoreFactorization( const IBasisFactorization *other )
{
    const LUFactorization *otherLUFactorization = (const LUFactorization *)other;

    ASSERT( _m == otherLUFactorization->_m );
    ASSERT( otherLUFactorization->_etas.size() == 0 );

    // Clear any existing data
    clearFactorization();

    // Store the new basis and factorization
    memcpy( _B, otherLUFactorization->_B, sizeof(double) * _m * _m );
    otherLUFactorization->_luFactors.storeToOther( &_luFactors );
}

void LUFactorization::invertBasis( double *result )
{
    if ( !_etas.empty() )
        throw BasisFactorizationError( BasisFactorizationError::CANT_INVERT_BASIS_BECAUSE_OF_ETAS );

    ASSERT( result );

    _luFactors.invertBasis( result );
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
    obtainFreshBasis();
}

void LUFactorization::dump() const
{
    printf( "*** Dumping LU factorization ***\n\n" );

    printf( "\nDumping LU factors:\n" );
    _luFactors.dump();
    printf( "\n\n" );

    printf( "Dumping etas:\n" );
    for ( const auto &eta : _etas )
    {
        eta->dump();
        printf( "\n" );
    }
    printf( "*** Done dumping LU factorization ***\n\n" );
}

void LUFactorization::obtainFreshBasis()
{
    for ( unsigned column = 0; column < _m; ++column )
    {
        _basisColumnOracle->getColumnOfBasis( column, _z );
        for ( unsigned row = 0; row < _m; ++row )
            _B[row * _m + column] = _z[row];
    }

    factorizeBasis();
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
