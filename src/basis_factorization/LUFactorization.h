/*********************                                                        */
/*! \file LUFactorization.h
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

#ifndef __LUFactorization_h__
#define __LUFactorization_h__

#include "IBasisFactorization.h"
#include "LPElement.h"
#include "List.h"
#include "MString.h"

class EtaMatrix;
class LPElement;

class LUFactorization : public IBasisFactorization
{
public:
    LUFactorization( unsigned m, const BasisColumnOracle &basisColumnOracle );
    ~LUFactorization();

    /*
      Free any allocated memory.
    */
    void freeIfNeeded();

    /*
      Adds a new eta matrix to the basis factorization. The matrix is
      the identity matrix with the specified column replaced by the one
      provided. If the number of stored eta matrices exceeds a certain
      threshold, re-factorization may occur.
    */
    void pushEtaMatrix( unsigned columnIndex, const double *column );

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

      For now, assume that B0 = I, so we start with u0 = y.

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

      For now, assume that B0 = I, so we start with u0 = x.

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
      Factorize a matrix into LU form. The resuling upper triangular
      matrix is stored in _U and the lower triangular and permutation matrices
      are stored in _LP.
	*/
    void factorizeMatrix( double *matrix );

	/*
      Set B0 to a non-identity matrix (or have it retrieved from the oracle),
      and then factorize it.
	*/
	void setBasis( const double *B );
    void refactorizeBasis();

	/*
      Swap two rows of a matrix.
    */
    void rowSwap( unsigned rowOne, unsigned rowTwo, double *matrix );

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

    /*
      Compute the inverse of B0, using the LP factorization already stored.
      This can only be done when B0 is "fresh", i.e. when there are no stored etas.
     */
    void invertBasis( double *result );


public:
    /*
      Functions made public strictly for testing, not part of the interface
    */

    /*
      Getter functions for the various factorization components.
    */
	const double *getU() const;
	const List<LPElement *> getLP() const;
	const List<EtaMatrix *> getEtas() const;

    /*
      Debug
    */
    void dump() const;

private:
    /*
      The Basis matrix.
    */
	double *_B0;

    /*
      The dimension of the basis matrix.
    */
    unsigned _m;

    /*
      The LU factorization on B0. U is upper triangular, and LP is a
      sequence of lower triangular eta matrices and permuatation
      pairs.
    */
	double *_U;
	List<LPElement *> _LP;

    /*
      A sequence of eta matrices.
    */
    List<EtaMatrix *> _etas;

    /*
      Working space
    */
    double *_tempY;
    double *_LCol;

    /*
      Clear a previous factorization.
    */
	void clearFactorization();

    /*
      Helper functions for backward- and forward-transformations.
      Compute L*X or X*L, where X is vector of length m and L is an (m x m)
      lower triangular eta matrix.
    */
	void LMultiplyLeft( const EtaMatrix *L, double *X ) const;
    void LMultiplyRight( const EtaMatrix *L, double *X ) const;

	/*
      Multiply matrix U on the left by lower triangular eta matrix L,
      store result in U.
    */
	void LFactorizationMultiply( const EtaMatrix *L );

    static void log( const String &message );
};

#endif // __LUFactorization_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
