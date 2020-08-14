/*********************                                                        */
/*! \file PolairtyBasedDivider.cpp
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

#include "Debug.h"
#include "EngineState.h"
#include "FloatUtils.h"
#include "PolarityBasedDivider.h"
#include "MStringf.h"
#include "PiecewiseLinearCaseSplit.h"

PolarityBasedDivider::PolarityBasedDivider( std::shared_ptr<IEngine> engine )
    : _engine( std::move( engine ) )
{
};

void PolarityBasedDivider::createSubQueries( unsigned numNewSubqueries, const
                                             String queryIdPrefix, const
                                             unsigned previousDepth, const
                                             PiecewiseLinearCaseSplit
                                             &previousSplit, const unsigned
                                             timeoutInSeconds, SubQueries
                                             &subQueries )
{
    unsigned numBisects = (unsigned)log2( numNewSubqueries );

    List<PiecewiseLinearCaseSplit *> splits;
    auto split = new PiecewiseLinearCaseSplit();
    *split = previousSplit;
    splits.append( split );

    for ( unsigned i = 0; i < numBisects; ++i )
    {
        List<PiecewiseLinearCaseSplit *> newSplits;
        for ( const auto &split : splits )
        {
            PiecewiseLinearConstraint *pLConstraintToSplit =
                getPLConstraintToSplit( *split );
            if ( pLConstraintToSplit == NULL )
            {
                auto newSplit = new PiecewiseLinearCaseSplit();
                *newSplit = *split;
                newSplits.append( newSplit );
            }
            else
            {
                auto caseSplits = pLConstraintToSplit->getCaseSplits();
                for ( const auto &caseSplit : caseSplits )
                {
                    auto newSplit = new PiecewiseLinearCaseSplit();
                    *newSplit = *split;
                    for ( const auto &tightening : caseSplit.getBoundTightenings() )
                        newSplit->storeBoundTightening( tightening );
                    newSplits.append( newSplit );
                }
            }
            delete split;
        }
        splits = newSplits;
    }

    unsigned queryIdSuffix = 1; // For query id
    // Create a new subquery for each newly created input region
    for ( const auto &split : splits )
    {
        // Create a new query id
        String queryId;
        if ( queryIdPrefix == "" )
            queryId = queryIdPrefix + Stringf( "%u", queryIdSuffix++ );
        else
            queryId = queryIdPrefix + Stringf( "-%u", queryIdSuffix++ );

        // Construct the new subquery and add it to subqueries
        SubQuery *subQuery = new SubQuery;
        subQuery->_queryId = queryId;
        subQuery->_depth = previousDepth + 1;
        subQuery->_split.reset( split );
        subQuery->_timeoutInSeconds = timeoutInSeconds;
        subQueries.append( subQuery );
    }
}

PiecewiseLinearConstraint *PolarityBasedDivider::getPLConstraintToSplit
( const PiecewiseLinearCaseSplit &split )
{
    EngineState *engineStateBeforeSplit = new EngineState();
    _engine->storeState( *engineStateBeforeSplit, true );
    _engine->applySplit( split );

    PiecewiseLinearConstraint *constraintToSplit = NULL;
    constraintToSplit = _engine->pickSplitPLConstraintSnC( SnCDivideStrategy::Polarity );
    _engine->restoreState( *engineStateBeforeSplit );
    delete engineStateBeforeSplit;
    return constraintToSplit;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
