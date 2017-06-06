/*********************                                                        */
/*! \file BasisFactorization.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Reluplex project.
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

    /* Adds a new eta matrix to the basis factorization. The matrix is
    the identity matrix with the specified column replaced by the one
    provided */
    void pushEtaMatrix( unsigned columnIndex, double *column );

    /* Perform a forward transformation, i.e. find d such that d = inv(B) * a,
       The solution is found by solving Bd = a.

       Bd = (B0 * E1 * E2 ... * En) d = B0 * ( E1 ( ... ( En * d ) ) ) = a
                                                         -- u_n --
                                                  ----- u_1 ------
                                             ------- u_0 ---------

      And the equation is solved iteratively:
      B0     * u0   =   a  --> obtain u0
      E1     * u1   =  u0  --> obtain u1
      ...
      En     * d    =  un  --> obtain d

      For now, assume that B0 = I, so we start with u0 = a.

      Result needs to be of size m */
    void forwardTransformation( const double *a, double *result );

private:
    unsigned _m;
    double *_B0;
    List<EtaMatrix *> _etas;
};

#endif // __BasisFactorization_h__

//
// Local Variables:
// compile-command: "make -C . "
// tags-file-name: "./TAGS"
// c-basic-offset: 4
// End:
//
