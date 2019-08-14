/*********************                                                        */
/*! \file PermutationMatrix.h
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
    ~PermutationMatrix();
    PermutationMatrix &operator=( const PermutationMatrix &other );

    /*
      Reset the permutation to the identity permutation, or check
      if the permutation is the identity
    */
    void resetToIdentity();
    bool isIdentity() const;

    /*
      Change the permutation so that two rows/columns are swapped
    */
    void swapRows( unsigned a, unsigned b );
    void swapColumns( unsigned a, unsigned b );

    /*
      Produce the inverse of the permutation matrix
    */
    PermutationMatrix *invert() const;
    void invert( PermutationMatrix &inv ) const;

    /*
      Return the index of an identity row in this
      permuation matrix.
    */
    unsigned findIndexOfRow( unsigned row ) const;

    /*
      Return the matrix size
    */
    unsigned getM() const;

    /*
      Clone the permutation matrix
    */
    void storeToOther( PermutationMatrix *other ) const;

    /*
      The row and column permutation orderings
      _rowOrdering[i] = j implies that entry [i,j] is 1,
      and _columnOrdering[j] = i is equivalent
    */
    unsigned *_rowOrdering;
    unsigned *_columnOrdering;

    /*
      Debugging
    */
    void dump() const;

private:
    /*
      The dimension of the matrix
    */
    unsigned _m;
};

#endif // __PermutationMatrix_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
