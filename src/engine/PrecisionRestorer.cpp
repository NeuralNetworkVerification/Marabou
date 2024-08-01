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
#include "QuitFromPrecisionRestorationException.h"
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
                                          RestoreBasics restoreBasics,
                                          bool shouldQuit )
{
    // Store the dimensions, bounds and basic variables in the current tableau,
    // before restoring it
    unsigned targetM = tableau.getM();
    unsigned targetN = tableau.getN();

    Set<unsigned> shouldBeBasic = tableau.getBasicVariables();

    BoundExplainer boundExplainerBackup( targetN, targetM, engine.getContext() );
    Vector<double> groundUpperBoundsBackup;
    Vector<double> groundLowerBoundsBackup;

    Vector<double> upperBoundsBackup;
    Vector<double> lowerBoundsBackup;

    if ( engine.shouldProduceProofs() )
    {
        groundUpperBoundsBackup = Vector<double>( targetN, 0 );
        groundLowerBoundsBackup = Vector<double>( targetN, 0 );

        upperBoundsBackup = Vector<double>( targetN, 0 );
        lowerBoundsBackup = Vector<double>( targetN, 0 );

        boundExplainerBackup = *engine.getBoundExplainer();

        for ( unsigned i = 0; i < targetN; ++i )
        {
            lowerBoundsBackup[i] = tableau.getLowerBound( i );
            upperBoundsBackup[i] = tableau.getUpperBound( i );

            groundUpperBoundsBackup[i] = engine.getGroundBound( i, Tightening::UB );
            groundLowerBoundsBackup[i] = engine.getGroundBound( i, Tightening::LB );
        }
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
            if ( shouldQuit )
                throw QuitFromPrecisionRestorationException();

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

    if ( engine.shouldProduceProofs() )
    {
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
            ASSERT( GlobalConfiguration::USE_COLUMN_MERGING_EQUATIONS ||
                    tableau.getN() == targetN );
            ASSERT( GlobalConfiguration::USE_COLUMN_MERGING_EQUATIONS ||
                    tableau.getM() == targetM );


            tableau.verifyInvariants();
        } );
    }
}
