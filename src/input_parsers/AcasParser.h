/*********************                                                        */
/*! \file AcasParser.h
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

#ifndef __AcasParser_h__
#define __AcasParser_h__

#include "AcasNeuralNetwork.h"
#include "Map.h"

class InputQuery;
class String;

class AcasParser
{
public:
    struct NodeIndex
    {
    public:
        NodeIndex( unsigned layer, unsigned node );
        bool operator<( const NodeIndex &other ) const;
        bool operator==( const NodeIndex &other ) const
        {
            return _layer == other._layer && _node == other._node;
        }

        unsigned _layer;
        unsigned _node;
    };

    AcasParser( const String &path );
    void generateQuery( InputQuery &inputQuery );

    unsigned getNumInputVaribales() const;
    unsigned getNumOutputVariables() const;
    unsigned getInputVariable( unsigned index ) const;
    unsigned getOutputVariable( unsigned index ) const;
    unsigned getBVariable( unsigned layer, unsigned index ) const;
    unsigned getFVariable( unsigned layer, unsigned index ) const;

    void evaluate( const Vector<double> &inputs, Vector<double> &outputs ) const;

private:
    AcasNeuralNetwork _acasNeuralNetwork;
    Map<NodeIndex, unsigned> _nodeToB;
    Map<NodeIndex, unsigned> _nodeToF;
};

#endif // __AcasParser_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
