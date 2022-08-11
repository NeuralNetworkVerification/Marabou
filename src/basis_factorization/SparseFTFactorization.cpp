/*********************                                                        */
/*! \file SparseFTFactorization.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#include "BasisFactorizationError.h"
#include "Debug.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "MalformedBasisException.h"
#include "SparseFTFactorization.h"

SparseFTFactorization::SparseFTFactorization( unsigned m, const BasisColumnOracle &basisColumnOracle )
    : IBasisFactorization( basisColumnOracle )
    , _B( m )
	, _m( m )
    , _sparseLUFactors( m )
    , _sparseGaussianEliminator( m )
    , _statistics( NULL )
    , _z1( NULL )
    , _z2( NULL )
    , _z3( NULL )
    , _z4( NULL )
{
    _z1 = new double[m];
    if ( !_z1 )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseFTFactorization::z1" );

    _z2 = new double[m];
    if ( !_z2 )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseFTFactorization::z2" );

    _z3 = new double[m];
    if ( !_z3 )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseFTFactorization::z3" );

    _z4 = new double[m];
    if ( !_z4 )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseFTFactorization::z4" );
}

SparseFTFactorization::~SparseFTFactorization()
{
    freeIfNeeded();
}

void SparseFTFactorization::freeIfNeeded()
{
    clearFactorization();

    if ( _z1 )
    {
        delete[] _z1;
        _z1 = NULL;
    }

    if ( _z2 )
    {
        delete[] _z2;
        _z2 = NULL;
    }

    if ( _z3 )
    {
        delete[] _z3;
        _z3 = NULL;
    }

    if ( _z4 )
    {
        delete[] _z4;
        _z4 = NULL;
    }
}

const double *SparseFTFactorization::getBasis() const
{
    printf( "Error! dense getBasis() not supported for SparseFTFactorization!\n" );
    exit( 1 );
}

const SparseMatrix *SparseFTFactorization::getSparseBasis() const
{
    printf( "SparseFTFactorization::getSparseBasis not supported!\n" );
    exit( 1 );
}

void SparseFTFactorization::updateToAdjacentBasis( unsigned columnIndex,
                                                   const double */* changeColumn */,
                                                   const double *newColumn )
{
    // q = columnIndex
    // s = uColumnIndex
    // p = vRowDiagonalIndex
    // t = lastNonZeroEntryInU

    if ( _etas.size() > GlobalConfiguration::REFACTORIZATION_THRESHOLD )
    {
        obtainFreshBasis();
        return;
    }

    fixPForL();

    /*
      The columnIndex column of B needs to be replaced with the new column.

      Step 1:

      Because B = FHV, this means that the columnIndex column of V needs to be
      replaced with:

        inv( FH ) * newColumn

      In terms of U, a column of U needs to be replaced, and the column index
      is determined by the permutation matrices and the connection to V.
    */

    // The index of the U column that changes
    unsigned uColumnIndex = _sparseLUFactors._Q._columnOrdering[columnIndex];
    unsigned vRowDiagonalIndex = _sparseLUFactors._P._columnOrdering[uColumnIndex];

    // Compute the new column of V: inv( FH ) * newColumn
    _sparseLUFactors.fForwardTransformation( newColumn, _z3 );
    hForwardTransformation( _z3, _z4 );

    // Replace this column of V in the sparse factors
    // Also find the index of the last non-zero entry in this column, for U
    unsigned lastNonZeroEntryInU = 0;
    DEBUG( bool foundNonZeroEntry = false );

    _sparseLUFactors._Vt->clear( columnIndex );
    for ( unsigned i = 0; i < _m; ++i )
    {
        if ( !FloatUtils::isZero( _z4[i] ) )
        {
            DEBUG( foundNonZeroEntry = true );

            unsigned uRow = _sparseLUFactors._P._rowOrdering[i];
            if ( uRow > lastNonZeroEntryInU )
                lastNonZeroEntryInU = uRow;

            _sparseLUFactors._Vt->append( columnIndex, i, _z4[i] );
        }

        _sparseLUFactors._V->set( i, columnIndex, _z4[i] );
    }

    double pivotElement = _z4[vRowDiagonalIndex];

    /*
      Step 2:

      The column of V has been replaced, and consequently so has the column of U.
      If U is upper traingular, we are done.
    */

    ASSERT( foundNonZeroEntry );
    if ( lastNonZeroEntryInU <= uColumnIndex )
    {
        _sparseLUFactors._vDiagonalElements[vRowDiagonalIndex] = pivotElement;
        ASSERT( uColumnIndex == lastNonZeroEntryInU ); // Otherwise, singular matrix
        return;
    }

    /*
      Step 3:

      Perform permutations of matrix U, to move its spike from being a
      column to being a row - so that we can afterwards eliminate it.
    */

    for ( unsigned i = uColumnIndex; i < lastNonZeroEntryInU; ++i )
    {
        // Rows move upwards, columns move to the left
        _sparseLUFactors._P._columnOrdering[i] = _sparseLUFactors._P._columnOrdering[i+1];
        _sparseLUFactors._Q._rowOrdering[i] = _sparseLUFactors._Q._rowOrdering[i+1];

        // Adjsut the transposed permutations, also
        _sparseLUFactors._P._rowOrdering[_sparseLUFactors._P._columnOrdering[i]] = i;
        _sparseLUFactors._Q._columnOrdering[_sparseLUFactors._Q._rowOrdering[i]] = i;
    }

    // Finally, the spike (row, column) are moved to the last position
    _sparseLUFactors._P._columnOrdering[lastNonZeroEntryInU] = vRowDiagonalIndex;
    _sparseLUFactors._Q._rowOrdering[lastNonZeroEntryInU] = columnIndex;

    _sparseLUFactors._P._rowOrdering[vRowDiagonalIndex] = lastNonZeroEntryInU;
    _sparseLUFactors._Q._columnOrdering[columnIndex] = lastNonZeroEntryInU;

    /*
      Now U's spike is a row instead of a column. We check if it is upper triangular.
      This is done by traversing V's corresponding row
    */

    SparseUnsortedArray::Entry entry;
    const SparseUnsortedArray *sparseRow = _sparseLUFactors._V->getRow( vRowDiagonalIndex );
    bool haveSpike = false;
    for ( unsigned i = 0; i < sparseRow->getNnz(); ++i )
    {
        entry = sparseRow->getByArrayIndex( i );

        unsigned vColumn = entry._index;
        unsigned uColumn = _sparseLUFactors._Q._columnOrdering[vColumn];

        if ( uColumn < lastNonZeroEntryInU )
        {
            haveSpike = true;
            break;
        }
    }

    if ( !haveSpike )
    {
        _sparseLUFactors._vDiagonalElements[vRowDiagonalIndex] = pivotElement;
        return;
    }

    /*
      Step 4:

      Perform Gaussian Elimination on the spike row of U
    */

    SparseEtaMatrix *sparseEtaMatrix = new SparseEtaMatrix( _m, vRowDiagonalIndex );
    // These eta matrices always have 1 as their pivot entry, but this is implicit.

    // Copy the spike row to work memory
    _sparseLUFactors._V->getRowDense( vRowDiagonalIndex, _z3 );

    for ( unsigned i = uColumnIndex; i < lastNonZeroEntryInU; ++i )
    {
        // We are eliminating the i'th column of the spike row

        // This is the location of the pivot element in V
        unsigned vPivotRow = _sparseLUFactors._P._columnOrdering[i];
        unsigned vPivotColumn = _sparseLUFactors._Q._rowOrdering[i];

        // This is the sub diagonal element (being eliminated) in V
        double subDiagonalElement = _z3[vPivotColumn];
        if ( FloatUtils::isZero( subDiagonalElement ) )
            continue;

        sparseRow = _sparseLUFactors._V->getRow( vPivotRow );
        double pivot = sparseRow->get( vPivotColumn );

        // Compute the Gaussian multiplier
        double multiplier = subDiagonalElement / pivot;

        // Store the multiplier in the new eta matrix
        sparseEtaMatrix->addEntry( vPivotRow, multiplier );

        // Adjust the spike row per the elimination step
        for ( unsigned j = 0; j < sparseRow->getNnz(); ++j )
        {
            entry = sparseRow->getByArrayIndex( j );
            unsigned column = entry._index;

            if ( column != vPivotColumn )
            {
                _z3[column] -= multiplier * entry._value;
                if ( FloatUtils::isZero( _z3[column] ) )
                    _z3[column] = 0;
            }
            else
                _z3[column] = 0;
        }
    }

    if ( -GlobalConfiguration::SPARSE_FORREST_TOMLIN_DIAGONAL_ELEMENT_TOLERANCE <
         _z3[columnIndex]
         &&
         _z3[columnIndex] <
         GlobalConfiguration::SPARSE_FORREST_TOMLIN_DIAGONAL_ELEMENT_TOLERANCE )
    {
        obtainFreshBasis();
        return;
    }

    /*
      Step 5:

      U is now upper triangular once more. We need to record the elimination
      step we performed in the eta file
    */
    _etas.append( sparseEtaMatrix );

    /*
      Step 6:

      Finally, copy the (eliminated) spike row back into V and Vt
    */
    _sparseLUFactors._V->updateSingleRow( vRowDiagonalIndex, _z3 );
    for ( unsigned i = 0; i < _m; ++i )
        _sparseLUFactors._Vt->set( i, vRowDiagonalIndex, _z3[i] );

    _sparseLUFactors._vDiagonalElements[vRowDiagonalIndex] = _z3[columnIndex];
}

void SparseFTFactorization::forwardTransformation( const double *y, double *x ) const
{
    /*
      We are solving Bx = y, and we have the factorization:

        B = FHV
    */

    // Eliminate F
    _sparseLUFactors.fForwardTransformation( y, _z1 );

    // Eliminate H
    hForwardTransformation( _z1, _z2 );

    // Eliminate V
    _sparseLUFactors.vForwardTransformation( _z2, x );
}

void SparseFTFactorization::backwardTransformation( const double *y, double *x ) const
{
    /*
      We are solving xB = y, and we have the factorization:

        B = FHV
    */

    // Eliminate V
    _sparseLUFactors.vBackwardTransformation( y, _z1 );

    // Eliminate H
    hBackwardTransformation( _z1, _z2 );

    // Eliminate F
    _sparseLUFactors.fBackwardTransformation( _z2, x );
}

void SparseFTFactorization::clearFactorization()
{
    List<SparseEtaMatrix *>::iterator it;
    for ( it = _etas.begin(); it != _etas.end(); ++it )
        delete *it;

    _etas.clear();
}

void SparseFTFactorization::factorizeBasis()
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

    if ( _statistics )
        _statistics->incLongAttribute( Statistics::NUM_BASIS_REFACTORIZATIONS );
}

void SparseFTFactorization::storeFactorization( IBasisFactorization *other )
{
    SparseFTFactorization *otherSparseFTFactorization = (SparseFTFactorization *)other;

    ASSERT( _m == otherSparseFTFactorization->_m );

    obtainFreshBasis();

    // Store the new basis and factorization
    _sparseLUFactors.storeToOther( &otherSparseFTFactorization->_sparseLUFactors );
}

void SparseFTFactorization::restoreFactorization( const IBasisFactorization *other )
{
    const SparseFTFactorization *otherSparseFTFactorization = (const SparseFTFactorization *)other;

    ASSERT( _m == otherSparseFTFactorization->_m );
    ASSERT( otherSparseFTFactorization->_etas.empty() );

    // Clear any existing data
    clearFactorization();

    // Store the new basis and factorization
    otherSparseFTFactorization->_sparseLUFactors.storeToOther( &_sparseLUFactors );
}

void SparseFTFactorization::invertBasis( double *result )
{
    if ( !_etas.empty() )
        throw BasisFactorizationError( BasisFactorizationError::CANT_INVERT_BASIS_BECAUSE_OF_ETAS );

    ASSERT( result );
    _sparseLUFactors.invertBasis( result );
}

bool SparseFTFactorization::explicitBasisAvailable() const
{
    return _etas.empty() && !_sparseLUFactors._usePForF;
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
    _basisColumnOracle->getSparseBasis( _B );
    factorizeBasis();
}

void SparseFTFactorization::hForwardTransformation( const double *y, double *x ) const
{
    /*
      We have E1 * ... * En * x = y.
      Eliminate etas one by one.
    */

    memcpy( x, y, sizeof(double) * _m );

    for ( const auto &eta : _etas )
    {
        unsigned pivotIndex = eta->_columnIndex;

        for ( const auto &entry : eta->_sparseColumn )
        {
            unsigned entryIndex = entry._index;
            double value = entry._value;
            x[pivotIndex] -= value * x[entryIndex];
        }
    }
}

void SparseFTFactorization::hBackwardTransformation( const double *y, double *x ) const
{
    /*
      We have x * E1 * ... * En = y.
      Eliminate etas one by one.
    */

    memcpy( x, y, sizeof(double) * _m );

    for ( auto eta = _etas.rbegin(); eta != _etas.rend(); ++eta )
    {
        unsigned pivotIndex = (*eta)->_columnIndex;
        double pivotValue = x[pivotIndex];

        for ( const auto &entry : (*eta)->_sparseColumn )
        {
            unsigned entryIndex = entry._index;
            double value = entry._value;
            x[entryIndex] -= value * pivotValue;
        }
    }
}

void SparseFTFactorization::fixPForL()
{
    if ( !_sparseLUFactors._usePForF )
    {
        _sparseLUFactors._usePForF = true;
        _sparseLUFactors._PForF = _sparseLUFactors._P;
    }
}

void SparseFTFactorization::dumpExplicitBasis() const
{
    // The basis is given by:   B = F * H * V
    double *result = new double[_m * _m];
    double *toMultiply = new double[_m * _m];
    double *temp = new double[_m * _m];

    // Start with F
    _sparseLUFactors._F->toDense( result );
    for ( unsigned i = 0; i < _m; ++i )
        result[i*_m + i] = 1;

    // Go eta by eta
    for ( const auto &eta : _etas )
    {
        eta->toMatrix( toMultiply );

        // Etas are transposed
        for ( unsigned i = 0; i < _m; ++i )
        {
            for ( unsigned j = 0; j < _m; ++j )
            {
                temp[i*_m + j] = 0;
                for ( unsigned k = 0; k < _m; ++k )
                {
                    temp[i*_m + j] += ( result[i*_m + k] * toMultiply[j*_m + k] );
                }
            }
        }

        memcpy( result, temp, sizeof(double) * _m * _m );
    }

    // End with V
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

    // Print out the result
    printf( "SparseFTFactorization dumping explicit basis:\n" );
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

void SparseFTFactorization::setStatistics( Statistics *statistics )
{
    _statistics = statistics;
    _sparseGaussianEliminator.setStatistics( statistics );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
