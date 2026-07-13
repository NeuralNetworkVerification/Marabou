/*********************                                                        */
/*! \file DeepPolyAnalysis.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Andrew Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
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
    DeepPolyAnalysis( LayerOwner *layerOwner,
                      bool storeOutputSymbolicBounds = false,
                      bool storePredecessorSymbolicBounds = false,
                      bool useParameterisedSBT = false,
                      Map<unsigned, Vector<double>> *layerIndicesToParameters = NULL,
                      Map<unsigned, Vector<double>> *outputSymbolicLb = NULL,
                      Map<unsigned, Vector<double>> *outputSymbolicUb = NULL,
                      Map<unsigned, Vector<double>> *outputSymbolicLowerBias = NULL,
                      Map<unsigned, Vector<double>> *outputSymbolicUpperBias = NULL,
                      Map<unsigned, Vector<double>> *predecessorSymbolicLb = NULL,
                      Map<unsigned, Vector<double>> *predecessorSymbolicUb = NULL,
                      Map<unsigned, Vector<double>> *predecessorSymbolicLowerBias = NULL,
                      Map<unsigned, Vector<double>> *predecessorSymbolicUpperBias = NULL );
    ~DeepPolyAnalysis();

    void run();

private:
    LayerOwner *_layerOwner;
    bool _storeOutputSymbolicBounds;
    bool _storePredecessorSymbolicBounds;
    bool _useParameterisedSBT;
    Map<unsigned, Vector<double>> *_layerIndicesToParameters;

    /*
      Maps layer index to the abstract element
    */
    Map<unsigned, DeepPolyElement *> _deepPolyElements;

    /*
      Working memory for the abstract elements to execute
    */
    double *_work1SymbolicLb;
    double *_work1SymbolicUb;
    double *_work2SymbolicLb;
    double *_work2SymbolicUb;
    double *_workSymbolicLowerBias;
    double *_workSymbolicUpperBias;

    Map<unsigned, Vector<double>> *_outputSymbolicLb;
    Map<unsigned, Vector<double>> *_outputSymbolicUb;
    Map<unsigned, Vector<double>> *_outputSymbolicLowerBias;
    Map<unsigned, Vector<double>> *_outputSymbolicUpperBias;

    Map<unsigned, Vector<double>> *_predecessorSymbolicLb;
    Map<unsigned, Vector<double>> *_predecessorSymbolicUb;
    Map<unsigned, Vector<double>> *_predecessorSymbolicLowerBias;
    Map<unsigned, Vector<double>> *_predecessorSymbolicUpperBias;

    unsigned _maxLayerSize;

    void allocateMemory();
    void freeMemoryIfNeeded();

    DeepPolyElement *createDeepPolyElement( Layer *layer );

    void log( const String &message );
};

} // namespace NLR

#endif // __DeepPolyAnalysis_h__
