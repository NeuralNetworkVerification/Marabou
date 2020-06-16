/*********************                                                        */
/*! \file SparseFTFactorization.h
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

#ifndef __SparseFTFactorization_h__
#define __SparseFTFactorization_h__

#include "IBasisFactorization.h"
#include "SparseColumnsOfBasis.h"
#include "SparseEtaMatrix.h"
#include "SparseGaussianEliminator.h"
#include "SparseLUFactors.h"
#include "Statistics.h"

#define SFTF_FACTORIZATION_LOG( x... ) LOG( GlobalConfiguration::BASIS_FACTORIZATION_LOGGING, "SparseFTFactorization: %s\n", x )

/*
  This class performs a sparse FT factorization of a given matrix.

  The factorization is of the form:

      A = F * H * V

  Where H represents a list of transposed eta matrices.

  This is an extension of the previous LU facotrization, where A = FV,
  with an extra matrix H that replaces the eta matrices. This factorization
  makes use of the previous LU factorization, but makes the necessary changes.
*/
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
                                const double *newColumn );

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
      Ask the basis factorization to obtain a fresh basis
      (through the previously-provided oracle).
	*/
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
    void dumpExplicitBasis() const;

private:
    /*
      The Basis matrix.
    */
    SparseColumnsOfBasis _B;

    /*
      The extra ForrstTomlin factorization eta matrices
    */
    List<SparseEtaMatrix *> _etas;

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
      An object for reporting statistics
    */
    Statistics *_statistics;

    /*
      Work memory.
    */
    mutable double *_z1;
    mutable double *_z2;
    double *_z3;
    double *_z4;

    /*
      Transformations on the H matrix (the list of etas)
    */
    void hForwardTransformation( const double *y, double *x ) const;
    void hBackwardTransformation( const double *y, double *x ) const;

    /*
      Free any allocated memory.
    */
    void freeIfNeeded();

    /*
      Factorize the stored _B matrix into LU form.
	*/
    void factorizeBasis();

    /*
      Compute the inverse of B0, using the LP factorization already stored.
    */
    void invertBasis( double *result );

    /*
      When moving to adjacent bases, fix the permutation matrix that is used
      for computing L from F in the underlying factorization.
    */
    void fixPForL();

    /*
      Clear a previous factorization.
    */
	void clearFactorization();

    /*
      Have the Basis Factoriaztion object start reporting statistics.
    */
    void setStatistics( Statistics *statistics );
};

#endif // __SparseFTFactorization_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
