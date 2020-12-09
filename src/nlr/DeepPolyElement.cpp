/*********************                                                        */
/*! \file DeepPolyElement.cpp
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

#include "DeepPolyElement.h"

namespace NLR {

    DeepPolyElement::DeepPolyElement()
        : _layer( NULL )
        , _layerIndex( 0 )
        , _predecessorSize( 0 )
        , _size( 0 )
        , _symbolicLb( NULL )
        , _symbolicUb( NULL )
        , _symbolicLowerBias( NULL )
        , _symbolicUpperBias( NULL )
        , _lb( NULL )
        , _ub( NULL )
    {};

    unsigned DeepPolyElement::getPredecessorSize() const
    {
        return _predecessorSize;
    }

    unsigned DeepPolyElement::getSize() const
    {
        return _size;
    }

    unsigned DeepPolyElement::getLayerIndex() const
    {
        return _layer->getLayerIndex();
    }

    Layer::Type DeepPolyElement::getLayerType() const
    {
        return _layer->getLayerType();
    }

    double *DeepPolyElement::getSymbolicLb() const
    {
        return _symbolicLb;
    }

    double *DeepPolyElement::getSymbolicUb() const
    {
        return _symbolicUb;
    }

    double *DeepPolyElement::getSymbolicLowerBias() const
    {
        return _symbolicLowerBias;
    }

    double *DeepPolyElement::getSymbolicUpperBias() const
    {
        return _symbolicUpperBias;
    }

    double DeepPolyElement::getLowerBound( unsigned index ) const
    {
        ASSERT( index < _size );
        return _lb[index];
    }

    double DeepPolyElement::getUpperBound( unsigned index ) const
    {
        ASSERT( index < _size );
        return _ub[index];
    }

} // namespace NLR
