/*********************                                                        */
/*! \file NeuronIndex.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#ifndef __NeuronIndex_h__
#define __NeuronIndex_h__

namespace NLR {

struct NeuronIndex
{
    NeuronIndex()
        : _layer( 0 )
        , _neuron( 0 )
    {
    }

    NeuronIndex( unsigned layer, unsigned neuron )
        : _layer( layer )
        , _neuron( neuron )
    {
    }

    bool operator<( const NeuronIndex &other ) const
    {
        if ( _layer < other._layer )
            return true;
        if ( _layer > other._layer )
            return false;

        return _neuron < other._neuron;
    }

    bool operator!=( const NeuronIndex &other ) const
    {
        return _layer != other._layer || _neuron != other._neuron;
    }

    unsigned _layer;
    unsigned _neuron;
};

} // namespace NLR

#endif // __NeuronIndex_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
