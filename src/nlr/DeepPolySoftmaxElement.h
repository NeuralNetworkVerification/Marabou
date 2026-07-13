/*********************                                                        */
/*! \file DeepPolySoftmaxElement.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#ifndef __DeepPolySoftmaxElement_h__
#define __DeepPolySoftmaxElement_h__

#include "DeepPolyElement.h"
#include "Layer.h"
#include "MStringf.h"
#include "NLRError.h"
#include "SoftmaxBoundType.h"

#include <climits>

namespace NLR {

class DeepPolySoftmaxElement : public DeepPolyElement
{
public:
    DeepPolySoftmaxElement( Layer *layer, unsigned maxLayerSize );
    ~DeepPolySoftmaxElement();

    void execute( const Map<unsigned, DeepPolyElement *> &deepPolyElements );

    void storePredecessorSymbolicBounds();

    void symbolicBoundInTermsOfPredecessor( const double *symbolicLb,
                                            const double *symbolicUb,
                                            double *symbolicLowerBias,
                                            double *symbolicUpperBias,
                                            double *symbolicLbInTermsOfPredecessor,
                                            double *symbolicUbInTermsOfPredecessor,
                                            unsigned targetLayerSize,
                                            DeepPolyElement *predecessor );

private:
    SoftmaxBoundType _boundType;
    unsigned _maxLayerSize;
    double *_work;

    void allocateMemory( unsigned maxLayerSize );
    void freeMemoryIfNeeded();
    void log( const String &message );
};

} // namespace NLR

#endif // __DeepPolySoftmaxElement_h__
