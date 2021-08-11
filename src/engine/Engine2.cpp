/*********************                                                        */
/*! \file Engine2.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Duligur Ibeling, Andrew Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief [[ Add one-line brief description here ]]
 **
 ** [[ Add lengthier description here ]]
 **/

#include "AutoConstraintMatrixAnalyzer.h"
#include "Debug.h"
#include "DisjunctionConstraint.h"
#include "Engine2.h"
#include "SmtState.h"
#include "EngineState.h"
#include "InfeasibleQueryException.h"
#include "InputQuery.h"
#include "MStringf.h"
#include "MalformedBasisException.h"
#include "MarabouError.h"
#include "NLRError.h"
#include "Options.h"
#include "PiecewiseLinearConstraint.h"
#include "Preprocessor.h"
#include "TableauRow.h"
#include "Tightening.h"
#include "TimeUtils.h"
#include "Pair.h"

Engine2::Engine2( std::shared_ptr<SplitProvidersManager> const& splitProvidersManager )
    : _rowBoundTightener( *_tableau )
    , _splitProvidersManager( splitProvidersManager )
    , _smtStackManager( this, splitProvidersManager )
    , _smtCoreSplitProvider( std::make_shared<SmtCoreSplitProvider>( this ) )
    , _numPlConstraintsDisabledByValidSplits( 0 )
    , _preprocessingEnabled( false )
    , _initialStateStored( false )
    , _work( NULL )
    , _basisRestorationRequired( Engine2::RESTORATION_NOT_NEEDED )
    , _basisRestorationPerformed( Engine2::NO_RESTORATION_PERFORMED )
    , _costFunctionManager( _tableau )
    , _quitRequested( false )
    , _exitCode( Engine2::NOT_DONE )
    , _constraintBoundTightener( *_tableau )
    , _numVisitedStatesAtPreviousRestoration( 0 )
    , _networkLevelReasoner( NULL )
    , _verbosity( Options::get()->getInt( Options::VERBOSITY ) )
    , _lastNumVisitedStates( 0 )
    , _lastIterationWithProgress( 0 )
    , _splittingStrategy( Options::get()->getDivideStrategy() )
    , _solveWithMILP( Options::get()->getBool( Options::SOLVE_WITH_MILP ) )
    , _gurobi( nullptr )
    , _milpEncoder( nullptr )
{
    _smtStackManager.setStatistics( &_statistics );
    _tableau->setStatistics( &_statistics );
    _rowBoundTightener->setStatistics( &_statistics );
    _constraintBoundTightener->setStatistics( &_statistics );
    _preprocessor.setStatistics( &_statistics );

    _activeEntryStrategy = _projectedSteepestEdgeRule;
    _activeEntryStrategy->setStatistics( &_statistics );

    _statistics.stampStartingTime();

    _splitProvidersManager->subscribeSplitProvider( _smtCoreSplitProvider );
}

Engine2::~Engine2()
{
    if ( _work )
    {
        delete[] _work;
        _work = NULL;
    }
}

void Engine2::setVerbosity( unsigned verbosity )
{
    _verbosity = verbosity;
}

void Engine2::adjustWorkMemorySize()
{
    if ( _work )
    {
        delete[] _work;
        _work = NULL;
    }

    _work = new double[_tableau->getM()];
    if ( !_work )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Engine2::work" );
}

bool Engine2::solve( unsigned timeoutInSeconds )
{
    SignalHandler::getInstance()->initialize();
    SignalHandler::getInstance()->registerClient( this );

    updateDirections();
    storeInitialEngineState();

    mainLoopStatistics();
    if ( _verbosity > 0 )
    {
        printf( "\nEngine2::solve: Initial statistics\n" );
        _statistics.print();
        printf( "\n---\n" );
    }

    applyAllValidConstraintCaseSplits();

    bool splitJustPerformed = true;
    struct timespec mainLoopStart = TimeUtils::sampleMicro();
    while ( true )
    {
        struct timespec mainLoopEnd = TimeUtils::sampleMicro();
        _statistics.addTimeMainLoop( TimeUtils::timePassed( mainLoopStart, mainLoopEnd ) );
        mainLoopStart = mainLoopEnd;

        if ( shouldExitDueToTimeout( timeoutInSeconds ) )
        {
            if ( _verbosity > 0 )
            {
                printf( "\n\nEngine2: quitting due to timeout...\n\n" );
                printf( "Final statistics:\n" );
                _statistics.print();
            }

            _exitCode = Engine2::TIMEOUT;
            _statistics.timeout();
            return false;
        }

        if ( _quitRequested )
        {
            if ( _verbosity > 0 )
            {
                printf( "\n\nEngine2: quitting due to external request...\n\n" );
                printf( "Final statistics:\n" );
                _statistics.print();
            }

            _exitCode = Engine2::QUIT_REQUESTED;
            return false;
        }

        try
        {
            DEBUG( _tableau->verifyInvariants() );

            mainLoopStatistics();
            if ( _verbosity > 1 && _statistics.getNumMainLoopIterations() %
                GlobalConfiguration::STATISTICS_PRINTING_FREQUENCY == 0 )
                _statistics.print();

            // Check whether progress has been made recently
            checkOverallProgress();

            // If the basis has become malformed, we need to restore it
            if ( basisRestorationNeeded() )
            {
                if ( _basisRestorationRequired == Engine2::STRONG_RESTORATION_NEEDED )
                {
                    performPrecisionRestoration( PrecisionRestorer::RESTORE_BASICS );
                    _basisRestorationPerformed = Engine2::PERFORMED_STRONG_RESTORATION;
                }
                else
                {
                    performPrecisionRestoration( PrecisionRestorer::DO_NOT_RESTORE_BASICS );
                    _basisRestorationPerformed = Engine2::PERFORMED_WEAK_RESTORATION;
                }

                _numVisitedStatesAtPreviousRestoration = _statistics.getNumVisitedTreeStates();
                _basisRestorationRequired = Engine2::RESTORATION_NOT_NEEDED;
                continue;
            }

            // Restoration is not required
            _basisRestorationPerformed = Engine2::NO_RESTORATION_PERFORMED;

            // Possible restoration due to preceision degradation
            if ( shouldCheckDegradation() && highDegradation() )
            {
                performPrecisionRestoration( PrecisionRestorer::RESTORE_BASICS );
                continue;
            }

            if ( _tableau->basisMatrixAvailable() )
            {
                explicitBasisBoundTightening();
                applyAllBoundTightenings();
                applyAllValidConstraintCaseSplits();
            }

            if ( splitJustPerformed )
            {
                do
                {
                    performSymbolicBoundTightening();
                } while ( applyAllValidConstraintCaseSplits() );
                splitJustPerformed = false;
            }


            _splitProvidersManager->letProvidersThink( _smtStackManager.getStack() );

            // Ask split providers for splits
            auto split = _splitProvidersManager->splitFromProviders();
            if ( split )
            {
                _smtStackManager.performSplit( *split );
                splitJustPerformed = true;
                continue;
            }

            if ( !_tableau->allBoundsValid() )
            {
                // Some variable bounds are invalid, so the query is unsat
                throw InfeasibleQueryException();
            }

            if ( allVarsWithinBounds() )
            {
                // The linear portion of the problem has been solved.
                // Check the status of the PL constraints
                collectViolatedPlConstraints();

                // If all constraints are satisfied, we are possibly done
                if ( allPlConstraintsHold() )
                {
                    if ( _tableau->getBasicAssignmentStatus() !=
                        ITableau::BASIC_ASSIGNMENT_JUST_COMPUTED )
                    {
                        if ( _verbosity > 0 )
                        {
                            printf( "Before declaring sat, recomputing...\n" );
                        }
                        // Make sure that the assignment is precise before declaring success
                        _tableau->computeAssignment();
                        continue;
                    }
                    if ( _verbosity > 0 )
                    {
                        printf( "\nEngine2::solve: sat assignment found\n" );
                        _statistics.print();
                    }
                    _exitCode = Engine2::SAT;
                    return true;
                }

                // We have violated piecewise-linear constraints.
                performConstraintFixingStep();

                // Finally, take this opporunity to tighten any bounds
                // and perform any valid case splits.
                tightenBoundsOnConstraintMatrix();
                applyAllBoundTightenings();
                // For debugging purposes
                checkBoundCompliancyWithDebugSolution();

                while ( applyAllValidConstraintCaseSplits() )
                    performSymbolicBoundTightening();

                continue;
            }

            // We have out-of-bounds variables.
            performSimplexStep();
            continue;
        }
        catch ( const MalformedBasisException& )
        {
            // Debug
            printf( "MalformedBasisException caught!\n" );
            //

            if ( _basisRestorationPerformed == Engine2::NO_RESTORATION_PERFORMED )
            {
                if ( _numVisitedStatesAtPreviousRestoration != _statistics.getNumVisitedTreeStates() )
                {
                    // We've tried a strong restoration before, and it didn't work. Do a weak restoration
                    _basisRestorationRequired = Engine2::WEAK_RESTORATION_NEEDED;
                }
                else
                {
                    _basisRestorationRequired = Engine2::STRONG_RESTORATION_NEEDED;
                }
            }
            else if ( _basisRestorationPerformed == Engine2::PERFORMED_STRONG_RESTORATION )
                _basisRestorationRequired = Engine2::WEAK_RESTORATION_NEEDED;
            else
            {
                printf( "Engine2: Cannot restore tableau!\n" );
                _exitCode = Engine2::ERROR;
                return false;
            }
        }
        catch ( const InfeasibleQueryException& )
        {
            // notify unsat for providers
            _splitProvidersManager->notifyUnsat();
            
            // The current query is unsat, and we need to pop.
            // If we're at level 0, the whole query is unsat.
            if ( !_smtStackManager.popSplit() )
            {
                if ( _verbosity > 0 )
                {
                    printf( "\nEngine2::solve: unsat query\n" );
                    _statistics.print();
                }
                _exitCode = Engine2::UNSAT;
                return false;
            }

        }
        catch ( ... )
        {
            _exitCode = Engine2::ERROR;
            printf( "Engine2: Unknown error!\n" );
            return false;
        }
    }
}

/*
bool isActiveSplit(PiecewiseLinearCaseSplit split)
{
    // check if active or not
    // is_active = there is at least one LB in _bounds
    // because there are tw cases in Relu split:
    // active: b>=0 (LB), f-b<=0 (UB)
    // inactive b<=0 (UB), f-b<=0 (UB)
    for (auto bound : split.getBoundTightenings())
    {
        if (bound._type == Tightening::BoundType::LB)
        {
            return true;
        }
    }
    return false;
};

bool Engine2::solve_with_rr( unsigned timeoutInSeconds )
{
    SignalHandler::getInstance()->initialize();
    SignalHandler::getInstance()->registerClient( this );

    updateDirections();
    storeInitialEngineState();

    mainLoopStatistics();
    if ( _verbosity > 0 )
    {
        printf( "\nEngine2::solve: Initial statistics\n" );
        _statistics.print();
        printf( "\n---\n" );
    }

    applyAllValidConstraintCaseSplits();

    bool splitJustPerformed = true;
    struct timespec mainLoopStart = TimeUtils::sampleMicro();
    while ( true )
    {
        struct timespec mainLoopEnd = TimeUtils::sampleMicro();
        _statistics.addTimeMainLoop( TimeUtils::timePassed( mainLoopStart, mainLoopEnd ) );
        mainLoopStart = mainLoopEnd;

        if ( shouldExitDueToTimeout( timeoutInSeconds ) )
        {
            if ( _verbosity > 0 )
            {
                printf( "\n\nEngine2: quitting due to timeout...\n\n" );
                printf( "Final statistics:\n" );
                _statistics.print();
            }

            _exitCode = Engine2::TIMEOUT;
            _statistics.timeout();
            return false;
        }

        if ( _quitRequested )
        {
            if ( _verbosity > 0 )
            {
                printf( "\n\nEngine2: quitting due to external request...\n\n" );
                printf( "Final statistics:\n" );
                _statistics.print();
            }

            _exitCode = Engine2::QUIT_REQUESTED;
            return false;
        }

        try
        {
            DEBUG( _tableau->verifyInvariants() );

            mainLoopStatistics();
            if ( _verbosity > 1 &&  _statistics.getNumMainLoopIterations() %
                 GlobalConfiguration::STATISTICS_PRINTING_FREQUENCY == 0 )
                _statistics.print();

            // Check whether progress has been made recently
            checkOverallProgress();

            // If the basis has become malformed, we need to restore it
            if ( basisRestorationNeeded() )
            {
                if ( _basisRestorationRequired == Engine2::STRONG_RESTORATION_NEEDED )
                {
                    performPrecisionRestoration( PrecisionRestorer::RESTORE_BASICS );
                    _basisRestorationPerformed = Engine2::PERFORMED_STRONG_RESTORATION;
                }
                else
                {
                    performPrecisionRestoration( PrecisionRestorer::DO_NOT_RESTORE_BASICS );
                    _basisRestorationPerformed = Engine2::PERFORMED_WEAK_RESTORATION;
                }

                _numVisitedStatesAtPreviousRestoration = _statistics.getNumVisitedTreeStates();
                _basisRestorationRequired = Engine2::RESTORATION_NOT_NEEDED;
                continue;
            }

            // Restoration is not required
            _basisRestorationPerformed = Engine2::NO_RESTORATION_PERFORMED;

            // Possible restoration due to preceision degradation
            if ( shouldCheckDegradation() && highDegradation() )
            {
                performPrecisionRestoration( PrecisionRestorer::RESTORE_BASICS );
                continue;
            }

            if ( _tableau->basisMatrixAvailable() )
            {
                explicitBasisBoundTightening();
                applyAllBoundTightenings();
                applyAllValidConstraintCaseSplits();
            }

            if ( splitJustPerformed )
            {
                // get current splits + Gamma's clauses and derive required splits
                List<Map<PiecewiseLinearCaseSplit, unsigned>> clauses = getGammaCluases(gamma);
                Map<PiecewiseLinearCaseSplit, unsigned> currentSplits = getCurrentSplits();
                Map<PiecewiseLinearCaseSplit, unsigned> requiredSplits = deriveRequiredSplits(currentSplits, clauses);
                // notify _smtCore about the new required splits
                _smtCore.handleRequiredSplits(requiredSplits);

                do
                {
                    performSymbolicBoundTightening();
                }
                while ( applyAllValidConstraintCaseSplits() );
                splitJustPerformed = false;
            }

            // Perform any SmtCore-initiated case splits
            if ( _smtCore.needToSplit())
            {
                _smtCore.performSplit();
                splitJustPerformed = true;
                continue;
            }

            if ( _smtCore.isThereRequiredSplits())
            {
                _smtCore.performReuiredSplits();
                splitJustPerformed = true;
                continue;
            }

            if ( !_tableau->allBoundsValid() )
            {
                // Some variable bounds are invalid, so the query is unsat
                throw InfeasibleQueryException();
            }

            if ( allVarsWithinBounds() )
            {
                // The linear portion of the problem has been solved.
                // Check the status of the PL constraints
                collectViolatedPlConstraints();

                // If all constraints are satisfied, we are possibly done
                if ( allPlConstraintsHold() )
                {
                    if ( _tableau->getBasicAssignmentStatus() !=
                         ITableau::BASIC_ASSIGNMENT_JUST_COMPUTED )
                    {
                        if ( _verbosity > 0 )
                        {
                            printf( "Before declaring sat, recomputing...\n" );
                        }
                        // Make sure that the assignment is precise before declaring success
                        _tableau->computeAssignment();
                        continue;
                    }
                    if ( _verbosity > 0 )
                    {
                        printf( "\nEngine2::solve: sat assignment found\n" );
                        _statistics.print();
                    }
                    _exitCode = Engine2::SAT;
                    return true;
                }

                // We have violated piecewise-linear constraints.
                performConstraintFixingStep();

                // Finally, take this opporunity to tighten any bounds
                // and perform any valid case splits.
                tightenBoundsOnConstraintMatrix();
                applyAllBoundTightenings();
                // For debugging purposes
                checkBoundCompliancyWithDebugSolution();

                while ( applyAllValidConstraintCaseSplits() )
                    performSymbolicBoundTightening();

                continue;
            }

            // We have out-of-bounds variables.
            performSimplexStep();
            continue;
        }
        catch ( const MalformedBasisException & )
        {
            // Debug
            printf( "MalformedBasisException caught!\n" );
            //

            if ( _basisRestorationPerformed == Engine2::NO_RESTORATION_PERFORMED )
            {
                if ( _numVisitedStatesAtPreviousRestoration != _statistics.getNumVisitedTreeStates() )
                {
                    // We've tried a strong restoration before, and it didn't work. Do a weak restoration
                    _basisRestorationRequired = Engine2::WEAK_RESTORATION_NEEDED;
                }
                else
                {
                    _basisRestorationRequired = Engine2::STRONG_RESTORATION_NEEDED;
                }
            }
            else if ( _basisRestorationPerformed == Engine2::PERFORMED_STRONG_RESTORATION )
                _basisRestorationRequired = Engine2::WEAK_RESTORATION_NEEDED;
            else
            {
                printf( "Engine2: Cannot restore tableau!\n" );
                _exitCode = Engine2::ERROR;
                return false;
            }
        }
        catch ( const InfeasibleQueryException & )
        {
            //add unsat split sequence to Gamma
            gamma.append(getCurrentSplits())
            // The current query is unsat, and we need to pop.
            // If we're at level 0, the whole query is unsat.
            if ( !_smtCore.popSplit() )
            {
                if ( _verbosity > 0 )
                {
                    printf( "\nEngine2::solve: unsat query\n" );
                    _statistics.print();
                }
                _exitCode = Engine2::UNSAT;
                return false;
            }
            else
            {
                splitJustPerformed = true;
            }

        }
        catch ( ... )
        {
            _exitCode = Engine2::ERROR;
            printf( "Engine2: Unknown error!\n" );
            return false;
        }
    }
}
Map<PiecewiseLinearCaseSplit, unsigned> deriveRequiredSplits(currentSplits, unsatClauses){
    // derives all required splits
    // a required split is derived if it is part of any unsat clause and all other parts
    // in the clause are mapped the same as they are mapped in currentSplits (the current mapping)
    // for example, if (notation is: active=1, inactive=0)
    // clauses = [{c1:1, c2:1}, {c1:1, c3:0}, {c2:0,c3:0}] and
    // currentSplits = {c1:1}
    // then 2 splits: {c2:0, c3:1} are derived from the 2 first clauses
    // (and no split is derived from the 3'rd clause)

    // implementation:
    // iterate the current splits, and for each one, assign all satisfied splits in any clause
    // if at last there are clauses with one remaining split, a required split with the oppose
    // activation will be added to the result
    // TBD: use watch literals

    Map<PiecewiseLinearCaseSplit, unsigned> derived;
    // list of mappings (for each clause)
    // from index in clause to bool if the split in this index in clause is satisfied
    List<Map<PiecewiseLinearCaseSplit, bool>> satisfiedSplits;
    for ( auto currentSplit : currentSplits ) {
        for ( auto clause_index, clause : unsatClauses ) {
            for ( auto split_index, split : clause ) {
                // if split is satisfied, assign it in satisfiedSplits
                bool isSameSplit = split.getVariable() == currentSplit.getVariable();
                bool isSameActivation = isActiveSplit(split) == isActiveSplit(currentSplit);
                if ( isSameSplit && isSameActivation ) {
                    satisfiedSplits[clause_index][split_index] = true;
                }
            }
        }
    }
    // for all clauses with exactly one unsatisfied split, derive oppose activation to this split
    for ( auto clause_index, clause : satisfiedSplits ) {
        unsigned int unsatisfiedCounter = 0;
        unsatisfiedSplitIndex = -1;
        for ( auto split_index, isSplitSatisfied : clause ) {
            if ( !isSplitSatisfied ) {
                unsatisfiedCounter += 1;
                if (unsatisfiedCounter > 1 ) {
                    break;
                }
            }
            else {
                unsatisfiedSplitIndex = split_index;
            }
        }
        if (unsatisfiedcounter == 1) {
            derivedSplit = unsatClauses[clause_index][unsatisfiedSplitIndex];
            // should assign active if unsat is imactive and vice versa
            unsigned int opposeActivation = -1 if isActiveSplit(derivedSplit) else 1;
            derived.add(key=split, value=opposeActivation)
        }
    }
    return derived;
}

bool isActiveSplit(PiecewiseLinearCaseSplit split)
{
    // check if active or not
    // is_active = there is at least one LB in _bounds
    // because there are tw cases in Relu split:
    // active: b>=0 (LB), f-b<=0 (UB)
    // inactive b<=0 (UB), f-b<=0 (UB)
    for (auto bound : split.getBoundTightenings())
    {
        if (bound._type == Tightening::BoundType::LB)
        {
            return true;
        }
    }
    return false;
};

*/

/*
bool Engine2::solve(
    List<Map<unsigned, bool>> gammaUnsat,
    Map<unsigned, Pair<unsigned, unsigned>> gammaAbs,
    Map<unsigned, bool> is_pos,
    Map<unsigned, bool> is_inc,
    Map<unsigned, unsigned> post_var_indices,
    unsigned timeoutInSeconds)
{
    SignalHandler::getInstance()->initialize();
    SignalHandler::getInstance()->registerClient(this);

    if (_solveWithMILP)
        return solveWithMILPEncoding(timeoutInSeconds);

    updateDirections();
    storeInitialEngineState();

    mainLoopStatistics();
    if (_verbosity > 0)
    {
        printf("\nEngine2::solve: Initial statistics\n");
        _statistics.print();
        printf("\n---\n");
    }

    applyAllValidConstraintCaseSplits();

    bool splitJustPerformed = true;
    struct timespec mainLoopStart = TimeUtils::sampleMicro();
    while (true)
    {
        struct timespec mainLoopEnd = TimeUtils::sampleMicro();
        _statistics.addTimeMainLoop(TimeUtils::timePassed(mainLoopStart, mainLoopEnd));
        mainLoopStart = mainLoopEnd;

        if (shouldExitDueToTimeout(timeoutInSeconds))
        {
            if (_verbosity > 0)
            {
                printf("\n\nEngine2: quitting due to timeout...\n\n");
                printf("Final statistics:\n");
                _statistics.print();
            }

            _exitCode = Engine2::TIMEOUT;
            _statistics.timeout();
            return false;
        }

        if (_quitRequested)
        {
            if (_verbosity > 0)
            {
                printf("\n\nEngine2: quitting due to external request...\n\n");
                printf("Final statistics:\n");
                _statistics.print();
            }

            _exitCode = Engine2::QUIT_REQUESTED;
            return false;
        }

        try
        {
            DEBUG(_tableau->verifyInvariants());

            mainLoopStatistics();
            if (_verbosity > 1 && _statistics.getNumMainLoopIterations() %
                                          GlobalConfiguration::STATISTICS_PRINTING_FREQUENCY ==
                                      0)
                _statistics.print();

            // Check whether progress has been made recently
            checkOverallProgress();

            // If the basis has become malformed, we need to restore it
            if (basisRestorationNeeded())
            {
                if (_basisRestorationRequired == Engine2::STRONG_RESTORATION_NEEDED)
                {
                    performPrecisionRestoration(PrecisionRestorer::RESTORE_BASICS);
                    _basisRestorationPerformed = Engine2::PERFORMED_STRONG_RESTORATION;
                }
                else
                {
                    performPrecisionRestoration(PrecisionRestorer::DO_NOT_RESTORE_BASICS);
                    _basisRestorationPerformed = Engine2::PERFORMED_WEAK_RESTORATION;
                }

                _numVisitedStatesAtPreviousRestoration = _statistics.getNumVisitedTreeStates();
                _basisRestorationRequired = Engine2::RESTORATION_NOT_NEEDED;
                continue;
            }

            // Restoration is not required
            _basisRestorationPerformed = Engine2::NO_RESTORATION_PERFORMED;

            // Possible restoration due to preceision degradation
            if (shouldCheckDegradation() && highDegradation())
            {
                performPrecisionRestoration(PrecisionRestorer::RESTORE_BASICS);
                continue;
            }

            if (_tableau->basisMatrixAvailable())
            {
                explicitBasisBoundTightening();
                applyAllBoundTightenings();
                applyAllValidConstraintCaseSplits();
            }

            if (splitJustPerformed)
            {
                // use GammaUnsat + gammaAbs to prune redundant splits
                // if last split is of n1 and n1 is refined from abstract node n
                // and n is (pos-inc & active) or (neg-dec & inactive)
                // and n is active in some sequence in GammaUnsat (assuming that
                // refinement is in abstraction order)
                // and post(n) are (pos-inc & active) or (neg-dec & inactive)
                // then we should split n2, the second node refined from n, to
                // the second (unequal to n1's) option (inactive/active)
                unsigned last_index = _smtCore.getLastSplit()->getBoundTightenings().front()._variable;

                auto const isVariableActive = [this](unsigned var) { !FloatUtils::lte(0, this->_tableau->getLowerBound(var)); };
                constexpr bool ACTIVE = true;
                constexpr bool INACTIVE = false;
                auto const getSplitVariable = [](PiecewiseLinearCaseSplit split) -> std::pair<unsigned, bool>
                {
                    for (auto const &bound : split.getBoundTightenings())
                    {
                        if(bound._type == Tightening::LB)
                        {
                            return {bound._variable, ACTIVE};
                        }
                        if (bound._type == Tightening::UB)
                        {
                            return {bound._variable, INACTIVE};
                        }
                    }
                };

                // condition 0: check if there is a decided node in the split seq, that is refined node of some abstract node

                // check condition 0: is there a decided node in the split seq, that is refined node of some abstract node
                bool condition0_holds = false;
                List<PiecewiseLinearCaseSplit> all_splits;
                _smtCore.allSplitsSoFar(all_splits);
                for (auto abstract_step : gammaAbs) // TODO: send GammaAbs in the query
                {
                    unsigned refined_1_index = abstract_step.second.first();
                    unsigned refined_2_index = abstract_step.second.second();
                    for (auto const &split : all_splits)
                    {
                        auto const splitVar = getSplitVariable(split);
                        if (splitVar.first == refined_1_index || splitVar.first == refined_2_index)
                        {
                            condition0_holds = true;
                        }
                    }
                }

                // condition 1: it is the same split sequence (order-invariant)
                bool condition1_holds= true;
                for (auto const &unsat_seq : gammaUnsat)
                {
                    for (auto pair : unsat_seq)
                    {
                        unsigned var = pair.first;
                        for(const auto & abstracted : gammaAbs)
                        {
                            if(var == abstracted.first)
                            {

                            }
                        }
                        bool is_active = pair.second;
                        bool is_in_current_seq_and_same_activation = false;
                        for (auto const &split : all_splits)
                        {
                            unsigned split_var; bool split_activation;
                            std::tie(split_var, split_activation) = getSplitVariable(split);
                            if (var == split_var && is_active == split_activation)
                            {
                                is_in_current_seq_and_same_activation = true;
                                break;
                            }
                        }
                        if(!is_in_current_seq_and_same_activation)
                        {
                            condition1_holds = false;
                        }
                    }

                }


                    // find if last_index is part of an abstraction step in gammaAbs
                    for ( auto abstract_step : gammaAbs )  // TODO: send GammaAbs in the query
                    {
                        // abstract_step maps abstract_index to refine indices pair
                        unsigned abstract_index = abstract_step.first;
                        unsigned refined_1_index = abstract_step.second.first();
                        unsigned refined_2_index = abstract_step.second.second();

                        // check one of the refined is the last node
                        if(! (refined_1_index == last_index || refined_2_index == last_index))
                        {
                            break;
                        }

                        bool is_refine1_active = !FloatUtils::lte( 0, _tableau->getLowerBound(refined_1_index) );
                        bool is_refine2_active = !FloatUtils::lte( 0, _tableau->getLowerBound(refined_2_index) );
                        // 1. is abstract node in UNSAT sequence? if so,
                        // 2. is abstract is inc+active or dec+inactive? if so,
                        // 3. is abstract node has equal activation to last_index?
                        bool is_unsat_seq = false;

                        // check 1. is abstract node in UNSAT sequence? if so,
                        bool is_abstract_active = false;
                        for ( auto unsat_seq : gammaUnsat ) {
                            // each element in seq is (var_index, Map<unsigned, PiecewiseLinearCaseSplit>)
                            for ( auto node_activation : unsat_seq ) {
                                // the next condition checks 1
                                if ( node_activation.first == abstract_index ) {
                                    is_abstract_active = node_activation.second;
                                }
                            }
                        }

                        bool is_last_active = !FloatUtils::lte( 0, _tableau->getLowerBound(last_index) );
                                   // the next condition checks 2
                                    if (is_last_active != is_abstract_active)
                                    {
                                        break; // can't learn from abstract case
                                    }
                                    // the next condition checks 3
                                    if (is_active == is_last_active)
                                    {
                                        is_unsat_seq = true;
                                        break;
                                    } // else, can't learn to refinement case
                            }  // else, can't learn from random node
                        }
                        // stop if found one unsat sequence with abstract node
                        if ( is_unsat_seq ) {
                            break;
                        }
                    }

                    // if any unsat sequence was not found, can't prune
                    if ( !is_unsat_seq ) {
                        continue;
                    }

                    // check if all post constraints enable pruning
                    bool all_post_constraints_activated_properly = false;
                    if ( last_index == refined_1_index || last_index == refined_2_index ) {
                        all_post_constraints_activated_properly = true;
                        for ( auto post_var : post_var_indices ) {
                            unsigned var_index = post_var_indices[post_var.second];
                            bool is_var_active = FloatUtils::lte(
                                0, _tableau->getLowerBound(var_index) );
                            if (!( ( is_var_active && is_pos[var_index] && is_inc[var_index] ) ||
                                 ( !is_var_active && !is_pos[var_index] && !is_inc[var_index] ) )) {
                                 all_post_constraints_activated_properly = false;
                                 break;
                            }
                        }
                    }
                    if ( !all_post_constraints_activated_properly ) {
                        continue;
                    }

                    // bcp: activate the second refined node as needed
                    bool is_refined_1_active = !FloatUtils::lte( 0, _tableau->getLowerBound(refined_1_index) );
                    bool is_refined_2_active = !FloatUtils::lte( 0, _tableau->getLowerBound(refined_2_index) );

                    // if equal activations, UNSAT from residual reasoning
                    if ( is_refined_1_active == is_refined_2_active ) {
                        throw InfeasibleQueryException();
                    }

                    // otherwise, activate as needed
                    if ( last_index == refined_1_index ) {
                        // assert is_refined_2_active != is_refined_1_active
                        if ( is_refined_1_active ) {
                            _tableau->tightenUpperBound( 0, refined_2_index );
                        }
                        else {  // !is_refined_1_active
                            _tableau->tightenLowerBound( refined_2_index, 0 );
                        }
                        break;
                    }
                    if ( last_index == refined_2_index ) {
                        // assert is_refined_1_active != is_refined_2_active
                        if ( is_refined_2_active ) {
                            _tableau->tightenUpperBound( refined_1_index, 0 );
                        }
                        else {
                            _tableau->tightenLowerBound( refined_1_index, 0 );
                        }
                    }
                    break;
                }

                do
                {
                    performSymbolicBoundTightening();
                }
                while ( applyAllValidConstraintCaseSplits() );
                splitJustPerformed = false;
            }

            // Perform any SmtCore-initiated case splits
            if ( _smtCore.needToSplit() )
            {
                _smtCore.performSplit();
                splitJustPerformed = true;
                continue;
            }

            if ( !_tableau->allBoundsValid() )
            {
                // Some variable bounds are invalid, so the query is unsat
                throw InfeasibleQueryException();
            }

            if ( allVarsWithinBounds() )
            {
                // The linear portion of the problem has been solved.
                // Check the status of the PL constraints
                collectViolatedPlConstraints();

                // If all constraints are satisfied, we are possibly done
                if ( allPlConstraintsHold() )
                {
                    if ( _tableau->getBasicAssignmentStatus() !=
                         ITableau::BASIC_ASSIGNMENT_JUST_COMPUTED )
                    {
                        if ( _verbosity > 0 )
                        {
                            printf( "Before declaring sat, recomputing...\n" );
                        }
                        // Make sure that the assignment is precise before declaring success
                        _tableau->computeAssignment();
                        continue;
                    }
                    if ( _verbosity > 0 )
                    {
                        printf( "\nEngine2::solve: sat assignment found\n" );
                        _statistics.print();
                    }
                    _exitCode = Engine2::SAT;
                    return true;
                }

                // We have violated piecewise-linear constraints.
                performConstraintFixingStep();

                // Finally, take this opporunity to tighten any bounds
                // and perform any valid case splits.
                tightenBoundsOnConstraintMatrix();
                applyAllBoundTightenings();
                // For debugging purposes
                checkBoundCompliancyWithDebugSolution();

                while ( applyAllValidConstraintCaseSplits() )
                    performSymbolicBoundTightening();
                continue;
            }

            // We have out-of-bounds variables.
            performSimplexStep();
            continue;
        }
        catch ( const MalformedBasisException & )
        {
            // Debug
            printf( "MalformedBasisException caught!\n" );
            //

            if ( _basisRestorationPerformed == Engine2::NO_RESTORATION_PERFORMED )
            {
                if ( _numVisitedStatesAtPreviousRestoration != _statistics.getNumVisitedTreeStates() )
                {
                    // We've tried a strong restoration before, and it didn't work. Do a weak restoration
                    _basisRestorationRequired = Engine2::WEAK_RESTORATION_NEEDED;
                }
                else
                {
                    _basisRestorationRequired = Engine2::STRONG_RESTORATION_NEEDED;
                }
            }
            else if ( _basisRestorationPerformed == Engine2::PERFORMED_STRONG_RESTORATION )
                _basisRestorationRequired = Engine2::WEAK_RESTORATION_NEEDED;
            else
            {
                printf( "Engine2: Cannot restore tableau!\n" );
                _exitCode = Engine2::ERROR;
                return false;
            }
        }
        catch ( const InfeasibleQueryException & )
        {
            // The current query is unsat, and we need to pop.
            // If we're at level 0, the whole query is unsat.
            if ( !_smtCore.popSplit() )
            {
                if ( _verbosity > 0 )
                {
                    printf( "\nEngine2::solve: unsat query\n" );
                    _statistics.print();
                }
                _exitCode = Engine2::UNSAT;
                Map<unsigned, bool> new_unsat_seq;
                auto is_active_relu = [](PiecewiseLinearCaseSplit split) -> std::pair<unsigned, bool>
                {
                    // check if active or not
                    // is_active = there is at least one LB in _bounds
                    // because there are tw cases in Relu split:
                    // active: b>=0 (LB), f-b<=0 (UB)
                    // inactive b<=0 (UB), f-b<=0 (UB)
                    for (auto bound : split.getBoundTightenings())
                    {
                        if (bound._type == Tightening::BoundType::LB)
                        {
                            return {bound._variable,true};
                        }
                    }
                    // return false;
                };

                //                new_unsat_seq = new Map<unsigned, PiecewiseLinearCaseSplit>;
                for (auto split : _smtCore.getStack().back()->_pastSplits)
                {
                    bool const is_active = is_active_relu(split);
                    for (const auto &bound : split.getBoundTightenings())
                    {
                        if (bound._type == Tightening::BoundType::LB && bound._value > 0)
                        {
                            new_unsat_seq.insert(bound._variable, true);
                        }
                    }
                }
                _statistics.appendGammaUnsatSplitSequence(new_unsat_seq);
               return false;
            }
            else
            {
                // add current splits sequence to UNSAT context list
                // Map<unsigned, PiecewiseLinearCaseSplit> new_unsat_seq;
                Map<unsigned, bool> new_unsat_seq;
//                new_unsat_seq = new Map<unsigned, PiecewiseLinearCaseSplit>;
                for (auto split : _smtCore.getStack().back()->_pastSplits) {
                    for (const auto &bound : split.getBoundTightenings())
                    {
                        if (bound._type == Tightening::BoundType::LB
                        && bound._value > 0)
                        {
                            new_unsat_seq.insert(bound._variable, true);
                        }
                    }

                }
                _statistics.appendGammaUnsatSplitSequence(new_unsat_seq);
                splitJustPerformed = true;
            }

        }
        catch ( ... )
        {
            _exitCode = Engine2::ERROR;
            printf( "Engine2: Unknown error!\n" );
            return false;
        }
    }
}
*/

void Engine2::mainLoopStatistics()
{
    struct timespec start = TimeUtils::sampleMicro();

    unsigned activeConstraints = 0;
    for ( const auto& constraint : _plConstraints )
        if ( constraint->isActive() )
            ++activeConstraints;

    _statistics.setNumActivePlConstraints( activeConstraints );
    _statistics.setNumPlValidSplits( _numPlConstraintsDisabledByValidSplits );
    _statistics.setNumPlSMTSplits( _plConstraints.size() -
        activeConstraints - _numPlConstraintsDisabledByValidSplits );

    _statistics.incNumMainLoopIterations();

    struct timespec end = TimeUtils::sampleMicro();
    _statistics.addTimeForStatistics( TimeUtils::timePassed( start, end ) );
}

void Engine2::performConstraintFixingStep()
{
    // Statistics
    _statistics.incNumConstraintFixingSteps();
    struct timespec start = TimeUtils::sampleMicro();

    // Select a violated constraint as the target
    selectViolatedPlConstraint();

    // Report the violated constraint to the SMT engine
    reportPlViolation();

    // Attempt to fix the constraint
    fixViolatedPlConstraintIfPossible();

    struct timespec end = TimeUtils::sampleMicro();
    _statistics.addTimeConstraintFixingSteps( TimeUtils::timePassed( start, end ) );
}

void Engine2::performSimplexStep()
{
    // Statistics
    _statistics.incNumSimplexSteps();
    struct timespec start = TimeUtils::sampleMicro();

    /*
      In order to increase numerical stability, we attempt to pick a
      "good" entering/leaving combination, by trying to avoid tiny pivot
      values. We do this as follows:

      1. Pick an entering variable according to the strategy in use.
      2. Find the entailed leaving variable.
      3. If the combination is bad, go back to (1) and find the
         next-best entering variable.
    */

    if ( _costFunctionManager->costFunctionInvalid() )
        _costFunctionManager->computeCoreCostFunction();
    else
        _costFunctionManager->adjustBasicCostAccuracy();

    DEBUG( {
        // Since we're performing a simplex step, there are out-of-bounds variables.
        // Therefore, if the cost function is fresh, it should not be zero.
        if ( _costFunctionManager->costFunctionJustComputed() )
        {
            const double* costFunction = _costFunctionManager->getCostFunction();
            unsigned size = _tableau->getN() - _tableau->getM();
            bool found = false;
            for ( unsigned i = 0; i < size; ++i )
            {
                if ( !FloatUtils::isZero( costFunction[i] ) )
                {
                    found = true;
                    break;
                }
            }

            if ( !found )
            {
                printf( "Error! Have OOB vars but cost function is zero.\n"
                        "Recomputing cost function. New one is:\n" );
                _costFunctionManager->computeCoreCostFunction();
                _costFunctionManager->dumpCostFunction();
                throw MarabouError( MarabouError::DEBUGGING_ERROR,
                                     "Have OOB vars but cost function is zero" );
            }
        }
        } );

    // Obtain all eligible entering varaibles
    List<unsigned> enteringVariableCandidates;
    _tableau->getEntryCandidates( enteringVariableCandidates );

    unsigned bestLeaving = 0;
    double bestChangeRatio = 0.0;
    Set<unsigned> excludedEnteringVariables;
    bool haveCandidate = false;
    unsigned bestEntering = 0;
    double bestPivotEntry = 0.0;
    unsigned tries = GlobalConfiguration::MAX_SIMPLEX_PIVOT_SEARCH_ITERATIONS;

    while ( tries > 0 )
    {
        --tries;

        // Attempt to pick the best entering variable from the available candidates
        if ( !_activeEntryStrategy->select( _tableau,
            enteringVariableCandidates,
            excludedEnteringVariables ) )
        {
            // No additional candidates can be found.
            break;
        }

        // We have a candidate!
        haveCandidate = true;

        // We don't want to re-consider this candidate in future
        // iterations
        excludedEnteringVariables.insert( _tableau->getEnteringVariableIndex() );

        // Pick a leaving variable
        _tableau->computeChangeColumn();
        _tableau->pickLeavingVariable();

        // A fake pivot always wins
        if ( _tableau->performingFakePivot() )
        {
            bestEntering = _tableau->getEnteringVariableIndex();
            bestLeaving = _tableau->getLeavingVariableIndex();
            bestChangeRatio = _tableau->getChangeRatio();
            memcpy( _work, _tableau->getChangeColumn(), sizeof( double ) * _tableau->getM() );
            break;
        }

        // Is the newly found pivot better than the stored one?
        unsigned leavingIndex = _tableau->getLeavingVariableIndex();
        double pivotEntry = FloatUtils::abs( _tableau->getChangeColumn()[leavingIndex] );
        if ( pivotEntry > bestPivotEntry )
        {
            bestEntering = _tableau->getEnteringVariableIndex();
            bestPivotEntry = pivotEntry;
            bestLeaving = leavingIndex;
            bestChangeRatio = _tableau->getChangeRatio();
            memcpy( _work, _tableau->getChangeColumn(), sizeof( double ) * _tableau->getM() );
        }

        // If the pivot is greater than the sought-after threshold, we
        // are done.
        if ( bestPivotEntry >= GlobalConfiguration::ACCEPTABLE_SIMPLEX_PIVOT_THRESHOLD )
            break;
        else
            _statistics.incNumSimplexPivotSelectionsIgnoredForStability();
    }

    // If we don't have any candidates, this simplex step has failed.
    if ( !haveCandidate )
    {
        if ( _tableau->getBasicAssignmentStatus() != ITableau::BASIC_ASSIGNMENT_JUST_COMPUTED )
        {
            // This failure might have resulted from a corrupt basic assignment.
            _tableau->computeAssignment();
            struct timespec end = TimeUtils::sampleMicro();
            _statistics.addTimeSimplexSteps( TimeUtils::timePassed( start, end ) );
            return;
        }
        else if ( !_costFunctionManager->costFunctionJustComputed() )
        {
            // This failure might have resulted from a corrupt cost function.
            ASSERT( _costFunctionManager->getCostFunctionStatus() ==
                ICostFunctionManager::COST_FUNCTION_UPDATED );
            _costFunctionManager->invalidateCostFunction();
            struct timespec end = TimeUtils::sampleMicro();
            _statistics.addTimeSimplexSteps( TimeUtils::timePassed( start, end ) );
            return;
        }
        else
        {
            // Cost function is fresh --- failure is real.
            struct timespec end = TimeUtils::sampleMicro();
            _statistics.addTimeSimplexSteps( TimeUtils::timePassed( start, end ) );
            throw InfeasibleQueryException();
        }
    }

    // Set the best choice in the tableau
    _tableau->setEnteringVariableIndex( bestEntering );
    _tableau->setLeavingVariableIndex( bestLeaving );
    _tableau->setChangeColumn( _work );
    _tableau->setChangeRatio( bestChangeRatio );

    bool fakePivot = _tableau->performingFakePivot();

    if ( !fakePivot &&
        bestPivotEntry < GlobalConfiguration::ACCEPTABLE_SIMPLEX_PIVOT_THRESHOLD )
    {
        /*
          Despite our efforts, we are stuck with a small pivot. If basis factorization
          isn't fresh, refresh it and terminate this step - perhaps in the next iteration
          a better pivot will be found
        */
        if ( !_tableau->basisMatrixAvailable() )
        {
            _tableau->refreshBasisFactorization();
            return;
        }

        _statistics.incNumSimplexUnstablePivots();
    }

    if ( !fakePivot )
    {
        _tableau->computePivotRow();
        _rowBoundTightener->examinePivotRow();
    }

    // Perform the actual pivot
    _activeEntryStrategy->prePivotHook( _tableau, fakePivot );
    _tableau->performPivot();
    _activeEntryStrategy->postPivotHook( _tableau, fakePivot );

    struct timespec end = TimeUtils::sampleMicro();
    _statistics.addTimeSimplexSteps( TimeUtils::timePassed( start, end ) );
}

void Engine2::fixViolatedPlConstraintIfPossible()
{
    List<PiecewiseLinearConstraint::Fix> fixes;

    if ( GlobalConfiguration::USE_SMART_FIX )
        fixes = _plConstraintToFix->getSmartFixes( _tableau );
    else
        fixes = _plConstraintToFix->getPossibleFixes();

    // First, see if we can fix without pivoting. We are looking for a fix concerning a
    // non-basic variable, that doesn't set that variable out-of-bounds.
    for ( const auto& fix : fixes )
    {
        if ( !_tableau->isBasic( fix._variable ) )
        {
            if ( _tableau->checkValueWithinBounds( fix._variable, fix._value ) )
            {
                _tableau->setNonBasicAssignment( fix._variable, fix._value, true );
                return;
            }
        }
    }

    // No choice, have to pivot. Look for a fix concerning a basic variable, that
    // doesn't set that variable out-of-bounds. If smart-fix is enabled and implemented,
    // we should probably not reach this point.
    bool found = false;
    auto it = fixes.begin();
    while ( !found && it != fixes.end() )
    {
        if ( _tableau->isBasic( it->_variable ) )
        {
            if ( _tableau->checkValueWithinBounds( it->_variable, it->_value ) )
            {
                found = true;
            }
        }
        if ( !found )
        {
            ++it;
        }
    }

    // If we couldn't find an eligible fix, give up
    if ( !found )
        return;

    PiecewiseLinearConstraint::Fix fix = *it;
    ASSERT( _tableau->isBasic( fix._variable ) );

    TableauRow row( _tableau->getN() - _tableau->getM() );
    _tableau->getTableauRow( _tableau->variableToIndex( fix._variable ), &row );

    // Pick the variable with the largest coefficient in this row for pivoting,
    // to increase numerical stability.
    unsigned bestCandidate = row._row[0]._var;
    double bestValue = FloatUtils::abs( row._row[0]._coefficient );

    unsigned n = _tableau->getN();
    unsigned m = _tableau->getM();
    for ( unsigned i = 1; i < n - m; ++i )
    {
        double contenderValue = FloatUtils::abs( row._row[i]._coefficient );
        if ( FloatUtils::gt( contenderValue, bestValue ) )
        {
            bestValue = contenderValue;
            bestCandidate = row._row[i]._var;
        }
    }

    if ( FloatUtils::isZero( bestValue ) )
    {
        // This can happen, e.g., if we have an equation x = 5, and is legal behavior.
        return;
    }

    // Switch between nonBasic and the variable we need to fix
    _tableau->setEnteringVariableIndex( _tableau->variableToIndex( bestCandidate ) );
    _tableau->setLeavingVariableIndex( _tableau->variableToIndex( fix._variable ) );

    // Make sure the change column and pivot row are up-to-date - strategies
    // such as projected steepest edge need these for their internal updates.
    _tableau->computeChangeColumn();
    _tableau->computePivotRow();

    _activeEntryStrategy->prePivotHook( _tableau, false );
    _tableau->performDegeneratePivot();
    _activeEntryStrategy->postPivotHook( _tableau, false );

    ASSERT( !_tableau->isBasic( fix._variable ) );
    _tableau->setNonBasicAssignment( fix._variable, fix._value, true );
}

bool Engine2::processInputQuery( InputQuery& inputQuery )
{
    return processInputQuery( inputQuery, GlobalConfiguration::PREPROCESS_INPUT_QUERY );
}

void Engine2::informConstraintsOfInitialBounds( InputQuery& inputQuery ) const
{
    for ( const auto& plConstraint : inputQuery.getPiecewiseLinearConstraints() )
    {
        List<unsigned> variables = plConstraint->getParticipatingVariables();
        for ( unsigned variable : variables )
        {
            plConstraint->notifyLowerBound( variable, inputQuery.getLowerBound( variable ) );
            plConstraint->notifyUpperBound( variable, inputQuery.getUpperBound( variable ) );
        }
    }
}

void Engine2::invokePreprocessor( const InputQuery& inputQuery, bool preprocess )
{
    if ( _verbosity > 0 )
        printf( "Engine2::processInputQuery: Input query (before preprocessing): "
            "%u equations, %u variables\n",
            inputQuery.getEquations().size(),
            inputQuery.getNumberOfVariables() );

    // If processing is enabled, invoke the preprocessor
    _preprocessingEnabled = preprocess;
    if ( _preprocessingEnabled )
        _preprocessedQuery = _preprocessor.preprocess
        ( inputQuery, GlobalConfiguration::PREPROCESSOR_ELIMINATE_VARIABLES );
    else
        _preprocessedQuery = inputQuery;

    if ( _verbosity > 0 )
        printf( "Engine2::processInputQuery: Input query (after preprocessing): "
            "%u equations, %u variables\n\n",
            _preprocessedQuery.getEquations().size(),
            _preprocessedQuery.getNumberOfVariables() );

    unsigned infiniteBounds = _preprocessedQuery.countInfiniteBounds();
    if ( infiniteBounds != 0 )
    {
        _exitCode = Engine2::ERROR;
        throw MarabouError( MarabouError::UNBOUNDED_VARIABLES_NOT_YET_SUPPORTED,
            Stringf( "Error! Have %u infinite bounds", infiniteBounds ).ascii() );
    }
}

void Engine2::printInputBounds( const InputQuery& inputQuery ) const
{
    printf( "Input bounds:\n" );
    for ( unsigned i = 0; i < inputQuery.getNumInputVariables(); ++i )
    {
        unsigned variable = inputQuery.inputVariableByIndex( i );
        double lb, ub;
        bool fixed = false;
        if ( _preprocessingEnabled )
        {
            // Fixed variables are easy: return the value they've been fixed to.
            if ( _preprocessor.variableIsFixed( variable ) )
            {
                fixed = true;
                lb = _preprocessor.getFixedValue( variable );
                ub = lb;
            }
            else
            {
                // Has the variable been merged into another?
                while ( _preprocessor.variableIsMerged( variable ) )
                    variable = _preprocessor.getMergedIndex( variable );

                // We know which variable to look for, but it may have been assigned
                // a new index, due to variable elimination
                variable = _preprocessor.getNewIndex( variable );

                lb = _preprocessedQuery.getLowerBound( variable );
                ub = _preprocessedQuery.getUpperBound( variable );
            }
        }
        else
        {
            lb = inputQuery.getLowerBound( variable );
            ub = inputQuery.getUpperBound( variable );
        }

        printf( "\tx%u: [%8.4lf, %8.4lf] %s\n", i, lb, ub, fixed ? "[FIXED]" : "" );
    }
    printf( "\n" );
}

void Engine2::storeEquationsInDegradationChecker()
{
    _degradationChecker.storeEquations( _preprocessedQuery );
}

double* Engine2::createConstraintMatrix()
{
    const List<Equation>& equations( _preprocessedQuery.getEquations() );
    unsigned m = equations.size();
    unsigned n = _preprocessedQuery.getNumberOfVariables();

    // Step 1: create a constraint matrix from the equations
    double* constraintMatrix = new double[n * m];
    if ( !constraintMatrix )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Engine2::constraintMatrix" );
    std::fill_n( constraintMatrix, n * m, 0.0 );

    unsigned equationIndex = 0;
    for ( const auto& equation : equations )
    {
        if ( equation._type != Equation::EQ )
        {
            _exitCode = Engine2::ERROR;
            throw MarabouError( MarabouError::NON_EQUALITY_INPUT_EQUATION_DISCOVERED );
        }

        for ( const auto& addend : equation._addends )
            constraintMatrix[equationIndex * n + addend._variable] = addend._coefficient;

        ++equationIndex;
    }

    return constraintMatrix;
}

void Engine2::removeRedundantEquations( const double* constraintMatrix )
{
    const List<Equation>& equations( _preprocessedQuery.getEquations() );
    unsigned m = equations.size();
    unsigned n = _preprocessedQuery.getNumberOfVariables();

    // Step 1: analyze the matrix to identify redundant rows
    AutoConstraintMatrixAnalyzer analyzer;
    analyzer->analyze( constraintMatrix, m, n );

    ENGINE_LOG( Stringf( "Number of redundant rows: %u out of %u",
        analyzer->getRedundantRows().size(), m ).ascii() );

    // Step 2: remove any equations corresponding to redundant rows
    Set<unsigned> redundantRows = analyzer->getRedundantRows();

    if ( !redundantRows.empty() )
    {
        _preprocessedQuery.removeEquationsByIndex( redundantRows );
        m = equations.size();
    }
}

void Engine2::selectInitialVariablesForBasis( const double* constraintMatrix, List<unsigned>& initialBasis, List<unsigned>& basicRows )
{
    /*
      This method permutes rows and columns in the constraint matrix (prior
      to the addition of auxiliary variables), in order to obtain a set of
      column that constitue a lower triangular matrix. The variables
      corresponding to the columns of this matrix join the initial basis.

      (It is possible that not enough variables are obtained this way, in which
      case the initial basis will have to be augmented later).
    */

    const List<Equation>& equations( _preprocessedQuery.getEquations() );

    unsigned m = equations.size();
    unsigned n = _preprocessedQuery.getNumberOfVariables();

    // Trivial case, or if a trivial basis is requested
    if ( ( m == 0 ) || ( n == 0 ) || GlobalConfiguration::ONLY_AUX_INITIAL_BASIS )
    {
        for ( unsigned i = 0; i < m; ++i )
            basicRows.append( i );

        return;
    }

    unsigned* nnzInRow = new unsigned[m];
    unsigned* nnzInColumn = new unsigned[n];

    std::fill_n( nnzInRow, m, 0 );
    std::fill_n( nnzInColumn, n, 0 );

    unsigned* columnOrdering = new unsigned[n];
    unsigned* rowOrdering = new unsigned[m];

    for ( unsigned i = 0; i < m; ++i )
        rowOrdering[i] = i;

    for ( unsigned i = 0; i < n; ++i )
        columnOrdering[i] = i;

    // Initialize the counters
    for ( unsigned i = 0; i < m; ++i )
    {
        for ( unsigned j = 0; j < n; ++j )
        {
            if ( !FloatUtils::isZero( constraintMatrix[i * n + j] ) )
            {
                ++nnzInRow[i];
                ++nnzInColumn[j];
            }
        }
    }

    DEBUG( {
            for ( unsigned i = 0; i < m; ++i )
            {
                ASSERT( nnzInRow[i] > 0 );
            }
        } );

    unsigned numExcluded = 0;
    unsigned numTriangularRows = 0;
    unsigned temp;

    while ( numExcluded + numTriangularRows < n )
    {
        // Do we have a singleton row?
        unsigned singletonRow = m;
        for ( unsigned i = numTriangularRows; i < m; ++i )
        {
            if ( nnzInRow[i] == 1 )
            {
                singletonRow = i;
                break;
            }
        }

        if ( singletonRow < m )
        {
            // Have a singleton row! Swap it to the top and update counters
            temp = rowOrdering[singletonRow];
            rowOrdering[singletonRow] = rowOrdering[numTriangularRows];
            rowOrdering[numTriangularRows] = temp;

            temp = nnzInRow[numTriangularRows];
            nnzInRow[numTriangularRows] = nnzInRow[singletonRow];
            nnzInRow[singletonRow] = temp;

            // Find the non-zero entry in the row and swap it to the diagonal
            DEBUG( bool foundNonZero = false );
            for ( unsigned i = numTriangularRows; i < n - numExcluded; ++i )
            {
                if ( !FloatUtils::isZero( constraintMatrix[rowOrdering[numTriangularRows] * n + columnOrdering[i]] ) )
                {
                    temp = columnOrdering[i];
                    columnOrdering[i] = columnOrdering[numTriangularRows];
                    columnOrdering[numTriangularRows] = temp;

                    temp = nnzInColumn[numTriangularRows];
                    nnzInColumn[numTriangularRows] = nnzInColumn[i];
                    nnzInColumn[i] = temp;

                    DEBUG( foundNonZero = true );
                    break;
                }
            }

            ASSERT( foundNonZero );

            // Remove all entries under the diagonal entry from the row counters
            for ( unsigned i = numTriangularRows + 1; i < m; ++i )
            {
                if ( !FloatUtils::isZero( constraintMatrix[rowOrdering[i] * n + columnOrdering[numTriangularRows]] ) )
                    --nnzInRow[i];
            }

            ++numTriangularRows;
        }
        else
        {
            // No singleton rows. Exclude the densest column
            unsigned maxDensity = nnzInColumn[numTriangularRows];
            unsigned column = numTriangularRows;

            for ( unsigned i = numTriangularRows; i < n - numExcluded; ++i )
            {
                if ( nnzInColumn[i] > maxDensity )
                {
                    maxDensity = nnzInColumn[i];
                    column = i;
                }
            }

            // Update the row counters to account for the excluded column
            for ( unsigned i = numTriangularRows; i < m; ++i )
            {
                double element = constraintMatrix[rowOrdering[i] * n + columnOrdering[column]];
                if ( !FloatUtils::isZero( element ) )
                {
                    ASSERT( nnzInRow[i] > 1 );
                    --nnzInRow[i];
                }
            }

            columnOrdering[column] = columnOrdering[n - 1 - numExcluded];
            nnzInColumn[column] = nnzInColumn[n - 1 - numExcluded];
            ++numExcluded;
        }
    }

    // Final basis: diagonalized columns + non-diagonalized rows
    List<unsigned> result;

    for ( unsigned i = 0; i < numTriangularRows; ++i )
    {
        initialBasis.append( columnOrdering[i] );
    }

    for ( unsigned i = numTriangularRows; i < m; ++i )
    {
        basicRows.append( rowOrdering[i] );
    }

    // Cleanup
    delete[] nnzInRow;
    delete[] nnzInColumn;
    delete[] columnOrdering;
    delete[] rowOrdering;
}

void Engine2::addAuxiliaryVariables()
{
    List<Equation>& equations( _preprocessedQuery.getEquations() );

    unsigned m = equations.size();
    unsigned originalN = _preprocessedQuery.getNumberOfVariables();
    unsigned n = originalN + m;

    _preprocessedQuery.setNumberOfVariables( n );

    // Add auxiliary variables to the equations and set their bounds
    unsigned count = 0;
    for ( auto& eq : equations )
    {
        unsigned auxVar = originalN + count;
        eq.addAddend( -1, auxVar );
        _preprocessedQuery.setLowerBound( auxVar, eq._scalar );
        _preprocessedQuery.setUpperBound( auxVar, eq._scalar );
        eq.setScalar( 0 );

        ++count;
    }
}

void Engine2::augmentInitialBasisIfNeeded( List<unsigned>& initialBasis, const List<unsigned>& basicRows )
{
    unsigned m = _preprocessedQuery.getEquations().size();
    unsigned n = _preprocessedQuery.getNumberOfVariables();
    unsigned originalN = n - m;

    if ( initialBasis.size() != m )
    {
        for ( const auto& basicRow : basicRows )
            initialBasis.append( basicRow + originalN );
    }
}

void Engine2::initializeTableau( const double* constraintMatrix, const List<unsigned>& initialBasis )
{
    const List<Equation>& equations( _preprocessedQuery.getEquations() );
    unsigned m = equations.size();
    unsigned n = _preprocessedQuery.getNumberOfVariables();

    _tableau->setDimensions( m, n );

    adjustWorkMemorySize();

    unsigned equationIndex = 0;
    for ( const auto& equation : equations )
    {
        _tableau->setRightHandSide( equationIndex, equation._scalar );
        ++equationIndex;
    }

    // Populate constriant matrix
    _tableau->setConstraintMatrix( constraintMatrix );

    for ( unsigned i = 0; i < n; ++i )
    {
        _tableau->setLowerBound( i, _preprocessedQuery.getLowerBound( i ) );
        _tableau->setUpperBound( i, _preprocessedQuery.getUpperBound( i ) );
    }

    _tableau->registerToWatchAllVariables( _rowBoundTightener );
    _tableau->registerResizeWatcher( _rowBoundTightener );

    _tableau->registerToWatchAllVariables( _constraintBoundTightener );
    _tableau->registerResizeWatcher( _constraintBoundTightener );

    _rowBoundTightener->setDimensions();
    _constraintBoundTightener->setDimensions();

    // Register the constraint bound tightener to all the PL constraints
    for ( auto& plConstraint : _preprocessedQuery.getPiecewiseLinearConstraints() )
        plConstraint->registerConstraintBoundTightener( _constraintBoundTightener );

    _plConstraints = _preprocessedQuery.getPiecewiseLinearConstraints();
    for ( const auto& constraint : _plConstraints )
    {
        constraint->registerAsWatcher( _tableau );
        constraint->setStatistics( &_statistics );
    }

    _tableau->initializeTableau( initialBasis );

    _costFunctionManager->initialize();
    _tableau->registerCostFunctionManager( _costFunctionManager );
    _activeEntryStrategy->initialize( _tableau );

    _statistics.setNumPlConstraints( _plConstraints.size() );
}

void Engine2::initializeNetworkLevelReasoning()
{
    _networkLevelReasoner = _preprocessedQuery.getNetworkLevelReasoner();

    if ( _networkLevelReasoner )
        _networkLevelReasoner->setTableau( _tableau );
}

bool Engine2::processInputQuery( InputQuery& inputQuery, bool preprocess )
{
    ENGINE_LOG( "processInputQuery starting\n" );

    struct timespec start = TimeUtils::sampleMicro();

    try
    {
        informConstraintsOfInitialBounds( inputQuery );
        invokePreprocessor( inputQuery, preprocess );
        if ( _verbosity > 0 )
            printInputBounds( inputQuery );

        double* constraintMatrix = createConstraintMatrix();
        removeRedundantEquations( constraintMatrix );

        // The equations have changed, recreate the constraint matrix
        delete[] constraintMatrix;
        constraintMatrix = createConstraintMatrix();

        List<unsigned> initialBasis;
        List<unsigned> basicRows;
        selectInitialVariablesForBasis( constraintMatrix, initialBasis, basicRows );
        addAuxiliaryVariables();
        augmentInitialBasisIfNeeded( initialBasis, basicRows );

        storeEquationsInDegradationChecker();

        // The equations have changed, recreate the constraint matrix
        delete[] constraintMatrix;
        constraintMatrix = createConstraintMatrix();

        initializeNetworkLevelReasoning();
        initializeTableau( constraintMatrix, initialBasis );

        if ( GlobalConfiguration::WARM_START )
            warmStart();

        delete[] constraintMatrix;

        if ( preprocess )
            performMILPSolverBoundedTightening();

        if ( _splittingStrategy == DivideStrategy::Auto )
        {
            _splittingStrategy =
                ( _preprocessedQuery.getInputVariables().size() <
                    GlobalConfiguration::INTERVAL_SPLITTING_THRESHOLD ) ?
                DivideStrategy::LargestInterval : DivideStrategy::ReLUViolation;
        }

        struct timespec end = TimeUtils::sampleMicro();
        _statistics.setPreprocessingTime( TimeUtils::timePassed( start, end ) );
    }
    catch ( const InfeasibleQueryException& )
    {
        ENGINE_LOG( "processInputQuery done\n" );

        struct timespec end = TimeUtils::sampleMicro();
        _statistics.setPreprocessingTime( TimeUtils::timePassed( start, end ) );

        _exitCode = Engine2::UNSAT;
        return false;
    }

    ENGINE_LOG( "processInputQuery done\n" );

    DEBUG( {
        // Initially, all constraints should be active
        for ( const auto& plc : _plConstraints )
            {
                ASSERT( plc->isActive() );
            }
        } );

    _smtStackManager.storeDebuggingSolution( _preprocessedQuery._debuggingSolution );
    return true;
}

void Engine2::performMILPSolverBoundedTightening()
{
    if ( _networkLevelReasoner && Options::get()->gurobiEnabled() )
    {
        _networkLevelReasoner->obtainCurrentBounds();

        switch ( Options::get()->getMILPSolverBoundTighteningType() )
        {
        case MILPSolverBoundTighteningType::LP_RELAXATION:
        case MILPSolverBoundTighteningType::LP_RELAXATION_INCREMENTAL:
            _networkLevelReasoner->lpRelaxationPropagation();
            break;

        case MILPSolverBoundTighteningType::MILP_ENCODING:
        case MILPSolverBoundTighteningType::MILP_ENCODING_INCREMENTAL:
            _networkLevelReasoner->MILPPropagation();
            break;
        case MILPSolverBoundTighteningType::ITERATIVE_PROPAGATION:
            _networkLevelReasoner->iterativePropagation();
            break;
        case MILPSolverBoundTighteningType::NONE:
            return;
        }
        List<Tightening> tightenings;
        _networkLevelReasoner->getConstraintTightenings( tightenings );

        for ( const auto& tightening : tightenings )
        {
            if ( tightening._type == Tightening::LB )
                _tableau->tightenLowerBound( tightening._variable, tightening._value );

            else if ( tightening._type == Tightening::UB )
                _tableau->tightenUpperBound( tightening._variable, tightening._value );
        }
    }
}

void Engine2::extractSolution( InputQuery& inputQuery )
{
    if ( _solveWithMILP )
    {
        extractSolutionFromGurobi( inputQuery );
        return;
    }

    for ( unsigned i = 0; i < inputQuery.getNumberOfVariables(); ++i )
    {
        if ( _preprocessingEnabled )
        {
            // Has the variable been merged into another?
            unsigned variable = i;
            while ( _preprocessor.variableIsMerged( variable ) )
                variable = _preprocessor.getMergedIndex( variable );

            // Fixed variables are easy: return the value they've been fixed to.
            if ( _preprocessor.variableIsFixed( variable ) )
            {
                inputQuery.setSolutionValue( i, _preprocessor.getFixedValue( variable ) );
                inputQuery.setLowerBound( i, _preprocessor.getFixedValue( variable ) );
                inputQuery.setUpperBound( i, _preprocessor.getFixedValue( variable ) );
                continue;
            }

            // We know which variable to look for, but it may have been assigned
            // a new index, due to variable elimination
            variable = _preprocessor.getNewIndex( variable );

            // Finally, set the assigned value
            inputQuery.setSolutionValue( i, _tableau->getValue( variable ) );
            inputQuery.setLowerBound( i, _tableau->getLowerBound( variable ) );
            inputQuery.setUpperBound( i, _tableau->getUpperBound( variable ) );
        }
        else
        {
            inputQuery.setSolutionValue( i, _tableau->getValue( i ) );
            inputQuery.setLowerBound( i, _tableau->getLowerBound( i ) );
            inputQuery.setUpperBound( i, _tableau->getUpperBound( i ) );
        }
    }
}

bool Engine2::allVarsWithinBounds() const
{
    return !_tableau->existsBasicOutOfBounds();
}

void Engine2::collectViolatedPlConstraints()
{
    _violatedPlConstraints.clear();
    for ( const auto& constraint : _plConstraints )
    {
        if ( constraint->isActive() && !constraint->satisfied() )
            _violatedPlConstraints.append( constraint );
    }
}

bool Engine2::allPlConstraintsHold()
{
    return _violatedPlConstraints.empty();
}

void Engine2::selectViolatedPlConstraint()
{
    ASSERT( !_violatedPlConstraints.empty() );

    _plConstraintToFix = _smtCoreSplitProvider->chooseViolatedConstraintForFixing( _violatedPlConstraints );

    ASSERT( _plConstraintToFix );
}

void Engine2::reportPlViolation()
{
    _smtCoreSplitProvider->reportViolatedConstraint( _plConstraintToFix );
}

void Engine2::storeTableauState( TableauState& state ) const
{
    _tableau->storeState( state );
}

void Engine2::restoreTableauState( const TableauState& state )
{
    ENGINE_LOG( "\tRestoring tableau state" );
    _tableau->restoreState( state );
}

void Engine2::storeState( EngineState& state, bool storeAlsoTableauState ) const
{
    if ( storeAlsoTableauState )
    {
        _tableau->storeState( state._tableauState );
        state._tableauStateIsStored = true;
    }
    else
        state._tableauStateIsStored = false;

    for ( const auto& constraint : _plConstraints )
        state._plConstraintToState[constraint] = constraint->duplicateConstraint();

    state._numPlConstraintsDisabledByValidSplits = _numPlConstraintsDisabledByValidSplits;
}

void Engine2::restoreState( const EngineState& state )
{
    ENGINE_LOG( "Restore state starting" );

    if ( !state._tableauStateIsStored )
        throw MarabouError( MarabouError::RESTORING_ENGINE_FROM_INVALID_STATE );

    ENGINE_LOG( "\tRestoring tableau state" );
    _tableau->restoreState( state._tableauState );

    ENGINE_LOG( "\tRestoring constraint states" );
    for ( auto& constraint : _plConstraints )
    {
        if ( !state._plConstraintToState.exists( constraint ) )
            throw MarabouError( MarabouError::MISSING_PL_CONSTRAINT_STATE );

        constraint->restoreState( state._plConstraintToState[constraint] );
    }

    _numPlConstraintsDisabledByValidSplits = state._numPlConstraintsDisabledByValidSplits;

    // Make sure the data structures are initialized to the correct size
    _rowBoundTightener->setDimensions();
    _constraintBoundTightener->setDimensions();
    adjustWorkMemorySize();
    _activeEntryStrategy->resizeHook( _tableau );
    _costFunctionManager->initialize();

    // Reset the violation counts in the SMT core
    _smtCoreSplitProvider->resetReportedViolations();
}

void Engine2::setNumPlConstraintsDisabledByValidSplits( unsigned numConstraints )
{
    _numPlConstraintsDisabledByValidSplits = numConstraints;
}

bool Engine2::attemptToMergeVariables( unsigned x1, unsigned x2 )
{
    /*
      First, we need to ensure that the variables are both non-basic.
    */

    unsigned n = _tableau->getN();
    unsigned m = _tableau->getM();

    if ( _tableau->isBasic( x1 ) )
    {
        TableauRow x1Row( n - m );
        _tableau->getTableauRow( _tableau->variableToIndex( x1 ), &x1Row );

        bool found = false;
        double bestCoefficient = 0.0;
        unsigned nonBasic = 0;
        for ( unsigned i = 0; i < n - m; ++i )
        {
            if ( x1Row._row[i]._var != x2 )
            {
                double contender = FloatUtils::abs( x1Row._row[i]._coefficient );
                if ( FloatUtils::gt( contender, bestCoefficient ) )
                {
                    found = true;
                    nonBasic = x1Row._row[i]._var;
                    bestCoefficient = contender;
                }
            }
        }

        if ( !found )
            return false;

        _tableau->setEnteringVariableIndex( _tableau->variableToIndex( nonBasic ) );
        _tableau->setLeavingVariableIndex( _tableau->variableToIndex( x1 ) );

        // Make sure the change column and pivot row are up-to-date - strategies
        // such as projected steepest edge need these for their internal updates.
        _tableau->computeChangeColumn();
        _tableau->computePivotRow();

        _activeEntryStrategy->prePivotHook( _tableau, false );
        _tableau->performDegeneratePivot();
        _activeEntryStrategy->postPivotHook( _tableau, false );
    }

    if ( _tableau->isBasic( x2 ) )
    {
        TableauRow x2Row( n - m );
        _tableau->getTableauRow( _tableau->variableToIndex( x2 ), &x2Row );

        bool found = false;
        double bestCoefficient = 0.0;
        unsigned nonBasic = 0;
        for ( unsigned i = 0; i < n - m; ++i )
        {
            if ( x2Row._row[i]._var != x1 )
            {
                double contender = FloatUtils::abs( x2Row._row[i]._coefficient );
                if ( FloatUtils::gt( contender, bestCoefficient ) )
                {
                    found = true;
                    nonBasic = x2Row._row[i]._var;
                    bestCoefficient = contender;
                }
            }
        }

        if ( !found )
            return false;

        _tableau->setEnteringVariableIndex( _tableau->variableToIndex( nonBasic ) );
        _tableau->setLeavingVariableIndex( _tableau->variableToIndex( x2 ) );

        // Make sure the change column and pivot row are up-to-date - strategies
        // such as projected steepest edge need these for their internal updates.
        _tableau->computeChangeColumn();
        _tableau->computePivotRow();

        _activeEntryStrategy->prePivotHook( _tableau, false );
        _tableau->performDegeneratePivot();
        _activeEntryStrategy->postPivotHook( _tableau, false );
    }

    // Both variables are now non-basic, so we can merge their columns
    _tableau->mergeColumns( x1, x2 );
    DEBUG( _tableau->verifyInvariants() );

    // Reset the entry strategy
    _activeEntryStrategy->initialize( _tableau );

    return true;
}

void Engine2::applySplit( const PiecewiseLinearCaseSplit& split )
{
    ENGINE_LOG( "" );
    ENGINE_LOG( "Applying a split. " );

    DEBUG( _tableau->verifyInvariants() );

    List<Tightening> bounds = split.getBoundTightenings();
    List<Equation> equations = split.getEquations();
    for ( auto& equation : equations )
    {
        /*
          First, adjust the equation if any variables have been merged.
          E.g., if the equation is x1 + x2 + x3 = 0, and x1 and x2 have been
          merged, the equation becomes 2x1 + x3 = 0
        */
        for ( auto& addend : equation._addends )
            addend._variable = _tableau->getVariableAfterMerging( addend._variable );

        List<Equation::Addend>::iterator addend;
        List<Equation::Addend>::iterator otherAddend;

        addend = equation._addends.begin();
        while ( addend != equation._addends.end() )
        {
            otherAddend = addend;
            ++otherAddend;

            while ( otherAddend != equation._addends.end() )
            {
                if ( otherAddend->_variable == addend->_variable )
                {
                    addend->_coefficient += otherAddend->_coefficient;
                    otherAddend = equation._addends.erase( otherAddend );
                }
                else
                    ++otherAddend;
            }

            if ( FloatUtils::isZero( addend->_coefficient ) )
                addend = equation._addends.erase( addend );
            else
                ++addend;
        }

        /*
          In the general case, we just add the new equation to the tableau.
          However, we also support a very common case: equations of the form
          x1 = x2, which are common, e.g., with ReLUs. For these equations we
          may be able to merge two columns of the tableau.
        */
        unsigned x1, x2;
        bool canMergeColumns =
            // Only if the flag is on
            GlobalConfiguration::USE_COLUMN_MERGING_EQUATIONS &&
            // Only if the equation has the correct form
            equation.isVariableMergingEquation( x1, x2 ) &&
            // And only if the variables are not out of bounds
            ( !_tableau->isBasic( x1 ) ||
                !_tableau->basicOutOfBounds( _tableau->variableToIndex( x1 ) ) )
            &&
            ( !_tableau->isBasic( x2 ) ||
                !_tableau->basicOutOfBounds( _tableau->variableToIndex( x2 ) ) );

        bool columnsSuccessfullyMerged = false;
        if ( canMergeColumns )
            columnsSuccessfullyMerged = attemptToMergeVariables( x1, x2 );

        if ( !columnsSuccessfullyMerged )
        {
            // General case: add a new equation to the tableau
            unsigned auxVariable = _tableau->addEquation( equation );
            _activeEntryStrategy->resizeHook( _tableau );

            switch ( equation._type )
            {
            case Equation::GE:
                bounds.append( Tightening( auxVariable, 0.0, Tightening::UB ) );
                break;

            case Equation::LE:
                bounds.append( Tightening( auxVariable, 0.0, Tightening::LB ) );
                break;

            case Equation::EQ:
                bounds.append( Tightening( auxVariable, 0.0, Tightening::LB ) );
                bounds.append( Tightening( auxVariable, 0.0, Tightening::UB ) );
                break;

            default:
                ASSERT( false );
                break;
            }
        }
    }

    adjustWorkMemorySize();

    _rowBoundTightener->resetBounds();
    _constraintBoundTightener->resetBounds();

    for ( auto& bound : bounds )
    {
        unsigned variable = _tableau->getVariableAfterMerging( bound._variable );

        if ( bound._type == Tightening::LB )
        {
            ENGINE_LOG( Stringf( "x%u: lower bound set to %.3lf", variable, bound._value ).ascii() );
            _tableau->tightenLowerBound( variable, bound._value );
        }
        else
        {
            ENGINE_LOG( Stringf( "x%u: upper bound set to %.3lf", variable, bound._value ).ascii() );
            _tableau->tightenUpperBound( variable, bound._value );
        }
    }

    DEBUG( _tableau->verifyInvariants() );
    ENGINE_LOG( "Done with split\n" );
}

void Engine2::applyAllRowTightenings()
{
    List<Tightening> rowTightenings;
    _rowBoundTightener->getRowTightenings( rowTightenings );

    for ( const auto& tightening : rowTightenings )
    {
        if ( tightening._type == Tightening::LB )
            _tableau->tightenLowerBound( tightening._variable, tightening._value );
        else
            _tableau->tightenUpperBound( tightening._variable, tightening._value );
    }
}

void Engine2::applyAllConstraintTightenings()
{
    List<Tightening> entailedTightenings;

    _constraintBoundTightener->getConstraintTightenings( entailedTightenings );

    for ( const auto& tightening : entailedTightenings )
    {
        _statistics.incNumBoundsProposedByPlConstraints();

        if ( tightening._type == Tightening::LB )
            _tableau->tightenLowerBound( tightening._variable, tightening._value );
        else
            _tableau->tightenUpperBound( tightening._variable, tightening._value );
    }
}

void Engine2::applyAllBoundTightenings()
{
    struct timespec start = TimeUtils::sampleMicro();

    applyAllRowTightenings();
    applyAllConstraintTightenings();

    struct timespec end = TimeUtils::sampleMicro();
    _statistics.addTimeForApplyingStoredTightenings( TimeUtils::timePassed( start, end ) );
}

bool Engine2::applyAllValidConstraintCaseSplits()
{
    struct timespec start = TimeUtils::sampleMicro();

    bool appliedSplit = false;
    for ( auto& constraint : _plConstraints )
        if ( applyValidConstraintCaseSplit( constraint ) )
            appliedSplit = true;

    struct timespec end = TimeUtils::sampleMicro();
    _statistics.addTimeForValidCaseSplit( TimeUtils::timePassed( start, end ) );

    return appliedSplit;
}

bool Engine2::applyValidConstraintCaseSplit( PiecewiseLinearConstraint* constraint )
{
    if ( constraint->isActive() && constraint->phaseFixed() )
    {
        String constraintString;
        constraint->dump( constraintString );
        ENGINE_LOG( Stringf( "A constraint has become valid. Dumping constraint: %s",
            constraintString.ascii() ).ascii() );

        constraint->setActiveConstraint( false );
        PiecewiseLinearCaseSplit validSplit = constraint->getValidCaseSplit();
        _smtStackManager.recordImpliedValidSplit( validSplit );
        applySplit( validSplit );
        ++_numPlConstraintsDisabledByValidSplits;

        return true;
    }

    return false;
}

bool Engine2::shouldCheckDegradation()
{
    return _statistics.getNumMainLoopIterations() %
        GlobalConfiguration::DEGRADATION_CHECKING_FREQUENCY == 0;
}

bool Engine2::highDegradation()
{
    struct timespec start = TimeUtils::sampleMicro();

    double degradation = _degradationChecker.computeDegradation( *_tableau );
    _statistics.setCurrentDegradation( degradation );

    bool result = FloatUtils::gt( degradation, GlobalConfiguration::DEGRADATION_THRESHOLD );

    struct timespec end = TimeUtils::sampleMicro();
    _statistics.addTimeForDegradationChecking( TimeUtils::timePassed( start, end ) );

    // Debug
    if ( result )
        printf( "High degradation found!\n" );
    //

    return result;
}

void Engine2::tightenBoundsOnConstraintMatrix()
{
    struct timespec start = TimeUtils::sampleMicro();

    if ( _statistics.getNumMainLoopIterations() %
        GlobalConfiguration::BOUND_TIGHTING_ON_CONSTRAINT_MATRIX_FREQUENCY == 0 )
    {
        _rowBoundTightener->examineConstraintMatrix( true );
        _statistics.incNumBoundTighteningOnConstraintMatrix();
    }

    struct timespec end = TimeUtils::sampleMicro();
    _statistics.addTimeForConstraintMatrixBoundTightening( TimeUtils::timePassed( start, end ) );
}

void Engine2::explicitBasisBoundTightening()
{
    struct timespec start = TimeUtils::sampleMicro();

    bool saturation = GlobalConfiguration::EXPLICIT_BOUND_TIGHTENING_UNTIL_SATURATION;

    _statistics.incNumBoundTighteningsOnExplicitBasis();

    switch ( GlobalConfiguration::EXPLICIT_BASIS_BOUND_TIGHTENING_TYPE )
    {
    case GlobalConfiguration::COMPUTE_INVERTED_BASIS_MATRIX:
        _rowBoundTightener->examineInvertedBasisMatrix( saturation );
        break;

    case GlobalConfiguration::USE_IMPLICIT_INVERTED_BASIS_MATRIX:
        _rowBoundTightener->examineImplicitInvertedBasisMatrix( saturation );
        break;

    case GlobalConfiguration::DISABLE_EXPLICIT_BASIS_TIGHTENING:
        break;
    }

    struct timespec end = TimeUtils::sampleMicro();
    _statistics.addTimeForExplicitBasisBoundTightening( TimeUtils::timePassed( start, end ) );
}

void Engine2::performPrecisionRestoration( PrecisionRestorer::RestoreBasics restoreBasics )
{
    struct timespec start = TimeUtils::sampleMicro();

    // debug
    double before = _degradationChecker.computeDegradation( *_tableau );
    //

    _precisionRestorer.restorePrecision( *this, *_tableau, _smtStackManager, restoreBasics );
    struct timespec end = TimeUtils::sampleMicro();
    _statistics.addTimeForPrecisionRestoration( TimeUtils::timePassed( start, end ) );

    _statistics.incNumPrecisionRestorations();
    _rowBoundTightener->clear();
    _constraintBoundTightener->resetBounds();

    // debug
    double after = _degradationChecker.computeDegradation( *_tableau );
    if ( _verbosity > 0 )
        printf( "Performing precision restoration. Degradation before: %.15lf. After: %.15lf\n",
            before,
            after );
    //

    if ( highDegradation() && ( restoreBasics == PrecisionRestorer::RESTORE_BASICS ) )
    {
        // First round, with basic restoration, still resulted in high degradation.
        // Try again!
        start = TimeUtils::sampleMicro();
        _precisionRestorer.restorePrecision( *this, *_tableau, _smtStackManager,
            PrecisionRestorer::DO_NOT_RESTORE_BASICS );
        end = TimeUtils::sampleMicro();
        _statistics.addTimeForPrecisionRestoration( TimeUtils::timePassed( start, end ) );
        _statistics.incNumPrecisionRestorations();

        _rowBoundTightener->clear();
        _constraintBoundTightener->resetBounds();

        // debug
        double afterSecond = _degradationChecker.computeDegradation( *_tableau );
        if ( _verbosity > 0 )
            printf( "Performing 2nd precision restoration. Degradation before: %.15lf. After: %.15lf\n",
                after,
                afterSecond );

        if ( highDegradation() )
            throw MarabouError( MarabouError::RESTORATION_FAILED_TO_RESTORE_PRECISION );
    }
}

void Engine2::storeInitialEngineState()
{
    if ( !_initialStateStored )
    {
        _precisionRestorer.storeInitialEngineState( *this );
        _initialStateStored = true;
    }
}

bool Engine2::basisRestorationNeeded() const
{
    return
        _basisRestorationRequired == Engine2::STRONG_RESTORATION_NEEDED ||
        _basisRestorationRequired == Engine2::WEAK_RESTORATION_NEEDED;
}

const Statistics* Engine2::getStatistics() const
{
    return &_statistics;
}

InputQuery* Engine2::getInputQuery()
{
    return &_preprocessedQuery;
}

void Engine2::checkBoundCompliancyWithDebugSolution()
{
    if ( _smtStackManager.checkSkewFromDebuggingSolution() )
    {
        // The stack is compliant, we should not have learned any non-compliant bounds
        for ( const auto& var : _preprocessedQuery._debuggingSolution )
        {
            // printf( "Looking at var %u\n", var.first );

            if ( FloatUtils::gt( _tableau->getLowerBound( var.first ), var.second, 1e-5 ) )
            {
                printf( "Error! The stack is compliant, but learned an non-compliant bound: "
                    "Solution for x%u is %.15lf, but learned lower bound %.15lf\n",
                    var.first,
                    var.second,
                    _tableau->getLowerBound( var.first ) );

                throw MarabouError( MarabouError::DEBUGGING_ERROR );
            }

            if ( FloatUtils::lt( _tableau->getUpperBound( var.first ), var.second, 1e-5 ) )
            {
                printf( "Error! The stack is compliant, but learned an non-compliant bound: "
                    "Solution for %u is %.15lf, but learned upper bound %.15lf\n",
                    var.first,
                    var.second,
                    _tableau->getUpperBound( var.first ) );

                throw MarabouError( MarabouError::DEBUGGING_ERROR );
            }
        }
    }
}

void Engine2::quitSignal()
{
    _quitRequested = true;
}

Engine2::ExitCode Engine2::getExitCode() const
{
    return _exitCode;
}

std::atomic_bool* Engine2::getQuitRequested()
{
    return &_quitRequested;
}

List<unsigned> Engine2::getInputVariables() const
{
    return _preprocessedQuery.getInputVariables();
}

void Engine2::addSplitProvider( std::shared_ptr<ISmtSplitProvider> const& splitProvider )
{
    _splitProvidersManager->subscribeSplitProvider( splitProvider );
}

void Engine2::performSymbolicBoundTightening()
{
    if ( ( !GlobalConfiguration::USE_SYMBOLIC_BOUND_TIGHTENING ) ||
        ( !_networkLevelReasoner ) )
        return;

    struct timespec start = TimeUtils::sampleMicro();

    unsigned numTightenedBounds = 0;

    // Step 1: tell the NLR about the current bounds
    _networkLevelReasoner->obtainCurrentBounds();

    // Step 2: perform SBT
    _networkLevelReasoner->symbolicBoundPropagation();

    // Step 3: Extract the bounds
    List<Tightening> tightenings;
    _networkLevelReasoner->getConstraintTightenings( tightenings );

    for ( const auto& tightening : tightenings )
    {

        if ( tightening._type == Tightening::LB &&
            FloatUtils::gt( tightening._value, _tableau->getLowerBound( tightening._variable ) ) )
        {
            _tableau->tightenLowerBound( tightening._variable, tightening._value );
            ++numTightenedBounds;
        }

        if ( tightening._type == Tightening::UB &&
            FloatUtils::lt( tightening._value, _tableau->getUpperBound( tightening._variable ) ) )
        {
            _tableau->tightenUpperBound( tightening._variable, tightening._value );
            ++numTightenedBounds;
        }
    }

    struct timespec end = TimeUtils::sampleMicro();
    _statistics.addTimeForSymbolicBoundTightening( TimeUtils::timePassed( start, end ) );
    _statistics.incNumTighteningsFromSymbolicBoundTightening( numTightenedBounds );
}

bool Engine2::shouldExitDueToTimeout( unsigned timeout ) const
{
    enum {
        MILLISECONDS_TO_SECONDS = 1000,
    };

    // A timeout value of 0 means no time limit
    if ( timeout == 0 )
        return false;

    return _statistics.getTotalTime() / MILLISECONDS_TO_SECONDS > timeout;
}

void Engine2::reset()
{
    resetStatistics();
    clearViolatedPLConstraints();
    resetSmtCore();
    resetBoundTighteners();
    resetExitCode();
}

void Engine2::resetStatistics()
{
    Statistics statistics;
    _statistics = statistics;
    _smtStackManager.setStatistics( &_statistics );
    _tableau->setStatistics( &_statistics );
    _rowBoundTightener->setStatistics( &_statistics );
    _constraintBoundTightener->setStatistics( &_statistics );
    _preprocessor.setStatistics( &_statistics );
    _activeEntryStrategy->setStatistics( &_statistics );

    _statistics.stampStartingTime();
}

void Engine2::clearViolatedPLConstraints()
{
    _violatedPlConstraints.clear();
    _plConstraintToFix = NULL;
}

void Engine2::resetSmtCore()
{
    _smtStackManager.reset();
}

void Engine2::resetExitCode()
{
    _exitCode = Engine2::NOT_DONE;
}

void Engine2::resetBoundTighteners()
{
    _constraintBoundTightener->resetBounds();
    _rowBoundTightener->resetBounds();
}

void Engine2::warmStart()
{
    // An NLR is required for a warm start
    if ( !_networkLevelReasoner )
        return;

    // First, choose an arbitrary assignment for the input variables
    unsigned numInputVariables = _preprocessedQuery.getNumInputVariables();
    unsigned numOutputVariables = _preprocessedQuery.getNumOutputVariables();

    if ( numInputVariables == 0 )
    {
        // Trivial case: all inputs are fixed, nothing to evaluate
        return;
    }

    double* inputAssignment = new double[numInputVariables];
    double* outputAssignment = new double[numOutputVariables];

    for ( unsigned i = 0; i < numInputVariables; ++i )
    {
        unsigned variable = _preprocessedQuery.inputVariableByIndex( i );
        inputAssignment[i] = _tableau->getLowerBound( variable );
    }

    // Evaluate the network for this assignment
    _networkLevelReasoner->evaluate( inputAssignment, outputAssignment );

    // Try to update as many variables as possible to match their assignment
    for ( unsigned i = 0; i < _networkLevelReasoner->getNumberOfLayers(); ++i )
    {
        const NLR::Layer* layer = _networkLevelReasoner->getLayer( i );
        unsigned layerSize = layer->getSize();
        const double* assignment = layer->getAssignment();

        for ( unsigned j = 0; j < layerSize; ++j )
        {
            if ( layer->neuronHasVariable( j ) )
            {
                unsigned variable = layer->neuronToVariable( j );
                if ( !_tableau->isBasic( variable ) )
                    _tableau->setNonBasicAssignment( variable, assignment[j], false );
            }
        }
    }

    // We did what we could for the non-basics; now let the tableau compute
    // the basic assignment
    _tableau->computeAssignment();

    delete[] outputAssignment;
    delete[] inputAssignment;
}

void Engine2::checkOverallProgress()
{
    // Get fresh statistics
    unsigned numVisitedStates = _statistics.getNumVisitedTreeStates();
    unsigned long long currentIteration = _statistics.getNumMainLoopIterations();

    if ( numVisitedStates > _lastNumVisitedStates )
    {
        // Progress has been made
        _lastNumVisitedStates = numVisitedStates;
        _lastIterationWithProgress = _statistics.getNumMainLoopIterations();
    }
    else
    {
        // No progress has been made. If it's been too long, request a restoration
        if ( currentIteration >
            _lastIterationWithProgress +
            GlobalConfiguration::MAX_ITERATIONS_WITHOUT_PROGRESS )
        {
            ENGINE_LOG( "checkOverallProgress detected cycling. Requesting a precision restoration" );
            _basisRestorationRequired = Engine2::STRONG_RESTORATION_NEEDED;
            _lastIterationWithProgress = currentIteration;
        }
    }
}

void Engine2::updateDirections()
{
    if ( GlobalConfiguration::USE_POLARITY_BASED_DIRECTION_HEURISTICS )
        for ( const auto& constraint : _plConstraints )
            if ( constraint->supportPolarity() &&
                constraint->isActive() && !constraint->phaseFixed() )
                constraint->updateDirection();
}

PiecewiseLinearConstraint* Engine2::pickSplitPLConstraintBasedOnPolarity()
{
    ENGINE_LOG( Stringf( "Using Polarity-based heuristics..." ).ascii() );

    if ( !_networkLevelReasoner )
        throw MarabouError( MarabouError::NETWORK_LEVEL_REASONER_NOT_AVAILABLE );

    List<PiecewiseLinearConstraint*> constraints =
        _networkLevelReasoner->getConstraintsInTopologicalOrder();

    Map<double, PiecewiseLinearConstraint*> scoreToConstraint;
    for ( auto& plConstraint : constraints )
    {
        if ( plConstraint->supportPolarity() &&
            plConstraint->isActive() && !plConstraint->phaseFixed() )
        {
            plConstraint->updateScoreBasedOnPolarity();
            scoreToConstraint[plConstraint->getScore()] = plConstraint;
            if ( scoreToConstraint.size() >=
                GlobalConfiguration::POLARITY_CANDIDATES_THRESHOLD )
                break;
        }
    }
    if ( scoreToConstraint.size() > 0 )
    {
        ENGINE_LOG( Stringf( "Score of the picked ReLU: %f",
            ( *scoreToConstraint.begin() ).first ).ascii() );
        return ( *scoreToConstraint.begin() ).second;
    }
    else
        return NULL;
}

PiecewiseLinearConstraint* Engine2::pickSplitPLConstraintBasedOnTopology()
{
    // We push the first unfixed ReLU in the topology order to the _candidatePlConstraints
    ENGINE_LOG( Stringf( "Using EarliestReLU heuristics..." ).ascii() );

    if ( !_networkLevelReasoner )
        throw MarabouError( MarabouError::NETWORK_LEVEL_REASONER_NOT_AVAILABLE );

    List<PiecewiseLinearConstraint*> constraints =
        _networkLevelReasoner->getConstraintsInTopologicalOrder();

    for ( auto& plConstraint : constraints )
    {
        if ( plConstraint->isActive() && !plConstraint->phaseFixed() )
            return plConstraint;
    }
    return NULL;
}

PiecewiseLinearConstraint* Engine2::pickSplitPLConstraintBasedOnIntervalWidth()
{
    // We push the first unfixed ReLU in the topology order to the _candidatePlConstraints
    ENGINE_LOG( Stringf( "Using LargestInterval heuristics..." ).ascii() );

    unsigned inputVariableWithLargestInterval = 0;
    double largestIntervalSoFar = 0;
    for ( const auto& variable : _preprocessedQuery.getInputVariables() )
    {
        double interval = _tableau->getUpperBound( variable ) -
            _tableau->getLowerBound( variable );
        if ( interval > largestIntervalSoFar )
        {
            inputVariableWithLargestInterval = variable;
            largestIntervalSoFar = interval;
        }
    }

    if ( largestIntervalSoFar == 0 )
        return NULL;
    else
    {
        double mid = ( _tableau->getLowerBound( inputVariableWithLargestInterval )
            + _tableau->getUpperBound( inputVariableWithLargestInterval )
            ) / 2;
        PiecewiseLinearCaseSplit s1;
        s1.storeBoundTightening( Tightening( inputVariableWithLargestInterval,
            mid, Tightening::UB ) );
        PiecewiseLinearCaseSplit s2;
        s2.storeBoundTightening( Tightening( inputVariableWithLargestInterval,
            mid, Tightening::LB ) );

        List<PiecewiseLinearCaseSplit> splits;
        splits.append( s1 );
        splits.append( s2 );
        _disjunctionForSplitting = std::unique_ptr<DisjunctionConstraint>
            ( new DisjunctionConstraint( splits ) );
        return _disjunctionForSplitting.get();
    }
}

PiecewiseLinearConstraint* Engine2::pickSplitPLConstraint()
{
    ENGINE_LOG( Stringf( "Picking a split PLConstraint..." ).ascii() );

    PiecewiseLinearConstraint* candidatePLConstraint = NULL;
    if ( _splittingStrategy == DivideStrategy::Polarity )
        candidatePLConstraint = pickSplitPLConstraintBasedOnPolarity();
    else if ( _splittingStrategy == DivideStrategy::EarliestReLU )
        candidatePLConstraint = pickSplitPLConstraintBasedOnTopology();
    else if ( _splittingStrategy == DivideStrategy::LargestInterval &&
        _smtStackManager.getStackDepth() %
        GlobalConfiguration::INTERVAL_SPLITTING_FREQUENCY == 0 )
        // Conduct interval splitting periodically.
        candidatePLConstraint = pickSplitPLConstraintBasedOnIntervalWidth();
    ENGINE_LOG( Stringf( ( candidatePLConstraint ?
        "Picked..." :
        "Unable to pick using the current strategy..." ) ).ascii() );

    return candidatePLConstraint;
}

PiecewiseLinearConstraint* Engine2::pickSplitPLConstraintSnC( SnCDivideStrategy strategy )
{
    PiecewiseLinearConstraint* candidatePLConstraint = NULL;
    if ( strategy == SnCDivideStrategy::Polarity )
        candidatePLConstraint = pickSplitPLConstraintBasedOnPolarity();
    else if ( strategy == SnCDivideStrategy::EarliestReLU )
        candidatePLConstraint = pickSplitPLConstraintBasedOnTopology();

    ENGINE_LOG( Stringf( "Done updating scores..." ).ascii() );
    ENGINE_LOG( Stringf( ( candidatePLConstraint ?
        "Picked..." :
        "Unable to pick using the current strategy..." ) ).ascii() );
    return candidatePLConstraint;
}

template <typename T>
struct empty_delete
{
    empty_delete() /* noexcept */
    {
    }

    template <typename U>
    empty_delete( const empty_delete<U>&,
        typename std::enable_if<
        std::is_convertible<U*, T*>::value
        >::type* = nullptr ) /* noexcept */
    {
    }

    void operator()( T* const ) const /* noexcept */
    {
        // do nothing
    }
};

bool Engine2::restoreSmtState( SmtState& smtState )
{
    try
    {
        ASSERT( _smtStackManager.getStackDepth() == 0 );

        // Step 1: all implied valid splits at root
        for ( auto& validSplit : smtState._impliedValidSplitsAtRoot )
        {
            applySplit( validSplit );
            _smtStackManager.recordImpliedValidSplit( validSplit );
        }

        tightenBoundsOnConstraintMatrix();
        applyAllBoundTightenings();
        // For debugging purposes
        checkBoundCompliancyWithDebugSolution();
        do
            performSymbolicBoundTightening();
        while ( applyAllValidConstraintCaseSplits() );

        // Step 2: replay the stack
        for ( auto stackEntry : smtState._stack )
        {
            _smtStackManager.replaySmtStackEntry( stackEntry );
            // Do all the bound propagation, and set ReLU constraints to inactive (at
            // least the one corresponding to the _activeSplit applied above.
            tightenBoundsOnConstraintMatrix();
            applyAllBoundTightenings();
            // For debugging purposes
            checkBoundCompliancyWithDebugSolution();
            do
                performSymbolicBoundTightening();
            while ( applyAllValidConstraintCaseSplits() );

        }
    }
    catch ( const InfeasibleQueryException& )
    {
        // The current query is unsat, and we need to pop.
        // If we're at level 0, the whole query is unsat.
        if ( !_smtStackManager.popSplit() )
        {
            if ( _verbosity > 0 )
            {
                printf( "\nEngine2::solve: UNSAT query\n" );
                _statistics.print();
            }
            _exitCode = Engine2::UNSAT;
            for ( PiecewiseLinearConstraint* p : _plConstraints )
                p->setActiveConstraint( true );
            return false;
        }
    }
    return true;
}

void Engine2::storeSmtState( SmtState& smtState )
{
    _smtStackManager.storeSmtState( smtState );
}

bool Engine2::solveWithMILPEncoding( unsigned timeoutInSeconds )
{
    // Apply bound tightening before handing to Gurobi
    if ( _tableau->basisMatrixAvailable() )
    {
        explicitBasisBoundTightening();
        applyAllBoundTightenings();
        applyAllValidConstraintCaseSplits();
    }

    do
    {
        performSymbolicBoundTightening();
    } while ( applyAllValidConstraintCaseSplits() );

    ENGINE_LOG( "Encoding the input query with Gurobi...\n" );
    _gurobi = std::unique_ptr<GurobiWrapper>( new GurobiWrapper() );
    _milpEncoder = std::unique_ptr<MILPEncoder>( new MILPEncoder( *_tableau ) );
    _milpEncoder->encodeInputQuery( *_gurobi, _preprocessedQuery );
    ENGINE_LOG( "Query encoded in Gurobi...\n" );

    double timeoutForGurobi = ( timeoutInSeconds == 0 ? FloatUtils::infinity()
        : timeoutInSeconds );
    ENGINE_LOG( Stringf( "Gurobi timeout set to %f\n", timeoutForGurobi ).ascii() )
        _gurobi->setTimeLimit( timeoutForGurobi );

    _gurobi->solve();

    if ( _gurobi->haveFeasibleSolution() )
    {
        _exitCode = IEngine::SAT;
        return true;
    }
    else if ( _gurobi->infeasbile() )
        _exitCode = IEngine::UNSAT;
    else if ( _gurobi->timeout() )
        _exitCode = IEngine::TIMEOUT;
    else
        throw NLRError( NLRError::UNEXPECTED_RETURN_STATUS_FROM_GUROBI );
    return false;
}

void Engine2::extractSolutionFromGurobi( InputQuery& inputQuery )
{
    ASSERT( _gurobi != nullptr );
    Map<String, double> assignment;
    double costOrObjective;
    _gurobi->extractSolution( assignment, costOrObjective );

    for ( unsigned i = 0; i < inputQuery.getNumberOfVariables(); ++i )
    {
        if ( _preprocessingEnabled )
        {
            // Has the variable been merged into another?
            unsigned variable = i;
            while ( _preprocessor.variableIsMerged( variable ) )
                variable = _preprocessor.getMergedIndex( variable );

            // Fixed variables are easy: return the value they've been fixed to.
            if ( _preprocessor.variableIsFixed( variable ) )
            {
                inputQuery.setSolutionValue( i, _preprocessor.getFixedValue( variable ) );
                inputQuery.setLowerBound( i, _preprocessor.getFixedValue( variable ) );
                inputQuery.setUpperBound( i, _preprocessor.getFixedValue( variable ) );
                continue;
            }

            // We know which variable to look for, but it may have been assigned
            // a new index, due to variable elimination
            variable = _preprocessor.getNewIndex( variable );

            // Finally, set the assigned value
            String variableName = _milpEncoder->getVariableNameFromVariable( variable );
            inputQuery.setSolutionValue( i, assignment[variableName] );
        }
        else
        {
            String variableName = _milpEncoder->getVariableNameFromVariable( i );
            inputQuery.setSolutionValue( i, assignment[variableName] );
        }
    }
}
