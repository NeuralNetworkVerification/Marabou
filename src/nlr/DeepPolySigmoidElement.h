/*********************                                                        */
/*! \file DeepPolySigmoidElement.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Teruhiro Tagomori
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#ifndef __DeepPolySigmoidElement_h__
#define __DeepPolySigmoidElement_h__

#include "DeepPolyElement.h"
#include "Layer.h"
#include "MStringf.h"
#include "NLRError.h"

#include <climits>

namespace NLR {

class DeepPolySigmoidElement : public DeepPolyElement
{
public:
    DeepPolySigmoidElement( Layer *layer );
    ~DeepPolySigmoidElement();

    void execute( const Map<unsigned, DeepPolyElement *> &deepPolyElementsBefore );

    void symbolicBoundInTermsOfPredecessor( const double *symbolicLb,
                                            const double *symbolicUb,
                                            double *symbolicLowerBias,
                                            double *symbolicUpperBias,
                                            double *symbolicLbInTermsOfPredecessor,
                                            double *symbolicUbInTermsOfPredecessor,
                                            unsigned targetLayerSize,
                                            DeepPolyElement *predecessor );

private:
    void allocateMemory();
    void freeMemoryIfNeeded();
    void log( const String &message );
};

} // namespace NLR

#endif // __DeepPolyElement_h__
