/*********************                                                        */
/*! \file DeepPolyAnalysis.h
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

#ifndef __DeepPolyAnalysis_h__
#define __DeepPolyAnalysis_h__

#include "DeepPolyElement.h"
#include "Layer.h"
#include "LayerOwner.h"
#include "Map.h"
#include <climits>

namespace NLR {

class DeepPolyAnalysis
{
public:

    DeepPolyAnalysis( LayerOwner *layerOwner );
    ~DeepPolyAnalysis();

    void run( const Map<unsigned, Layer *> &layers );

    void setStartLayerIndex( unsigned startLayerIndex );

    void setEndLayerIndex( unsigned endLayerIndex );

private:
    LayerOwner *_layerOwner;
    Map<unsigned, DeepPolyElement *> _layerIndexTodeepPolyElement;
    double *_workSymbolicLb;
    double *_workSymbolicUb;
    double *_workSymbolicLowerBias;
    double *_workSymbolicUpperBias;

    unsigned _startLayerIndex;
    unsigned _endLayerIndex;

    void allocateMemory();
    void freeMemoryIfNeeded();

    void computeBoundsWithBackSubstitution( DeepPolyElement &deepPolyElement );

    void executeDeepPolyElement( DeepPolyElement &deepPolyElement );
    void executeDeepPolyElementForInput( DeepPolyElement &deepPolyElement );
    void executeDeepPolyElementForWeightedSum( DeepPolyElement &deepPolyElement );
    void executeDeepPolyElementForReLU( DeepPolyElement &deepPolyElement );
};

} // namespace NLR

#endif // __DeepPolyAnalysis_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
