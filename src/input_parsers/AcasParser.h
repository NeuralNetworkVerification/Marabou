/*********************                                                        */
/*! \file AcasParser.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __AcasParser_h__
#define __AcasParser_h__

#include "Map.h"
#include "AcasNeuralNetwork.h"

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

    unsigned getInputVariable( unsigned index ) const;
    unsigned getOutputVariable( unsigned index ) const;

private:
    AcasNeuralNetwork _acasNeuralNetwork;
    Map<NodeIndex, unsigned> _nodeToB;
    Map<NodeIndex, unsigned> _nodeToF;
    Map<NodeIndex, unsigned> _nodeToAux;
};

#endif // __AcasParser_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
