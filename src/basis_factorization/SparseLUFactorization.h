/*********************                                                        */
/*! \file SparseLUFactorization.h
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

#ifndef __SparseLUFactorization_h__
#define __SparseLUFactorization_h__

#include "IBasisFactorization.h"
#include "List.h"
#include "SparseGaussianEliminator.h"
#include "SparseLUFactors.h"

#define BASIS_FACTORIZATION_LOG( x, ... ) LOG( GlobalConfiguration::BASIS_FACTORIZATION_LOGGING, "SparseLUFactorization: %s\n", x )

class EtaMatrix;
class LPElement;

class SparseLUFactorization : public IBasisFactorization
{
public:
    SparseLUFactorization( unsigned m, const BasisColumnOracle &basisColumnOracle );
    ~SparseLUFactorization();

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

      Bx = (B0 * E1 * E2 ... * En) x = B0 * ( E1 ( ... ( En * x ) ) ) = y
                                                        -- u_n --
                                                 ----- u_1 ------
                                            ------- u_0 ---------

      And the equation is solved iteratively:
      B0     * u0   =   y  --> obtain u0
      E1     * u1   =  u0  --> obtain u1
      ...
      En     * x    =  un  --> obtain x

      Result needs to be of size m.
    */
    void forwardTransformation( const double *y, double *x ) const;

    /*
      Perform a backward transformation, i.e. find x such that x = y * inv(B),
      The solution is found by solving xB = y.

      xB = x (B0 * E1 * E2 ... * En) = ( ( ( x B0 ) * E1 ... ) En ) = y
                                            ------- u_n ---------
                                            --- u_1 ----
                                            - u_0 -

      And the equation is solved iteratively:
      u_n-1  * En   =  y   --> obtain u_n-1
      ...
      u1     * E2   =  u2  --> obtain u1
      u0     * E1   =  u1  --> obtain u0

      Result needs to be of size m.
    */
    void backwardTransformation( const double *y, double *x ) const;

    /*
      Store and restore the basis factorization. Storing triggers
      condesning the etas.
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

public:
    /*
      Functions made public strictly for testing, not part of the interface
    */

    /*
      Getter functions for the various factorization components.
    */
	const List<EtaMatrix *> getEtas() const;

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
      The dimension of the basis matrix.
    */
    unsigned _m;

    /*
      The LU factors of B.
    */
    SparseLUFactors _sparseLUFactors;

    /*
      A sequence of eta matrices.
    */
    List<EtaMatrix *> _etas;

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
      Compute the inverse of B0, using the LP factorization already stored.
      This can only be done when B0 is "fresh", i.e. when there are no stored etas.
     */
    void invertBasis( double *result );

    /*
      Clear a previous factorization.
    */
	void clearFactorization();
};

#endif // __SparseLUFactorization_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
