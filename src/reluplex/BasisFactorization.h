/*********************                                                        */
/*! \file BasisFactorization.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __BasisFactorization_h__
#define __BasisFactorization_h__

#include "List.h"
#include <queue>
#include "LPElement.h"

using std::queue;

class EtaMatrix;

class LPElement;

class BasisFactorization
{
public:
    BasisFactorization( unsigned m );
    ~BasisFactorization();

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
    void pushEtaMatrix( unsigned columnIndex, double *column );

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

      Result needs to be of size m
    */
    void forwardTransformation( const double *y, double *x );

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

      Result needs to be of size m
    */
    void backwardTransformation( const double *y, double *x );

    /*
      Store and restore the basis factorization.
    */
    void storeFactorization( BasisFactorization *other ) const;
    void restoreFactorization( const BasisFactorization *other );

	/*
      Factorize a matrix into LU form. The resuling upper triangular
      matrix is stored in _U and the lower triangular and permutation matrices
      are stored in _LP.
	*/
    void factorizeMatrix( double *matrix );

	/*
      For the purposes of testing: here we set B0 to a non-identity matrix and set the _start flag to false,
      thus testing the LP and U multiplications
	*/
	void setB0( const double *B0 ); //testing

	// Swap two rows of a matrix
    void rowSwap( unsigned rowOne, unsigned rowTwo, double *matrix );

	// void coutMatrix ( const double *m );

    /*
      Getter functions for the various factorization components.
    */
	const double *getU() const;
	const List<LPElement *> getLP() const;
    //    	const std::vector<LPElement *> getLP() const;
	const double *getB0() const;
	const List<EtaMatrix *> getEtas() const;

    /*
      Check/set whether factorization is enabled.
    */
    bool factorizationEnabled() const;
    void toggleFactorization( bool value );

    /*
      A helper function for matrix multiplication. left * right =
      result
    */
    static void matrixMultiply( unsigned dimension, const double *left, const double *right, double *result );

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
      A flag that controls whether LU-factorization is enabled or
      disabled.
    */
    bool _factorizationEnabled;

    /*
      Clear a previous factorization.
    */
	void clearLPU();

	//LB = R, where B is a _m x 1 matrix and L represents a _m x _m lower triangular matrix
	void LMultiplyLeft( const EtaMatrix *L, double *B );
	//BL = R, where B is a 1 x _m matrix and L represents a _m x _m lower triangular matrix
	void LMultiplyRight( const EtaMatrix *L, double *B );

	/*
      Multiply matrix U on the left by lower triangular eta matrix L,
      store result in U.
    */
	void LFactorizationMultiply( const EtaMatrix *L );

    /*
      Compute B0 * E1 ... *En for all stored eta matrices, and place
      the result in B0.
    */
	void condenseEtas();
};

#endif // __BasisFactorization_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
