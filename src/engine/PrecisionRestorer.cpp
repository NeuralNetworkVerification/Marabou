/*********************                                                        */
/*! \file PrecisionRestorer.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#include "PrecisionRestorer.h"

#include "Debug.h"
#include "FloatUtils.h"
#include "MalformedBasisException.h"
#include "MarabouError.h"
#include "SmtCore.h"
#include "TableauStateStorageLevel.h"
#include "UnsatCertificateNode.h"

void PrecisionRestorer::storeInitialEngineState( const IEngine &engine )
{
    engine.storeState( _initialEngineState, TableauStateStorageLevel::STORE_ENTIRE_TABLEAU_STATE );
}

void PrecisionRestorer::restoreInitialEngineState( IEngine &engine )
{
    engine.restoreState( _initialEngineState );
}

void PrecisionRestorer::restorePrecision( IEngine &engine,
                                          ITableau &tableau,
                                          RestoreBasics restoreBasics )
{
    // Store the dimensions, bounds and basic variables in the current tableau,
    // before restoring it
    unsigned targetM = tableau.getM();
    unsigned targetN = tableau.getN();

    Set<unsigned> shouldBeBasic = tableau.getBasicVariables();
    EngineState targetEngineState;

    DEBUG({
        engine.storeState( targetEngineState, TableauStateStorageLevel::STORE_NONE );
    })

    BoundExplainer boundExplainerBackup( targetN, targetM, engine.getContext() );

    Vector<double> upperBoundsBackup = Vector<double>( targetN, 0 );
    Vector<double> lowerBoundsBackup = Vector<double>( targetN, 0 );

    boundExplainerBackup = *engine.getBoundExplainer();

    for ( unsigned i = 0; i < targetN; ++i )
    {
        lowerBoundsBackup[i] = tableau.getLowerBound( i );
        upperBoundsBackup[i] = tableau.getUpperBound( i );
    }

    // Restore engine and tableau to their original form
    restoreInitialEngineState( engine );
    engine.postContextPopHook();
    DEBUG( tableau.verifyInvariants() );

    // At this point, the tableau has the appropriate dimensions. Restore the
    // variable bounds and basic variables. Note that if column merging is
    // enabled, the dimensions may not be precisely those before the
    // restoration, because merging sometimes fails - in which case an equation
    // is added. If we fail to restore the dimensions, we cannot restore the
    // basics.

    bool dimensionsRestored = ( tableau.getN() == targetN ) && ( tableau.getM() == targetM );

    ASSERT( dimensionsRestored || GlobalConfiguration::USE_COLUMN_MERGING_EQUATIONS );

    Set<unsigned> currentBasics = tableau.getBasicVariables();

    if ( dimensionsRestored && restoreBasics == RESTORE_BASICS )
    {
        List<unsigned> shouldBeBasicList;
        for ( const auto &basic : shouldBeBasic )
            shouldBeBasicList.append( basic );

        bool failed = false;
        try
        {
            tableau.initializeTableau( shouldBeBasicList );
        }
        catch ( MalformedBasisException & )
        {
            failed = true;
        }

        if ( failed )
        {
            // The "restoreBasics" set leads to a malformed basis.
            // Try again without this part of the restoration
            shouldBeBasicList.clear();
            for ( const auto &basic : currentBasics )
                shouldBeBasicList.append( basic );

            try
            {
                tableau.initializeTableau( shouldBeBasicList );
            }
            catch ( MalformedBasisException & )
            {
                throw MarabouError( MarabouError::RESTORATION_FAILED_TO_REFACTORIZE_BASIS,
                                    "Precision restoration failed - could not refactorize "
                                    "basis after setting basics" );
            }
        }
    }

    engine.setBoundExplainerContent( &boundExplainerBackup );

    for ( unsigned i = 0; i < targetN; ++i )
    {
        tableau.tightenUpperBoundNaively( i, upperBoundsBackup[i] );
        tableau.tightenLowerBoundNaively( i, lowerBoundsBackup[i] );
    }

    // Notify all constraints for the restore bounds
    // Will fix phases if necessary
    engine.propagateBoundManagerTightenings();

    DEBUG( {
        // Same dimensions
        ASSERT( GlobalConfiguration::USE_COLUMN_MERGING_EQUATIONS || tableau.getN() == targetN );
        ASSERT( GlobalConfiguration::USE_COLUMN_MERGING_EQUATIONS || tableau.getM() == targetM );

        // Constraints should be in the same state before and after restoration
        for ( const auto &pair : targetEngineState._plConstraintToState )
        {
            ASSERT( pair.second->isActive() == pair.first->isActive() );
            // Only active constraints need to be synchronized
            ASSERT( !pair.second->isActive() ||
                    pair.second->phaseFixed() == pair.first->phaseFixed() );
            ASSERT( pair.second->constraintObsolete() == pair.first->constraintObsolete() );
        }

        tableau.verifyInvariants();
    } );
}
