/*********************                                                        */
/*! \file BaBsrHeuristicDivider.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Liam Davis
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "BaBsrHeuristicDivider.h"

#include "Debug.h"
#include "FloatUtils.h"
#include "MStringf.h"
#include "PiecewiseLinearCaseSplit.h"

BaBsrHeuristicDivider::BaBsrHeuristicDivider( const List<unsigned> &inputVariables )
    : _inputVariables( inputVariables )
{
}

void BaBsrHeuristicDivider::createSubQueries( unsigned numNewSubqueries,
                                              const String queryIdPrefix,
                                              const unsigned previousDepth,
                                              const PiecewiseLinearCaseSplit &previousSplit,
                                              const unsigned timeoutInSeconds,
                                              SubQueries &subQueries )
{
    unsigned numBisects = (unsigned)log2( numNewSubqueries );

    List<InputRegion> inputRegions;

    // Create the first input region from the previous case split
    InputRegion region;
    List<Tightening> bounds = previousSplit.getBoundTightenings();
    for ( const auto &bound : bounds )
    {
        if ( bound._type == Tightening::LB )
        {
            region._lowerBounds[bound._variable] = bound._value;
        }
        else
        {
            ASSERT( bound._type == Tightening::UB );
            region._upperBounds[bound._variable] = bound._value;
        }
    }
    inputRegions.append( region );

    // repeatedly bisect the dimension with the BaBSR Heuristic
    for ( unsigned i = 0; i < numBisects; ++i )
    {
        List<InputRegion> newInputRegions;

        for ( const auto &inputRegion : inputRegions )
        {
            unsigned nodeToSplit = getNodeToSplit( inputRegion );
            bisectInputRegion( inputRegion, nodeToSplit, newInputRegions );
        }
        inputRegions = newInputRegions;
    }

    unsigned queryIdSuffix = 1; // For query id
    // Create a new subquery for each newly created input region
    for ( const auto &inputRegion : inputRegions )
    {
        // Create a new query id
        String queryId;
        if ( queryIdPrefix == "" )
            queryId = queryIdPrefix + Stringf( "%u", queryIdSuffix++ );
        else
            queryId = queryIdPrefix + Stringf( "-%u", queryIdSuffix++ );

        // Create a new case split
        auto split = std::unique_ptr<PiecewiseLinearCaseSplit>( new PiecewiseLinearCaseSplit() );
        // Add bound as equations for each input variable
        for ( const auto &variable : _inputVariables )
        {
            double lb = inputRegion._lowerBounds[variable];
            double ub = inputRegion._upperBounds[variable];
            split->storeBoundTightening( Tightening( variable, lb, Tightening::LB ) );
            split->storeBoundTightening( Tightening( variable, ub, Tightening::UB ) );
        }

        // Construct the new subquery and add it to subqueries
        SubQuery *subQuery = new SubQuery;
        subQuery->_queryId = queryId;
        subQuery->_split = std::move( split );
        subQuery->_timeoutInSeconds = timeoutInSeconds;
        subQuery->_depth = previousDepth + 1;
        subQueries.append( subQuery );
    }
}

unsigned BaBsrHeuristicDivider::getNodeToSplit( const InputRegion &inputRegion )
{
    ASSERT( inputRegion._lowerBounds.size() == inputRegion._upperBounds.size() );
    unsigned bestNode = 0;
    double maxScore = -1;

    DEBUG( bool haveCandidate = false );

    // iterate over all ReLU nodes in the network
    for ( const auto &node : _inputVariables )
    {
        // get the upper and lower bound of the input region
        double upper_bound = inputRegion._upperBounds[node];
        double lower_bound = inputRegion._lowerBounds[node];

        // calculate the BaBSR heuristic score for each ReLU node
        double score = calculateHeuristicScore( node, upper_bound, lower_bound );

        DEBUG( haveCandidate = true );

        if ( score > maxScore )
        {
            bestNode = node;
            maxScore = score;
        }
    }

    // return the ReLU node with the highest score
    ASSERT( maxScore >= 0 );
    ASSERT( haveCandidate );
    return bestNode;
}

unsigned BaBsrHeuristicDivider::calculateHeuristicScore( unsigned node,
                                                         double upper_bound,
                                                         double lower_bound )
{
    (void)node;

    // get ReLU output value and bias term
    double output = 0; // getReLUOutput(node);
    double bias = 0;   // getBias(node);

    // calculate score
    double scaler = upper_bound / ( upper_bound - lower_bound );
    double term1 = std::min( scaler * output * bias, ( scaler - 1.0 ) * output * bias );
    double term2 = std::max( scaler * lower_bound, 0.0 ) * std::max( 0.0, output );

    // return score
    return abs( term1 - term2 );
}
//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//