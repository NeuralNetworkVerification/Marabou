/*********************                                                        */
/*! \file SODDivider.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "LargestIntervalDivider.h"

LargestIntervalDivider::LargestIntervalDivider( List<unsigned> &inputVariables,
                                                double timeoutFactor )
    : _timeoutFactor( timeoutFactor )
{
    _inputVariables = inputVariables;
}

void LargestIntervalDivider::createSubQueries( unsigned numNewSubqueries,
                                               const SubQuery &previousSubQuery,
                                               SubQueries &subQueries )
{
    unsigned numBisects = ( unsigned ) log2( numNewSubqueries );

    // Get the query id, previous case split, and the previous timeout
    const std::string queryIdPrefix = std::get<0>( previousSubQuery );
    const PiecewiseLinearCaseSplit previousSplit = *( std::get<1>
                                                      ( previousSubQuery ) );
    const unsigned timeoutInSeconds = std::get<2>( previousSubQuery );

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

    // Repeatedly bisect the dimension with the largest interval
    for ( unsigned i = 0; i < numBisects; ++i )
    {
        List<InputRegion> newInputRegions;
        for ( const auto &inputRegion : inputRegions )
        {
            unsigned dimensionToSplit = getLargestInterval( inputRegion );
            bisectInputRegion( inputRegion, dimensionToSplit, newInputRegions );
        }
        inputRegions = newInputRegions;
    }

    unsigned queryIdSuffix = 1; // For query id
    // Create a new subquery for each newly created input region
    for ( const auto &inputRegion : inputRegions )
    {
        // Create a new query id
        std::string queryId;
        if ( queryIdPrefix == "" )
            queryId = queryIdPrefix + std::to_string( queryIdSuffix++ );
        else
            queryId = queryIdPrefix + "-" + std::to_string( queryIdSuffix++ );

        // Create a new case split
        auto split = std::unique_ptr<PiecewiseLinearCaseSplit>
            ( new PiecewiseLinearCaseSplit() );
        // Add bound as equations for each input variable
        for ( const auto &variable : _inputVariables )
        {
            double lb = inputRegion._lowerBounds[variable];
            double ub = inputRegion._upperBounds[variable];
            split->storeBoundTightening( Tightening( variable, lb,
                                                     Tightening::LB ) );
            split->storeBoundTightening( Tightening( variable, ub,
                                                     Tightening::UB ) );
        }

        // Construct the new subquery and add it to subqueries
        SubQuery *subQuery = new SubQuery( queryId, std::move(split),
                                           (unsigned) (timeoutInSeconds *
                                                       _timeoutFactor) );
        subQueries.append( subQuery );
    }
}

unsigned LargestIntervalDivider::getLargestInterval( const InputRegion
                                                     &inputRegion )
{
    ASSERT( inputRegion._lowerBounds.size() == inputRegion._lowerBounds.size() );
    unsigned dimensionToSplit = 0;
    double largestInterval = 0;

    for ( const auto &variable : _inputVariables )
    {
        double interval = inputRegion._upperBounds[variable] -
            inputRegion._lowerBounds[variable];
        ASSERT( interval > 0 );
        if ( interval > largestInterval )
        {
            dimensionToSplit = variable;
            largestInterval = interval;
        }
    }
    return dimensionToSplit;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
