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
    virtual void execute( Map<unsigned, DeepPolyElement *> deepPolyElements ) = 0;

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
    unsigned _layerIndex;
    unsigned _predecessorSize;
    unsigned _size;

    // Abstract element described in
    // https://files.sri.inf.ethz.ch/website/papers/DeepPoly.pdf
    double *_symbolicLb;
    double *_symbolicUb;
    double *_symbolicLowerBias;
    double *_symbolicUpperBias;
    double *_lb;
    double *_ub;
};

} // namespace NLR

#endif // __DeepPolyElement_h__
