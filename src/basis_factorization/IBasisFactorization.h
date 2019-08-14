/*********************                                                        */
/*! \file IBasisFactorization.h
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

#ifndef __IBasisFactorization_h__
#define __IBasisFactorization_h__

class SparseColumnsOfBasis;
class SparseMatrix;
class SparseUnsortedList;
class Statistics;

class IBasisFactorization
{
    /*
      This is the interfact class for a basis factorization.
    */
public:
    /*
      A callback for obtaining columns of the basis matrix
    */
    class BasisColumnOracle
    {
    public:
        virtual ~BasisColumnOracle() {}
        virtual void getColumnOfBasis( unsigned column, double *result ) const = 0;
        virtual void getColumnOfBasis( unsigned column, SparseUnsortedList *result ) const = 0;
        virtual void getSparseBasis( SparseColumnsOfBasis &basis ) const = 0;
    };

    IBasisFactorization( const BasisColumnOracle &basisColumnOracle )
        : _basisColumnOracle( &basisColumnOracle )
    {
    }

    virtual ~IBasisFactorization() {}

    /*
      Inform the basis factorization that the basis has been changed
      by a pivot step. The parameters are:

      1. The index of the column in question
      2. The changeColumn -- this is the so called Eta matrix column
      3. The new explicit column that is being added to the basis

      A basis factorization may make use of just one of the two last
      parameters.
    */
    virtual void updateToAdjacentBasis( unsigned columnIndex,
                                        const double *changeColumn,
                                        const double *newColumn ) = 0;

    /*
      Perform a forward transformation, i.e. find x such that Bx = y.
      Result needs to be of size m.
    */
    virtual void forwardTransformation( const double *y, double *x ) const = 0;

    /*
      Perform a forward transformation, i.e. find x such that xB = y.
      Result needs to be of size m.
    */
    virtual void backwardTransformation( const double *y, double *x ) const = 0;

    /*
      Store/restore the basis factorization.
    */
    virtual void storeFactorization( IBasisFactorization *other ) = 0;
    virtual void restoreFactorization( const IBasisFactorization *other ) = 0;

	/*
      Ask the basis factorization to obtain a fresh basis
      (through the previously-provided oracle).
	*/
    virtual void obtainFreshBasis() = 0;

    /*
      Return true iff the basis matrix B is explicitly available.
    */
    virtual bool explicitBasisAvailable() const = 0;

    /*
      Make the basis explicitly available
    */
    virtual void makeExplicitBasisAvailable() = 0;

    /*
      Get the explicit basis matrix
    */
    virtual const double *getBasis() const = 0;
    virtual const SparseMatrix *getSparseBasis() const = 0;

    /*
      Compute the inverse of B (should only be called when B is explicitly available).
     */
    virtual void invertBasis( double *result ) = 0;

    /*
      Have the Basis Factoriaztion object start reporting statistics.
    */
    virtual void setStatistics( Statistics * ) {};

    /*
      For debugging
    */
    virtual void dump() const {};

protected:
    const BasisColumnOracle *_basisColumnOracle;
};

#endif // __IBasisFactorization_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
