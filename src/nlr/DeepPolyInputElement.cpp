/*********************                                                        */
/*! \file DeepPolyInputElement.cpp
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

#include "DeepPolyInputElement.h"
#include "FloatUtils.h"
#include "NLRError.h"

namespace NLR {

    DeepPolyInputElement::DeepPolyInputElement( Layer *layer )
    {
        _layer = layer;
    }

    DeepPolyInputElement::~DeepPolyInputElement()
    {
        freeMemoryIfNeeded();
    }

    void DeepPolyInputElement::execute( const Map<unsigned, DeepPolyElement *>
                                   &deepPolyElementsBefore )
    {
        if ( !deepPolyElementsBefore.empty() )
        {
            throw NLRError( NLRError::INPUT_LAYER_NOT_THE_FIRST_LAYER );
        }
        // Update the concrete bounds
        freeMemoryIfNeeded();
        allocateMemoryForUpperAndLowerBounds();
        getConcreteBounds();
    }

} // namespace NLR
