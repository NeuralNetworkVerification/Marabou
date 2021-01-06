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
    ( const double *symbolicLb, const double*symbolicUb, double
      *symbolicLowerBias, double *symbolicUpperBias, double
      *symbolicLbInTermsOfPredecessor, double *symbolicUbInTermsOfPredecessor,
      unsigned targetLayerSize, DeepPolyElement *predecessor );

private:
    /*
      Compute the concrete upper- and lower- bounds of this layer by concretizing
      the symbolic bounds with respect to every preceding element.
    */
    void computeBoundWithBackSubstitution( const Map<unsigned, DeepPolyElement *>
                                           &deepPolyElementsBefore );

    /*
      Compute concrete bounds using symbolic bounds with respect to a
      sourceElement.
    */
    void concretizeSymbolicBound( const double *symbolicLb, const double
                                  *symbolicUb, const double *symbolicLowerBias,
                                  const double *symbolicUpperBias,
                                  DeepPolyElement *sourceElement );

    void log( const String &message );
};

} // namespace NLR

#endif // __DeepPolyWeightedSumElement_h__
