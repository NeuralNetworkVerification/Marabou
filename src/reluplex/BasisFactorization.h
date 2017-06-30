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

class EtaMatrix;

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
      provided.
    */
    void pushEtaMatrix( unsigned columnIndex, double *column );

    /* Perform a forward transformation, i.e. find x such that x = inv(B) * y,
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

      Result needs to be of size m */
    void forwardTransformation( const double *y, double *x );

    /* Perform a backward transformation, i.e. find x such that x = y * inv(B),
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

      Result needs to be of size m */
    void backwardTransformation( const double *y, double *x );

    /*
      Store and restore the basis factorization.
    */
    void storeFactorization( BasisFactorization *other ) const;
    void restoreFactorization( const BasisFactorization *other );

private:
    unsigned _m;
    double *_B0;
    List<EtaMatrix *> _etas;
};

#endif // __BasisFactorization_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
