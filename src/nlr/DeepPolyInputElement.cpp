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

#include "Debug.h"
#include "DeepPolyInputElement.h"
#include "FloatUtils.h"
#include "NLRError.h"

namespace NLR {

    DeepPolyInputElement::DeepPolyInputElement( Layer *layer )
    {
        _layer = layer;
        _size = layer->getSize();
        _layerIndex = layer->getLayerIndex();
    }

    DeepPolyInputElement::~DeepPolyInputElement()
    {
        freeMemoryIfNeeded();
    }

    void DeepPolyInputElement::execute( const Map<unsigned, DeepPolyElement *>
                                   &deepPolyElementsBefore )
    {
        log( "Executing..." );
        if ( !deepPolyElementsBefore.empty() )
        {
            throw NLRError( NLRError::INPUT_LAYER_NOT_THE_FIRST_LAYER );
        }
        // Update the concrete bounds
        freeMemoryIfNeeded();
        allocateMemory();
        getConcreteBounds();
        log( "Executing - done" );
    }

    void DeepPolyInputElement::symbolicBoundInTermsOfPredecessor
    ( const double *, const double *, double *, double *, double *, double *,
      unsigned, unsigned, unsigned )
    {
        // Input layer should not have a predecessor
        ASSERT( false );
    }

    void DeepPolyInputElement::log( const String &message )
    {
        if ( GlobalConfiguration::NETWORK_LEVEL_REASONER_LOGGING )
            printf( "DeepPolyInputElement: %s\n", message.ascii() );
    }

} // namespace NLR
