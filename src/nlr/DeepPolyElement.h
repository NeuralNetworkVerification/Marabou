/*********************                                                        */
/*! \file DeepPolyElement.h
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

#ifndef __DeepPolyElement_h__
#define __DeepPolyElement_h__

#include "Layer.h"
#include "MStringf.h"
#include "NLRError.h"
#include <climits>

namespace NLR {

struct DeepPolyElement
{
    DeepPolyElement( Layer *layer )
        : _layer( layer )
        , _layerIndex( layer->getLayerIndex() )
        , _size( _layer->getSize() )
        , _symbolicLb( NULL )
        , _symbolicUb( NULL )
        , _symbolicLowerBias( NULL )
        , _symbolicUpperBias( NULL )
        , _lb( NULL )
        , _ub( NULL )
    {
        allocateMemory();
    }

    ~DeepPolyElement()
    {

    }

    void allocateMemory()
    {
        _lb = new double[_size];
        _ub = new double[_size];

        switch ( _layer->getLayerType() )
        {
        case Layer::RELU:
            _symbolicLb = new double[_size];
            _symbolicUb = new double[_size];

            std::fill_n( _symbolicLb, _size, 0 );
            std::fill_n( _symbolicUb, _size, 0 );

            _symbolicLowerBias = new double[_size];
            _symbolicUpperBias = new double[_size];

            std::fill_n( _symbolicLowerBias, _size, 0 );
            std::fill_n( _symbolicUpperBias, _size, 0 );
            break;
        default:
            break;
        }
    }


    void freeMemoryIfNeeded()
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

    Layer *_layer;
    unsigned _layerIndex;
    unsigned _size;

    // Abstract element described in
    // https://files.sri.inf.ethz.ch/website/papers/DeepPoly.pdf
    double *_symbolicLb;
    double *_symbolicUb;
    double *_symbolicLowerBias;
    double *_symbolicUpperBias;
    double *_lb;
    double *_ub;

};

} // namespace NLR

#endif // __DeepPolyElement_h__
