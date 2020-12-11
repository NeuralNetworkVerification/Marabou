/*********************                                                        */
/*! \file DeepPolyWeightedSumElement.h
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

#ifndef __DeepPolyWeightedSumElement_h__
#define __DeepPolyWeightedSumElement_h__

#include "DeepPolyElement.h"
#include "Layer.h"
#include "MStringf.h"
#include "NLRError.h"
#include <climits>

namespace NLR {

class DeepPolyWeightedSumElement : public DeepPolyElement
{
public:

    DeepPolyWeightedSumElement( Layer *layer );
    ~DeepPolyWeightedSumElement();

    void execute( const Map<unsigned, DeepPolyElement *> &deepPolyElements );
    void symbolicBoundInTermsOfPredecessor
    ( const double *symbolicLbPositiveWeights, const double
      *symbolicLbNegativeWeights, const double *symbolicUbPositiveWeights,
      const double *symbolicUbNegativeWeights, double *symbolicLowerBias,
      double *symbolicUpperBias, double *symbolicLb, double *symbolicUb );

private:
    double * _workSymbolicLbPositiveWeights;
    double * _workSymbolicLbNegativeWeights;
    double * _workSymbolicUbPositiveWeights;
    double * _workSymbolicUbNegativeWeights;
    double * _workSymbolicLb;
    double * _workSymbolicUb;

    double * _workSymbolicLowerBias;
    double * _workSymbolicUpperBias;

    void allocateMemory( const Map<unsigned, DeepPolyElement *>
                         &deepPolyElements );
    void freeMemoryIfNeeded();

    void computeBoundWithBackSubstitution( const Map<unsigned, DeepPolyElement *>
                                           &deepPolyElementsBefore );

};

} // namespace NLR

#endif // __DeepPolyWeightedSumElement_h__
