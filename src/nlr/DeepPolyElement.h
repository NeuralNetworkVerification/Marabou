/*********************                                                        */
/*! \file DeepPolyElement.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Andrew Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#ifndef __DeepPolyElement_h__
#define __DeepPolyElement_h__

#include "Layer.h"
#include "Map.h"
#include "MStringf.h"
#include "NLRError.h"
#include <climits>

namespace NLR {

class DeepPolyElement
{
public:
    DeepPolyElement();
    virtual ~DeepPolyElement(){};

    // execute the abstract layer based on the abstract layers topologically
    // before it.
    virtual void execute( const Map<unsigned, DeepPolyElement *>
                          &deepPolyElementsBefore ) = 0;

    /*
      Takes in symbolic bounds of some layer Y in terms of this layer
      (the first six argument), store (in the last four arugment)
      the symbolic bounds of layer Y in terms of the predecessor of this layer
    */
    virtual void symbolicBoundInTermsOfPredecessor
    ( const double *symbolicLbPositiveWeights, const double
      *symbolicLbNegativeWeights, const double *symbolicUbPositiveWeights,
      const double *symbolicUbNegativeWeights, double *symbolicLowerBias,
      double *symbolicUpperBias, double *symbolicLb, double *symbolicUb ) = 0;


    unsigned getPredecessorSize() const;
    unsigned getSize() const;
    unsigned getLayerIndex() const;
    Layer::Type getLayerType() const;
    double *getSymbolicLb() const;
    double *getSymbolicUb() const;
    double *getSymbolicLowerBias() const;
    double *getSymbolicUpperBias() const;
    double getLowerBound( unsigned index ) const;
    double getUpperBound( unsigned index ) const;

protected:
    Layer *_layer;

    // Abstract element described in
    // https://files.sri.inf.ethz.ch/website/papers/DeepPoly.pdf
    double *_symbolicLb;
    double *_symbolicUb;
    double *_symbolicLowerBias;
    double *_symbolicUpperBias;
    double *_lb;
    double *_ub;

    void allocateMemoryForUpperAndLowerBounds();
    void freeMemoryIfNeeded();
    void getConcreteBounds();
};

} // namespace NLR

#endif // __DeepPolyElement_h__
