/*********************                                                        */
/*! \file BerkeleyParser.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __BerkeleyParser_h__
#define __BerkeleyParser_h__

#include "Map.h"
#include "BerkeleyNeuralNetwork.h"

class InputQuery;
class String;

class BerkeleyParser
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

    BerkeleyParser( const String &path );
    void generateQuery( InputQuery &inputQuery );

    Set<unsigned> getOutputVariables() const;

    void evaluate( const Vector<double> &inputs, Vector<double> &outputs ) const;

private:
    BerkeleyNeuralNetwork _berkeleyNeuralNetwork;
};

#endif // __BerkeleyParser_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
