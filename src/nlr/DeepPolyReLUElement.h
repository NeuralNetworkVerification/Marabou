/*********************                                                        */
/*! \file DeepPolyReLUElement.h
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

#ifndef __DeepPolyReLUElement_h__
#define __DeepPolyReLUElement_h__

#include "DeepPolyElement.h"
#include "Layer.h"
#include "MStringf.h"
#include "NLRError.h"
#include <climits>

namespace NLR {

class DeepPolyReLUElement : public DeepPolyElement
{
public:
    DeepPolyReLUElement( Layer *layer );
    ~DeepPolyReLUElement();

    void execute( const Map<unsigned, DeepPolyElement *>
                  &deepPolyElementsBefore );

    void symbolicBoundInTermsOfPredecessor
    ( const double *symbolicLbPositiveWeights, const double
      *symbolicLbNegativeWeights, const double *symbolicUbPositiveWeights,
      const double *symbolicUbNegativeWeights, double *symbolicLowerBias,
      double *symbolicUpperBias, double *symbolicLb, double *symbolicUb );

private:

    void allocateMemory();
};

} // namespace NLR

#endif // __DeepPolyElement_h__
