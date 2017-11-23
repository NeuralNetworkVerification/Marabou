/*********************                                                        */
/*! \file TfParser.h
** \verbatim
** Top contributors (to current version):
**   Kyle Julian
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __TfParser_h__
#define __TfParser_h__

#include "Map.h"
#include "TensorflowNeuralNetwork.h"

class InputQuery;
class String;

class TfParser
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

    TfParser( const String &path );
    void generateQuery( InputQuery &inputQuery );

    unsigned getInputVariable( unsigned index ) const;
    unsigned getOutputVariable( unsigned index ) const;
    unsigned getBVariable( unsigned layer, unsigned index ) const;
    unsigned getFVariable( unsigned layer, unsigned index ) const;
    unsigned getAuxVariable( unsigned layer, unsigned index ) const;
    unsigned getSlackVariable( unsigned layer, unsigned index ) const;

    void evaluate( const Vector<double> &inputs, Vector<double> &outputs ) const;
    int num_inputs();
    int num_outputs();    

private:
    TensorflowNeuralNetwork _tfNeuralNetwork;
    Map<NodeIndex, unsigned> _nodeToB;
    Map<NodeIndex, unsigned> _nodeToF;
    Map<NodeIndex, unsigned> _nodeToAux;
    Map<NodeIndex, unsigned> _nodeToSlack;
};

#endif // __TfParser_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
