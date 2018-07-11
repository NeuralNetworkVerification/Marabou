/*********************                                                        */
/*! \file IBasisFactorization.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __IBasisFactorization_h__
#define __IBasisFactorization_h__

class SparseVector;

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
        virtual void getColumnOfBasis( unsigned column, SparseVector *result ) const = 0;
    };

    IBasisFactorization( const BasisColumnOracle &basisColumnOracle )
        : _factorizationEnabled( true )
        , _basisColumnOracle( &basisColumnOracle )
    {
    }

    virtual ~IBasisFactorization() {}

    /*
      Inform the basis factorization that the basis has been changed
      by a pivot step. This results is an eta matrix by which the
      basis is multiplied on the right. This eta matrix is represented
      by the column index and column vector.
    */
    virtual void pushEtaMatrix( unsigned columnIndex, const double *column ) = 0;

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
      Set the basis matrix, or ask the basis factorization to obtain it
      itself (through the previously-provided oracle).
	*/
    virtual void setBasis( const double *B ) = 0;
    virtual void obtainFreshBasis() = 0;

    /*
      Control/check whether factorization is enabled.
    */
    bool factorizationEnabled() const
    {
        return _factorizationEnabled;
    }

    void toggleFactorization( bool value )
    {
        _factorizationEnabled = value;
    }

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

    /*
      Compute the inverse of B (should only be called when B is explicitly available).
     */
    virtual void invertBasis( double *result ) = 0;

private:
    /*
      A flag that controls whether factorization is enabled or
      disabled.
    */
    bool _factorizationEnabled;

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
