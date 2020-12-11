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
        , _symbolicLb( NULL )
        , _symbolicUb( NULL )
        , _symbolicLowerBias( NULL )
        , _symbolicUpperBias( NULL )
        , _lb( NULL )
        , _ub( NULL )
    {};

    unsigned DeepPolyElement::getSize() const
    {
        return _layer->getSize();
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
        ASSERT( index < getSize() );
        return _lb[index];
    }

    double DeepPolyElement::getUpperBound( unsigned index ) const
    {
        ASSERT( index < getSize() );
        return _ub[index];
    }

    void DeepPolyElement::getConcreteBounds()
    {
        unsigned size = getSize();
        std::memcpy( _lb, _layer->getLbs(), size );
        std::memcpy( _ub, _layer->getUbs(), size );
    }

    void DeepPolyElement::allocateMemoryForUpperAndLowerBounds()
    {
        unsigned size = getSize();
        _lb = new double[size];
        _ub = new double[size];

        std::fill_n( _lb, size, FloatUtils::negativeInfinity() );
        std::fill_n( _ub, size, FloatUtils::infinity() );
    }

    void DeepPolyElement::freeMemoryIfNeeded()
    {
        if ( _symbolicLb )
        {
            delete[] _symbolicLb;
            _symbolicLb = NULL;
        }

        if ( _symbolicUb )
        {
            delete[] _symbolicUb;
            _symbolicUb = NULL;
        }

        if ( _symbolicLowerBias )
        {
            delete[] _symbolicLowerBias;
            _symbolicLowerBias = NULL;
        }

        if ( _symbolicUpperBias )
        {
            delete[] _symbolicUpperBias;
            _symbolicUpperBias = NULL;
        }

        if ( _lb )
        {
            delete[] _lb;
            _lb = NULL;
        }

        if ( _ub )
        {
            delete[] _ub;
            _ub = NULL;
        }
    }

} // namespace NLR
