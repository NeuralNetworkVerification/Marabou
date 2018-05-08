/*********************                                                        */
/*! \file ForrestTomlinFactorization.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __ForrestTomlinFactorization_h__
#define __ForrestTomlinFactorization_h__

#include "AlmostIdentityMatrix.h"
#include "IBasisFactorization.h"
#include "LPElement.h"
#include "List.h"
#include "PermutationMatrix.h"

/*
  Forrest-Tomlin factorization looks like this:

  Ak...A1 * LsPs...L1P1 * B = Q * Um...U1 * inv(Q)

  Where:
  - The A matrices are "almost-diagonal", i.e. diagonal matrices
  with one extra entry off the diagonal.
  - The L and U matrices are as in a usual LU factorization
  - Q (and inv(Q)) are permutation matrices
*/
class ForrestTomlinFactorization : public IBasisFactorization
{
public:
    ForrestTomlinFactorization( unsigned m );
    ~ForrestTomlinFactorization();

    /*
      Inform the basis factorization that the basis has been changed
      by a pivot step. This results is an eta matrix by which the
      basis is multiplied on the right. This eta matrix is represented
      by the column index and column vector.
    */
    void pushEtaMatrix( unsigned columnIndex, const double *column );

    /*
      Perform a forward transformation, i.e. find x such that Bx = y.
      Result needs to be of size m.
    */
    void forwardTransformation( const double *y, double *x ) const;

    /*
      Perform a forward transformation, i.e. find x such that xB = y.
      Result needs to be of size m.
    */
    void backwardTransformation( const double *y, double *x ) const;

    /*
      Store/restore the basis factorization.
    */
    void storeFactorization( IBasisFactorization *other );
    void restoreFactorization( const IBasisFactorization *other );

	/*
      Set the basis matrix. This clears the previous factorization,
      and triggers a factorization into LU format of the matrix.
    */
	void setBasis( const double *B );

    /*
      Return true iff the basis matrix B is explicitly available.
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
      Compute the inverse of B (should only be called when B is explicitly available).
     */
    void invertBasis( double *result );

public:
    /*
      For testing purposes only
    */
    const PermutationMatrix *getQ() const;
    const PermutationMatrix *getInvQ() const;
    const EtaMatrix **getU() const;
    const List<LPElement *> *getLP() const;
    const List<AlmostIdentityMatrix *> *getA() const;
    const double *getInvLP() const;

    void pushA( const AlmostIdentityMatrix &matrix );
    void setQ( const PermutationMatrix &Q );

private:
    /*
      The dimension of the basis matrix.
    */
    unsigned _m;

    /*
      The current basis matrix B
    */
	double *_B;

    /*
      The components of the Forrest-Tomlin factorization.
    */
    PermutationMatrix _Q;
    PermutationMatrix _invQ;
    EtaMatrix **_U;
    // P1 is the last element of the list
    List<LPElement *> _LP;
    // A1 is the first element of the list
    List<AlmostIdentityMatrix *>_A;

    /*
      A helper matrix to facilitate computing the explicit basis.
      This matrix stores:
      inv(P1)inv(L1) ... inv(Ps)inv(LS)
    */
    double *_invLP;

    /*
      Indicates whether the explicit basis matrix is available.
    */
    bool _explicitBasisAvailable;

    /*
      Work memory
    */
    double *_workMatrix;
    double *_workVector;
    double *_workW;
    PermutationMatrix _workQ;
    PermutationMatrix _invWorkQ;
    unsigned *_workOrdering;

    /*
      After a new basis matrix is set, initialize the LU factorization
    */
    void clearFactorization();
    void initialLUFactorization();

	/*
      Swap two rows of a matrix.
    */
    void rowSwap( unsigned rowOne, unsigned rowTwo, double *matrix );

public:
    /*
      Debug
    */
    void dump() const;
    void dumpU() const;
};

#endif // __ForrestTomlinFactorization_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
