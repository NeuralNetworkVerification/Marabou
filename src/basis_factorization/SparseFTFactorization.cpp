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
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "MalformedBasisException.h"
#include "SparseFTFactorization.h"

SparseFTFactorization::SparseFTFactorization( unsigned m, const BasisColumnOracle &basisColumnOracle )
    : IBasisFactorization( basisColumnOracle )
	, _m( m )
    , _sparseLUFactors( m )
    , _sparseGaussianEliminator( m )
    , _z1( NULL )
    , _z2( NULL )
    , _z3( NULL )
    , _z4( NULL )
{
    _B = new CSRMatrix;
    if ( !_B )
        throw BasisFactorizationError( BasisFactorizationError::ALLOCATION_FAILED, "SparseFTFactorization::B" );
    _B->initializeToEmpty( m, m );

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

	if ( _B )
	{
		delete _B;
		_B = NULL;
	}

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
	return _B;
}

void SparseFTFactorization::updateToAdjacentBasis( unsigned columnIndex,
                                                   const double */* changeColumn */,
                                                   const double *newColumn )
{

    // q = columnIndex
    // s = uColumnIndex
    // p = vRowDiagonalIndex
    // t = lastNonZeroEntryInU

    static unsigned count = 1;
    printf( "\n\n******" );
    printf( "updateToAdjacentBasis starting (%u), dumping current state\n", count++ );
    _sparseLUFactors.dump();
    printf( "\nnumber of etas: %u\n", _etas.size() );
    printf( "New column %u being pushed:\n", columnIndex );
    for ( unsigned i = 0; i < _m; ++i )
    {
        printf( "\tcol[%u] = %lf\n", i, newColumn[i] );
    }


    printf( "******\n\n" );



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
    unsigned uColumnIndex = _sparseLUFactors._Q._columnOrdering[columnIndex]; // this is "s"
    unsigned vRowDiagonalIndex = _sparseLUFactors._P._columnOrdering[uColumnIndex]; // this is "p"

    // Compute the new column of V: inv( FH ) * newColumn
    _sparseLUFactors.fForwardTransformation( newColumn, _z3 );
    hForwardTransformation( _z3, _z4 );

    // Replace this column of V in the sparse factors
    // Also find the index of the last non-zero entry in this column, for U
    unsigned lastNonZeroEntryInU = 0;       // this is "t"
    // double diagonalElement;
    for ( unsigned i = 0; i < _m; ++i )
    {
        if ( !FloatUtils::isZero( _z4[i] ) )
        {
            unsigned uRow = _sparseLUFactors._P._rowOrdering[i];
            if ( uRow > lastNonZeroEntryInU )
                lastNonZeroEntryInU = uRow;
        }

        // if ( i == vDiagonalIndex )
        // {
            // _sparseLUFactors._V->commitChange( i, columnIndex, 0 );
            // _sparseLUFactors._Vt->commitChange( columnIndex, i, 0 );
            // diagonalElement = _z4[i];
            // continue;
        // }

        _sparseLUFactors._V->commitChange( i, columnIndex, _z4[i] );
        _sparseLUFactors._Vt->commitChange( columnIndex, i, _z4[i] );
    }

    // TODO: Added these for debug, but can keep them here?
    _sparseLUFactors._V->executeChanges();
    _sparseLUFactors._Vt->executeChanges();
    // done TODO

    printf( "Dumping after step 1: V's column has been changed\n" );
    _sparseLUFactors.dump();

    /*
      Step 2:

      The column of V has been replaced, and consequently so has the column of U.
      If U is upper traingular, we are done.
    */

    ASSERT( lastNonZeroEntryInU > 0 );
    if ( lastNonZeroEntryInU <= uColumnIndex )
    {
        // No spike, just store the diagonal element and be done
        // ASSERT( !FloatUtils::isZero( diagonalElement ) );
        // _sparseLUFactors._V->commitChange( vDiagonalIndex, columnIndex, diagonalElement );
        // _sparseLUFactors._Vt->commitChange( columnIndex, vDiagonalIndex, diagonalElement );

        _sparseLUFactors._V->executeChanges();
        _sparseLUFactors._Vt->executeChanges();
        return;
    }

    // We have a spike, execute the changes from before
    _sparseLUFactors._V->executeChanges();
    _sparseLUFactors._Vt->executeChanges();

    printf( "Step 2: U has a spike, so we're not done yet\n" );

    /*
      Step 3:

      Perform permutations of matrix U, to move its spike from being a
      column to being a row - so that we can afterwards eliminate it.
    */

    /*
*  The routine makes matrix U upper triangular as follows. First, it
*  moves rows and columns s+1, ..., t by one position to the left and
*  upwards, resp., and moves s-th row and s-th column to position t.
*  Due to such symmetric permutations matrix U becomes the following
*  (note that all diagonal elements remain on the diagonal, and element
*  u[s,s] becomes u[t,t]):

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

    printf( "Step 3: After permuting U, the spike should be a row and not a column\n" );
    _sparseLUFactors.dump();

    /*
      Now U's spike is a row instead of a column. We check if it is upper triangular.
      This is done by traversing V's corresponding row
    */

    SparseVector sparseRow( _m );
    _sparseLUFactors._V->getRow( vRowDiagonalIndex, &sparseRow );
    bool haveSpike = false;
    for ( unsigned i = 0; i < sparseRow.getNnz(); ++i )
    {
        unsigned vColumn = sparseRow.getIndexOfEntry( i );
        unsigned uColumn = _sparseLUFactors._Q._columnOrdering[vColumn];

        if ( uColumn < lastNonZeroEntryInU )
        {
            haveSpike = true;
            break;
        }
    }

    if ( !haveSpike )
    {
        // U is upper triangular. Store the diagonal element and be done
        // ASSERT( !FloatUtils::isZero( diagonalElement ) );
        // _sparseLUFactors._V->commitChange( vDiagonalIndex, columnIndex, diagonalElement );
        // _sparseLUFactors._Vt->commitChange( columnIndex, vDiagonalIndex, diagonalElement );

        // _sparseLUFactors._V->executeChanges();
        // _sparseLUFactors._Vt->executeChanges();
        printf( "Step 3: permuted U does not have a spike, so we're done!\n" );
        return;

    }

    printf( "Step 3: U still has a spike, so we continue\n" );

    /*
      Step 4:

      Perform Gaussian Elimination on the spike row of U
    */

    // q = columnIndex
    // s = uColumnIndex
    // p = vRowDiagonalIndex
    // t = lastNonZeroEntryInU

    SparseEtaMatrix *sparseEtaMatrix = new SparseEtaMatrix( _m, vRowDiagonalIndex );

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

        _sparseLUFactors._V->getRow( vPivotRow, &sparseRow );
        double pivot = sparseRow.get( vPivotColumn );

        // Compute the Gaussian multiplier
        double multiplier = subDiagonalElement / pivot;

        // Store the multiplier in the new eta matrix
        sparseEtaMatrix->commitChange( vPivotColumn, multiplier );

        // Adjust the spike row per the elimination step
        for ( unsigned j = 0; j < sparseRow.getNnz(); ++j )
        {
            unsigned column = sparseRow.getIndexOfEntry( j );

            if ( column != vPivotColumn )
                _z3[column] -= multiplier * sparseRow.getValueOfEntry( j );
            else
                _z3[column] = 0;
        }
    }

    /*
      Step 5:

      U is now upper triangular once more. We need to record the elimination
      step we performed in the eta file
    */

    sparseEtaMatrix->executeChanges();
    _etas.append( sparseEtaMatrix );
    //    _etas.appendHead( sparseEtaMatrix );

    printf( "\nStep 4 + 5: Came up with the following eta matrix to kill the spike:\n" );
    sparseEtaMatrix->dumpDenseTransposed();

    /*
      Step 6:

      Finally, copy the (eliminated) spike row back into V
    */

    for ( unsigned i = 0; i < _m; ++i )
    {
        _sparseLUFactors._V->commitChange( vRowDiagonalIndex, i, _z3[i] );
        _sparseLUFactors._Vt->commitChange( i, vRowDiagonalIndex, _z3[i] );
    }
    _sparseLUFactors._V->executeChanges();
    _sparseLUFactors._Vt->executeChanges();

    printf( "\nStep 6: Dumping reformed factorization\n" );
    _sparseLUFactors.dump();
}

void SparseFTFactorization::setBasis( const double *B )
{
    _B->initialize( B, _m, _m );
	factorizeBasis();
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

    obtainFreshBasis();

    // Store the new basis and factorization
    _B->storeIntoOther( otherSparseFTFactorization->_B );
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
    otherSparseFTFactorization->_B->storeIntoOther( _B );
    otherSparseFTFactorization->_sparseLUFactors.storeToOther( &_sparseLUFactors );
}

void SparseFTFactorization::invertBasis( double *result )
{
    if ( !_etas.empty() )
        throw BasisFactorizationError( BasisFactorizationError::CANT_INVERT_BASIS_BECAUSE_OF_ETAS );

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
    return _etas.empty();
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
        double pivotValue;

        for ( unsigned i = 0; i < eta->_sparseColumn.getNnz(); ++i )
        {
            unsigned entryIndex = eta->_sparseColumn.getIndexOfEntry( i );
            double value = eta->_sparseColumn.getValueOfEntry( i );

            if ( entryIndex == pivotIndex )
                pivotValue = value;
            else
            {
                x[pivotIndex] -= value * x[entryIndex];
                if ( FloatUtils::isZero( x[pivotIndex] ) )
                    x[pivotIndex] = 0.0;
            }
        }

        x[pivotIndex] = ( x[pivotIndex] / pivotValue );
        if ( FloatUtils::isZero( x[pivotIndex] ) )
            x[pivotIndex] = 0.0;
    }
}

void SparseFTFactorization::hBackwardTransformation( const double *y, double *x ) const
{
    memcpy( x, y, sizeof(double) * _m );

    // printf( "hBTrans starting:\n" );
    // printf( "Y:\n" );
    // for ( unsigned i = 0; i < _m; ++i )
    // {
    //     printf( "\tY[%u] = %lf\n", i, y[i] );
    // }

    for ( auto eta = _etas.rbegin(); eta != _etas.rend(); ++eta )
    {
        // printf( "Working on Eta:\n" );
        // (*eta)->dump();

        unsigned pivotIndex = (*eta)->_columnIndex;
        x[pivotIndex] = x[pivotIndex] / (*eta)->_sparseColumn.get( pivotIndex );
        double adjustedPivotValue = x[pivotIndex];

        ASSERT( !FloatUtils::isZero( adjustedPivotValue ) );

        for ( unsigned i = 0; i < (*eta)->_sparseColumn.getNnz(); ++i )
        {
            unsigned entryIndex = (*eta)->_sparseColumn.getIndexOfEntry( i );
            double value = (*eta)->_sparseColumn.getValueOfEntry( i );

            if ( entryIndex == pivotIndex )
                continue;
            else
            {
                x[entryIndex] -= value * adjustedPivotValue;
                if ( FloatUtils::isZero( x[entryIndex] ) )
                    x[entryIndex] = 0.0;
            }
        }

        // printf( "X after eta:\n" );
        // for ( unsigned i = 0; i < _m; ++i )
        // {
        //     printf( "\tx[%u] = %lf\n", i, x[i] );
        // }
    }
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
