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
        , _size( 0 )
        , _layerIndex( 0 )
        , _symbolicLb( NULL )
        , _symbolicUb( NULL )
        , _symbolicLowerBias( NULL )
        , _symbolicUpperBias( NULL )
        , _lb( NULL )
        , _ub( NULL )
        , _work1SymbolicLb( NULL )
        , _work1SymbolicUb( NULL )
        , _work2SymbolicLb( NULL )
        , _work2SymbolicUb( NULL )
        , _workSymbolicLowerBias( NULL )
        , _workSymbolicUpperBias( NULL )
    {};

    unsigned DeepPolyElement::getSize() const
    {
        return _size;
    }

    unsigned DeepPolyElement::getLayerIndex() const
    {
        return _layerIndex;
    }

    bool DeepPolyElement::hasPredecessor()
    {
        return !_layer->getSourceLayers().empty();
    }

    unsigned DeepPolyElement::getPredecessorIndex() const
    {
        const Map<unsigned, unsigned> &sourceLayers = _layer->getSourceLayers();
        // Right now, assume that each layer has one predecessor.
        ASSERT( sourceLayers.size() == 1 );
        return sourceLayers.begin()->first;
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
        for ( unsigned i = 0; i < size; ++i )
        {
            _lb[i] = _layer->getLb( i );
            _ub[i] = _layer->getUb( i );
        }
    }

    void DeepPolyElement::allocateMemory()
    {
        freeMemoryIfNeeded();

        unsigned size = getSize();
        _lb = new double[size];
        _ub = new double[size];

        std::fill_n( _lb, size, FloatUtils::negativeInfinity() );
        std::fill_n( _ub, size, FloatUtils::infinity() );
    }

    void DeepPolyElement::freeMemoryIfNeeded()
    {
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

    void DeepPolyElement::setWorkingMemory( double *work1SymbolicLb,
                                            double *work1SymbolicUb,
                                            double *work2SymbolicLb,
                                            double *work2SymbolicUb,
                                            double *workSymbolicLowerBias,
                                            double *workSymbolicUpperBias )
    {
        _work1SymbolicLb = work1SymbolicLb;
        _work1SymbolicUb = work1SymbolicUb;
        _work2SymbolicLb = work2SymbolicLb;
        _work2SymbolicUb = work2SymbolicUb;
        _workSymbolicLowerBias = workSymbolicLowerBias;
        _workSymbolicUpperBias = workSymbolicUpperBias;
    }

} // namespace NLR
