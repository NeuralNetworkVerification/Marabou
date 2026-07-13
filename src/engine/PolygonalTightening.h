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

#include "FloatUtils.h"
#include "MStringf.h"
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
        bool result = allFound && _value == other._value && _type == other._type &&
                      _neuronToCoefficient.size() == other._neuronToCoefficient.size();
        return result;
    }

    /*
       Get matching coefficent for a NeuronIndex,
       return 0 if the NeuronIndex is not present.
    */
    double getCoeff( NLR::NeuronIndex index ) const
    {
        if ( _neuronToCoefficient.exists( index ) )
            return _neuronToCoefficient[index];
        return 0;
    }

    void dump() const
    {
        String output = "PolygonalTightening: ";
        unsigned count = 0;
        for ( const auto &pair : _neuronToCoefficient )
        {
            double coeff = pair.second;
            if ( FloatUtils::isZero( coeff ) )
                continue;

            if ( count )
            {
                output += Stringf( "%s %.2lf neuron%u_%u ",
                                   FloatUtils::isPositive( coeff ) ? "+" : "-",
                                   FloatUtils::abs( coeff ),
                                   pair.first._layer,
                                   pair.first._neuron );
            }
            else
            {
                output +=
                    Stringf( "%.2lf neuron%u_%u ", coeff, pair.first._layer, pair.first._neuron );
            }
            ++count;
        }
        if ( count == 0 )
        {
            output += Stringf( "%.2lf ", 0 );
        }
        output += Stringf( "%s %.2lf", _type == LB ? ">=" : "<=", _value );
        printf( "%s\n", output.ascii() );
    }
};
#endif // __PolygonalTightening_h
