/*********************                                                        */
/*! \file PolygonalTightening.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Duligur Ibeling, Guy Katz, Ido Shmuel
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#ifndef __PolygonalTightening_h__
#define __PolygonalTightening_h__

#include "Map.h"
#include "NeuronIndex.h"

#include <cstdio>

class PolygonalTightening
{
public:
    enum PolygonalBoundType {
        LB = 0,
        UB = 1,
    };

    PolygonalTightening( Map<NLR::NeuronIndex, double> neuronToCoefficient,
                         double value,
                         PolygonalBoundType type )
        : _neuronToCoefficient( neuronToCoefficient )
        , _value( value )
        , _type( type )
    {
    }

    /*
      The coefficient of each neuron.
    */
    Map<NLR::NeuronIndex, double> _neuronToCoefficient;

    /*
      Its new value.
    */
    double _value;

    /*
      Whether the tightening tightens the
      lower bound or the upper bound.
    */
    PolygonalBoundType _type;

    /*
      Equality operator.
    */
    bool operator==( const PolygonalTightening &other ) const
    {
        bool allFound = true;
        for ( const auto &pair : _neuronToCoefficient )
        {
            bool currentFound = false;
            for ( const auto &otherPair : other._neuronToCoefficient )
            {
                currentFound |= ( pair.first._layer == otherPair.first._layer &&
                                  pair.first._neuron == otherPair.first._neuron &&
                                  pair.second == otherPair.second );
            }
            allFound &= currentFound;
        }
        bool result = allFound && _value == other._value && _type == other._type;
        return result;
    }

    void dump() const
    {
        printf( "PolygonalTightening: x %s %.2lf\n", _type == LB ? ">=" : "<=", _value );

        for ( const auto &pair : _neuronToCoefficient )
        {
            printf( "NeuronIndex: (layer %u, neuron %u), Coefficient: %.2lf )\n",
                    pair.first._layer,
                    pair.first._neuron,
                    pair.second );
        }
    }
};

#endif // __PolygonalTightening_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
// s
