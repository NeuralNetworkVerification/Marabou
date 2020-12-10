/*********************                                                        */
/*! \file DeepPolyWeightedSumElement.cpp
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

#include "DeepPolyWeightedSumElement.h"
#include "FloatUtils.h"
#include "NLRError.h"

namespace NLR {

    DeepPolyWeightedSumElement::DeepPolyWeightedSumElement( Layer *layer )
    {
        _layer = layer;
    }

    DeepPolyWeightedSumElement::~DeepPolyWeightedSumElement()
    {
        freeMemoryIfNeeded();
    }

    void DeepPolyWeightedSumElement::execute( const Map<unsigned, DeepPolyElement *>
                                   &deepPolyElementsBefore )
    {
        allocateMemory();
        if ( deepPolyElementsBefore.empty() )
        {
            // If this is the first layer, just update the concrete bounds
            getConcreteBounds();
        } else

        if ( !deepPolyElementsBefore.empty() )
        {
            throw NLRError( NLRError::INPUT_LAYER_NOT_THE_FIRST_LAYER );
        }
        // Update the concrete bounds
        getConcreteBounds();
    }

    void DeepPolyWeightedSumElement::allocateMemory()
    {
        unsigned size = getSize();
        _lb = new double[size];
        _ub = new double[size];

        std::fill_n( _lb, size, FloatUtils::negativeInfinity() );
        std::fill_n( _ub, size, FloatUtils::infinity() );
    }

} // namespace NLR
