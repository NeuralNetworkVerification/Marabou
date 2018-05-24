/*********************                                                        */
/*! \file PrecisionRestorer.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "Debug.h"
#include "FloatUtils.h"
#include "PrecisionRestorer.h"
#include "SmtCore.h"

void PrecisionRestorer::storeInitialEngineState( const IEngine &engine )
{
    engine.storeState( _initialEngineState, true );
}

void PrecisionRestorer::restorePrecision( IEngine &engine,
                                          ITableau &tableau,
                                          SmtCore &smtCore,
                                          RestoreBasics restoreBasics )
{
    // Store the dimensions, bounds and basic variables in the current tableau, before restoring it
    unsigned targetM = tableau.getM();
    unsigned targetN = tableau.getN();

    Set<unsigned> shouldBeBasic = tableau.getBasicVariables();

    double *lowerBounds = new double[targetN];
    double *upperBounds = new double[targetN];
    memcpy( lowerBounds, tableau.getLowerBounds(), sizeof(double) * targetN );
    memcpy( upperBounds, tableau.getUpperBounds(), sizeof(double) * targetN );

    try
    {
        EngineState targetEngineState;
        engine.storeState( targetEngineState, false );

        // Store the case splits performed so far
        List<PiecewiseLinearCaseSplit> targetSplits;
        smtCore.allSplitsSoFar( targetSplits );

        // Restore engine and tableau to their original form
        engine.restoreState( _initialEngineState );

        // Re-add all splits, which will restore variables and equations
        for ( const auto &split : targetSplits )
            engine.applySplit( split );

        // At this point, the tableau has the appropriate dimensions. Restore the variable bounds
        // and basic variables.

        ASSERT( tableau.getN() == targetN );
        ASSERT( tableau.getM() == targetM );

        if ( restoreBasics == RESTORE_BASICS )
        {
            Set<unsigned> basicAfterRestoration = tableau.getBasicVariables();
            Set<unsigned> needToBeBasic = Set<unsigned>::difference( shouldBeBasic, basicAfterRestoration );

            for ( unsigned variable : needToBeBasic )
            {
                /* This variable is currently non-basic. We attempt to make it basic by computing
                   its column and finding a basic variable that shouldn't be basic. Then we can
                   pivot these two variables. */

                unsigned enteringIndex = tableau.variableToIndex( variable );
                tableau.setEnteringVariableIndex( enteringIndex );

                tableau.computeChangeColumn();
                const double *changeColumn = tableau.getChangeColumn();

                // Find a variable that is basic but should be non-basic
                bool done = false;
                unsigned i = 0;
                while ( !done && ( i < targetM ) )
                {
                    if ( FloatUtils::gte( FloatUtils::abs( changeColumn[i] ),
                                          GlobalConfiguration::ACCEPTABLE_SIMPLEX_PIVOT_THRESHOLD ) )
                    {
                        unsigned basic = tableau.basicIndexToVariable( i );
                        if ( !shouldBeBasic.exists( basic ) )
                        {
                            tableau.setLeavingVariableIndex( i );
                            tableau.performPivot();
                            done = true;
                        }
                    }

                    ++i;
                }
            }
        }

        // Tighten bounds if needed. The tableau will ignore these bounds if
        // tighter bounds are already known, somehow.
        for ( unsigned i = 0; i < targetN; ++i )
        {
            tableau.tightenLowerBound( i, lowerBounds[i] );
            tableau.tightenUpperBound( i, upperBounds[i] );
        }

        // Restore constraint status
        for ( const auto &pair : targetEngineState._plConstraintToState )
            pair.first->setActiveConstraint( pair.second->isActive() );

        engine.setNumPlConstraintsDisabledByValidSplits
            ( targetEngineState._numPlConstraintsDisabledByValidSplits );

        DEBUG({
                // Same dimensions
                ASSERT( tableau.getN() == targetN );
                ASSERT( tableau.getM() == targetM );

                // Constraints should be in the same state before and after restoration
                for ( const auto &pair : targetEngineState._plConstraintToState )
                {
                    ASSERT( pair.second->isActive() == pair.first->isActive() );
                    ASSERT( pair.second->phaseFixed() == pair.first->phaseFixed() );
                    ASSERT( pair.second->constraintObsolete() == pair.first->constraintObsolete() );
                }

                EngineState currentEngineState;
                engine.storeState( currentEngineState, false );

                ASSERT( currentEngineState._numPlConstraintsDisabledByValidSplits ==
                        targetEngineState._numPlConstraintsDisabledByValidSplits );

                tableau.verifyInvariants();
            });
    }
    catch ( ... )
    {
        delete[] lowerBounds;
        delete[] upperBounds;

        throw;
    }

    delete[] lowerBounds;
    delete[] upperBounds;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
