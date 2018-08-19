/*********************                                                        */
/*! \file SparseLUFactorization.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __SparseFTFactorization_h__
#define __SparseFTFactorization_h__

#include "SparseGaussianEliminator.h"
#include "IBasisFactorization.h"
#include "SparseLUFactors.h"

class SparseFTFactorization : public IBasisFactorization
{
public:
    SparseFTFactorization( unsigned m, const BasisColumnOracle &basisColumnOracle );
    ~SparseFTFactorization();

    /*
      Inform the basis factorization that the basis has been changed
      by a pivot step. The parameters are:

      1. The index of the column in question
      2. The changeColumn -- this is the so called Eta matrix column
      3. The new explicit column that is being added to the basis

      A basis factorization may make use of just one of the two last
      parameters.
    */
    void updateToAdjacentBasis( unsigned columnIndex,
                                const double *changeColumn,
                                const SparseVector *newColumn );

    /*
      Perform a forward transformation, i.e. find x such that x = inv(B) * y,
      The solution is found by solving Bx = y.
    */
    void forwardTransformation( const double *y, double *x ) const;

    /*
      Perform a backward transformation, i.e. find x such that x = y * inv(B),
      The solution is found by solving xB = y.
    */
    void backwardTransformation( const double *y, double *x ) const;

    /*
      Store and restore the basis factorization.
    */
    void storeFactorization( IBasisFactorization *other );
    void restoreFactorization( const IBasisFactorization *other );

	/*
      Set B to a non-identity matrix (or have it retrieved from the oracle),
      and then factorize it.
	*/
	void setBasis( const double *B );
    void obtainFreshBasis();

    /*
      Return true iff the basis matrix B0 is explicitly available.
    */
    bool explicitBasisAvailable() const;

    /*
      Make the basis explicitly available
    */
    void makeExplicitBasisAvailable();

    /*
      Get the explicit basis matrix
    */
    const double *getBasis() const;
    const SparseMatrix *getSparseBasis() const;

    /*
      Debug
    */
    void dump() const;

private:
    /*
      The Basis matrix.
    */
    SparseMatrix *_B;

    /*
      The extra ForrstTomlin factorization matrix
    */
    SparseMatrix *_H;

    /*
      The dimension of the basis matrix.
    */
    unsigned _m;

    /*
      The LU factors of B.
    */
    SparseLUFactors _sparseLUFactors;

    /*
      The Gaussian eliminator, to compute basis factorizations
    */
    SparseGaussianEliminator _sparseGaussianEliminator;

    /*
      Work memory.
    */
    mutable double *_z;

    /*
      Free any allocated memory.
    */
    void freeIfNeeded();

    /*
      Factorize the stored _B matrix into LU form.
	*/
    void factorizeBasis();

	/*
      Swap two rows of a matrix.
    */
    void rowSwap( unsigned rowOne, unsigned rowTwo, double *matrix );

    /*
      Compute the inverse of B0, using the LP factorization already stored.
    */
    void invertBasis( double *result );

    /*
      Clear a previous factorization.
    */
	void clearFactorization();

    static void log( const String &message );
};

#endif // __SparseFTFactorization_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
