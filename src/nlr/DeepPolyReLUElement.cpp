/*********************                                                        */
/*! \file DeepPolyReLUElement.cpp
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

#include "DeepPolyReLUElement.h"

namespace NLR {

    DeepPolyReLUElement::DeepPolyReLUElement( Layer *layer )
    {
        _layer = layer;
    }

    DeepPolyReLUElement::~DeepPolyReLUElement()
    {
        freeMemoryIfNeeded();
    }

    void DeepPolyReLUElement::execute( Map<unsigned, DeepPolyElement *>
                                   deepPolyElements )
    {
        freeMemoryIfNeeded();
        allocateMemory();
        std::cout << &deepPolyElements << std::endl;
    }

    void DeepPolyReLUElement::allocateMemory()
    {
        _lb = new double[_size];
        _ub = new double[_size];

        _symbolicLb = new double[_size];
        _symbolicUb = new double[_size];

        std::fill_n( _symbolicLb, _size, 0 );
        std::fill_n( _symbolicUb, _size, 0 );

        _symbolicLowerBias = new double[_size];
        _symbolicUpperBias = new double[_size];

        std::fill_n( _symbolicLowerBias, _size, 0 );
        std::fill_n( _symbolicUpperBias, _size, 0 );
    }

    void DeepPolyReLUElement::freeMemoryIfNeeded()
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
