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
    void symbolicBoundInTermsOfPredecessor( const double *symbolicLb,
                                            const double *symbolicUb,
                                            double *symbolicLowerBias,
                                            double *symbolicUpperBias,
                                            double *symbolicLbInTermsOfPredecessor,
                                            double *symbolicUbInTermsOfPredecessor,
                                            unsigned targetLayerSize,
                                            DeepPolyElement *predecessor );

    // The following methods compute concrete softmax output bounds
    // using different linear approximation, as well as the coefficients
    // of softmax inputs in the symbolic bounds
    static double LSELowerBound( const Vector<double> &sourceMids,
                                 const Vector<double> &inputLbs,
                                 const Vector<double> &inputUbs,
                                 unsigned outputIndex );
    static double dLSELowerBound( const Vector<double> &sourceMids,
                                  const Vector<double> &inputLbs,
                                  const Vector<double> &inputUbs,
                                  unsigned outputIndex,
                                  unsigned inputIndex );
    static double LSELowerBound2( const Vector<double> &sourceMids,
                                  const Vector<double> &inputLbs,
                                  const Vector<double> &inputUbs,
                                  unsigned outputIndex );
    static double dLSELowerBound2( const Vector<double> &sourceMids,
                                   const Vector<double> &inputLbs,
                                   const Vector<double> &inputUbs,
                                   unsigned outputIndex,
                                   unsigned inputIndex );
    static double LSEUpperBound( const Vector<double> &sourceMids,
                                 const Vector<double> &outputLb,
                                 const Vector<double> &outputUb,
                                 unsigned outputIndex );
    static double dLSEUpperbound( const Vector<double> &sourceMids,
                                  const Vector<double> &outputLb,
                                  const Vector<double> &outputUb,
                                  unsigned outputIndex,
                                  unsigned inputIndex );
    static double ERLowerBound( const Vector<double> &sourceMids,
                                const Vector<double> &inputLbs,
                                const Vector<double> &inputUbs,
                                unsigned outputIndex );
    static double dERLowerBound( const Vector<double> &sourceMids,
                                 const Vector<double> &inputLbs,
                                 const Vector<double> &inputUbs,
                                 unsigned outputIndex,
                                 unsigned inputIndex );
    static double ERUpperBound( const Vector<double> &sourceMids,
                                const Vector<double> &outputLbs,
                                const Vector<double> &outputUbs,
                                unsigned outputIndex );
    static double dERUpperBound( const Vector<double> &sourceMids,
                                 const Vector<double> &outputLbs,
                                 const Vector<double> &outputUbs,
                                 unsigned outputIndex,
                                 unsigned inputIndex );
    static double linearLowerBound( const Vector<double> &outputLbs,
                                    const Vector<double> &outputUbs,
                                    unsigned outputIndex );
    static double linearUpperBound( const Vector<double> &outputLbs,
                                    const Vector<double> &outputUbs,
                                    unsigned outputIndex );

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
