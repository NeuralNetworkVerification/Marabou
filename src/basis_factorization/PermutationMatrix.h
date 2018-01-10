/*********************                                                        */
/*! \file PermutationMatrix.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __PermutationMatrix_h__
#define __PermutationMatrix_h__

#include "List.h"

class PermutationMatrix
{
    /*
      The identity matrix with its rows in a different order
    */
public:
    PermutationMatrix( unsigned m );

private:
    /*
      The dimension of the matrix
    */
    unsigned _m;

    /*
      The diagonal entries
    */
    List<unsigned> _ordering;
};

#endif // __PermutationMatrix_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
