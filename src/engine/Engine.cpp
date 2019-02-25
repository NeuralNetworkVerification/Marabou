/*********************                                                        */
/*! \file Engine.cpp
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
#include "Engine.h"
#include "EngineState.h"
#include "FactTracker.h"
#include "InfeasibleQueryException.h"
#include "InputQuery.h"
#include "MStringf.h"
#include "MalformedBasisException.h"
#include "PiecewiseLinearConstraint.h"
#include "Preprocessor.h"
#include "ReluplexError.h"
#include "SmtCore.h"
#include "TableauRow.h"
#include "TimeUtils.h"

Engine::Engine()
    : _rowBoundTightener( *_tableau )
    , _symbolicBoundTightener( NULL )
    , _smtCore( this )
    , _numPlConstraintsDisabledByValidSplits( 0 )
    , _preprocessingEnabled( false )
    , _work( NULL )
    , _basisRestorationRequired( Engine::RESTORATION_NOT_NEEDED )
    , _basisRestorationPerformed( Engine::NO_RESTORATION_PERFORMED )
    , _costFunctionManager( _tableau )
    , _quitRequested( false )
    , _exitCode( Engine::NOT_DONE )
    , _constraintBoundTightener( *_tableau )
    , _numVisitedStatesAtPreviousRestoration( 0 )
{
    _factTracker.setStatistics( &_statistics );
    _smtCore.setStatistics( &_statistics );
    _smtCore.setFactTracker( &_factTracker );
    _tableau->setStatistics( &_statistics );
    _tableau->setFactTracker( &_factTracker );
    _rowBoundTightener->setStatistics( &_statistics );
    _rowBoundTightener->setFactTracker( &_factTracker );
    _constraintBoundTightener->setStatistics( &_statistics );
    _preprocessor.setStatistics( &_statistics );

    _activeEntryStrategy = _projectedSteepestEdgeRule;
    _activeEntryStrategy->setStatistics( &_statistics );

    _statistics.stampStartingTime();
}

Engine::~Engine()
{
    if ( _work )
    {
        delete[] _work;
        _work = NULL;
    }

    if ( _symbolicBoundTightener )
    {
        delete _symbolicBoundTightener;
        _symbolicBoundTightener = NULL;
    }
}

void Engine::adjustWorkMemorySize()
{
    if ( _work )
    {
        delete[] _work;
        _work = NULL;
    }

    _work = new double[_tableau->getM()];
    if ( !_work )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Engine::work" );
}

bool Engine::solve( unsigned timeoutInSeconds )
{
    SignalHandler::getInstance()->initialize();
    SignalHandler::getInstance()->registerClient( this );

    storeInitialEngineState();

    printf( "\nEngine::solve: Initial statistics\n" );
    mainLoopStatistics();
    printf( "\n---\n" );

    struct timespec mainLoopStart = TimeUtils::sampleMicro();
    while ( true )
    {
        for(unsigned var=0; var<_tableau->getN(); var++){
          ASSERT(_factTracker.hasFactAffectingBound(var, FactTracker::LB));
          ASSERT(_factTracker.hasFactAffectingBound(var, FactTracker::UB));
        }
        for(unsigned id=0; id<_tableau->getM(); id++){
          ASSERT(_factTracker.hasFactAffectingEquation(id));
        }
        struct timespec mainLoopEnd = TimeUtils::sampleMicro();
        _statistics.addTimeMainLoop( TimeUtils::timePassed( mainLoopStart, mainLoopEnd ) );
        mainLoopStart = mainLoopEnd;

        if ( shouldExitDueToTimeout( timeoutInSeconds ) )
        {
            printf( "\n\nEngine: quitting due to timeout...\n\n" );
            printf( "Final statistics:\n" );
            _statistics.print();

            _exitCode = Engine::TIMEOUT;
            _statistics.timeout();
            return false;
        }

        if ( _quitRequested )
        {
            printf( "\n\nEngine: quitting due to external request...\n\n" );
            printf( "Final statistics:\n" );
            _statistics.print();

            _exitCode = Engine::TIMEOUT;
            return false;
        }

        try
        {
            DEBUG( _tableau->verifyInvariants() );

            mainLoopStatistics();

            // If the basis has become malformed, we need to restore it
            if ( basisRestorationNeeded() )
            {
                if ( _basisRestorationRequired == Engine::STRONG_RESTORATION_NEEDED )
                {
                    performPrecisionRestoration( PrecisionRestorer::RESTORE_BASICS );
                    _basisRestorationPerformed = Engine::PERFORMED_STRONG_RESTORATION;
                }
                else
                {
                    performPrecisionRestoration( PrecisionRestorer::DO_NOT_RESTORE_BASICS );
                    _basisRestorationPerformed = Engine::PERFORMED_WEAK_RESTORATION;
                }

                _numVisitedStatesAtPreviousRestoration = _statistics.getNumVisitedTreeStates();
                _basisRestorationRequired = Engine::RESTORATION_NOT_NEEDED;
                continue;
            }

            // Restoration is not required
            _basisRestorationPerformed = Engine::NO_RESTORATION_PERFORMED;

            // Possible restoration due to preceision degradation
            if ( shouldCheckDegradation() && highDegradation() )
            {
                performPrecisionRestoration( PrecisionRestorer::RESTORE_BASICS );
                continue;
            }

            if ( _tableau->basisMatrixAvailable() )
                explicitBasisBoundTightening();

            // Perform any SmtCore-initiated case splits
            if ( _smtCore.needToSplit() )
            {
                _smtCore.performSplit();

                do
                {
                    performSymbolicBoundTightening();
                }
                while ( applyAllValidConstraintCaseSplits() );
                continue;
            }

            if ( !_tableau->allBoundsValid() )
            {
                // Some variable bounds are invalid, so the query is unsat
                unsigned failureVar = _tableau->getInvalidBoundsVariable();
                List<const Fact*> failureExplanations;
                ASSERT( ( _factTracker.hasFactAffectingBound( failureVar, FactTracker::LB ) ));
                failureExplanations.append( _factTracker.getFactAffectingBound( failureVar, FactTracker::LB ) );
                ASSERT ( _factTracker.hasFactAffectingBound( failureVar, FactTracker::UB ) );
                failureExplanations.append( _factTracker.getFactAffectingBound( failureVar, FactTracker::UB ) );
                throw InfeasibleQueryException( failureExplanations );
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
                        printf( "Before declaring SAT, recomputing...\n" );
                        // Make sure that the assignment is precise before declaring success
                        _tableau->computeAssignment();
                        continue;
                    }

                    printf( "\nEngine::solve: SAT assignment found\n" );
                    _statistics.print();
                    _exitCode = Engine::SAT;
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

            if ( _basisRestorationPerformed == Engine::NO_RESTORATION_PERFORMED )
            {
                if ( _numVisitedStatesAtPreviousRestoration != _statistics.getNumVisitedTreeStates() )
                {
                    // We've tried a strong restoration before, and it didn't work. Do a weak restoration
                    _basisRestorationRequired = Engine::WEAK_RESTORATION_NEEDED;
                }
                else
                {
                    _basisRestorationRequired = Engine::STRONG_RESTORATION_NEEDED;
                }
            }
            else if ( _basisRestorationPerformed == Engine::PERFORMED_STRONG_RESTORATION )
                _basisRestorationRequired = Engine::WEAK_RESTORATION_NEEDED;
            else
            {
                printf( "Engine: Cannot restore tableau!\n" );
                _exitCode = Engine::ERROR;
                return false;
            }
        }
        catch ( const InfeasibleQueryException & e)
        {
            auto explanations = e.getExplanations();
            auto splits = _factTracker.getConstraintsAndSplitsCausingFacts(explanations);
            List<PiecewiseLinearCaseSplit> soFar;
            _smtCore.allSplitsSoFar(soFar);
            printf("CONTRADICTION #facts=%u, #splits_causing=%u, #splits_excluding_implied=%u, #splits_including_implied=%d\n",
              explanations.size(),
              splits.size(),
              _statistics.getCurrentStackDepth(),
              soFar.size());
            // The current query is unsat, and we need to pop.
            // If we're at level 0, the whole query is unsat.
            if ( !_smtCore.popSplit(explanations) )
            {
                printf( "\nEngine::solve: UNSAT query\n" );
                _statistics.print();
                _exitCode = Engine::UNSAT;
                return false;
            }
        }
        catch ( ... )
        {
            _exitCode = Engine::ERROR;
            printf( "Engine: Unknown error!\n" );
            return false;
        }
    }
}

void Engine::mainLoopStatistics()
{
    struct timespec start = TimeUtils::sampleMicro();

    unsigned activeConstraints = 0;
    for ( const auto &constraint : _plConstraints )
        if ( constraint->isActive() )
            ++activeConstraints;

    _statistics.setNumActivePlConstraints( activeConstraints );
    _statistics.setNumPlValidSplits( _numPlConstraintsDisabledByValidSplits );
    _statistics.setNumPlSMTSplits( _plConstraints.size() -
                                   activeConstraints - _numPlConstraintsDisabledByValidSplits );

    if ( _statistics.getNumMainLoopIterations() % GlobalConfiguration::STATISTICS_PRINTING_FREQUENCY == 0 )
        _statistics.print();

    _statistics.incNumMainLoopIterations();

    struct timespec end = TimeUtils::sampleMicro();
    _statistics.addTimeForStatistics( TimeUtils::timePassed( start, end ) );
}

void Engine::performConstraintFixingStep()
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

void Engine::performSimplexStep()
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

    DEBUG({
            // Since we're performing a simplex step, there are out-of-bounds variables.
            // Therefore, if the cost function is fresh, it should not be zero.
            if ( _costFunctionManager->costFunctionJustComputed() )
            {
                const double *costFunction = _costFunctionManager->getCostFunction();
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
                    throw ReluplexError( ReluplexError::DEBUGGING_ERROR,
                                         "Have OOB vars but cost function is zero" );
                }
            }
        });

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
            memcpy( _work, _tableau->getChangeColumn(), sizeof(double) * _tableau->getM() );
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
            memcpy( _work, _tableau->getChangeColumn(), sizeof(double) * _tableau->getM() );
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
            List<const Fact*> explanation = _tableau->getExplanationsForSaturatedTableauRow();
            /*
              TODO: explanations probably include fact that created leavingIndex equation,
              and bounds of variables with non-zero coefficients in the equation

              Guy: I'm afraid it may not be so simple. AFAIK, the standard way for "explaining"
              simplex UNSATs is via Farkas' lemma. This will basically give you a set of equations that
              are responsible for the failure - and then you can grab the facts leading to those equations, plus
              the varibale bounds of all variables involved in those equations.

              For now, it may be easier to not have an explanation for this case - i.e., just backtrack.

              For now, we go over all rows to find a contradiction row. And backtrack for that row. This is not
              a minimal explanation, but it is a sufficient one.
            */

            throw InfeasibleQueryException( explanation );
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

void Engine::fixViolatedPlConstraintIfPossible()
{
    List<PiecewiseLinearConstraint::Fix> fixes;

    if ( GlobalConfiguration::USE_SMART_FIX )
        fixes = _plConstraintToFix->getSmartFixes( _tableau );
    else
        fixes = _plConstraintToFix->getPossibleFixes();

    // First, see if we can fix without pivoting. We are looking for a fix concerning a
    // non-basic variable, that doesn't set that variable out-of-bounds.
    for ( const auto &fix : fixes )
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

bool Engine::processInputQuery( InputQuery &inputQuery )
{
    return processInputQuery( inputQuery, GlobalConfiguration::PREPROCESS_INPUT_QUERY );
}

bool Engine::processInputQuery( InputQuery &inputQuery, bool preprocess )
{
    log( "processInputQuery starting\n" );

    try
    {
        struct timespec start = TimeUtils::sampleMicro();

        // Inform the PL constraints of the initial variable bounds
        for ( const auto &plConstraint : inputQuery.getPiecewiseLinearConstraints() )
        {
            List<unsigned> variables = plConstraint->getParticipatingVariables();
            for ( unsigned variable : variables )
            {
                plConstraint->notifyLowerBound( variable, inputQuery.getLowerBound( variable ) );
                plConstraint->notifyUpperBound( variable, inputQuery.getUpperBound( variable ) );
            }
        }

        printf( "Engine::processInputQuery: Input query (before preprocessing): "
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

        printf( "Engine::processInputQuery: Input query (after preprocessing): "
                "%u equations, %u variables\n\n",
                _preprocessedQuery.getEquations().size(),
                _preprocessedQuery.getNumberOfVariables() );

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
                    if ( _preprocessor.variableIsMerged( variable ) )
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

        unsigned infiniteBounds = _preprocessedQuery.countInfiniteBounds();
        if ( infiniteBounds != 0 )
        {
            _exitCode = Engine::ERROR;
            throw ReluplexError( ReluplexError::UNBOUNDED_VARIABLES_NOT_YET_SUPPORTED,
                                 Stringf( "Error! Have %u infinite bounds", infiniteBounds ).ascii() );
        }

        const List<Equation> &equations( _preprocessedQuery.getEquations() );
        unsigned m = equations.size();
        unsigned n = _preprocessedQuery.getNumberOfVariables();

        _degradationChecker.storeEquations( _preprocessedQuery );

        /*
          Process the returned equations, to detect any redundancies, i.e. equations that
          are a linear combination of other equations. Such equations can be removed
        */

        // Step 1: create a constraint matrix from the equations
        double *constraintMatrix = new double[n*m];
        if ( !constraintMatrix )
            throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Engine::constraintMatrix" );
        std::fill_n( constraintMatrix, n*m, 0.0 );

        unsigned equationIndex = 0;
        for ( const auto &equation : equations )
        {
            if ( equation._type != Equation::EQ )
            {
                _exitCode = Engine::ERROR;
                throw ReluplexError( ReluplexError::NON_EQUALITY_INPUT_EQUATIONS_NOT_YET_SUPPORTED );
            }

            for ( const auto &addend : equation._addends )
                constraintMatrix[equationIndex*n + addend._variable] = addend._coefficient;

            ++equationIndex;
        }

        // Step 2: analyze the matrix to identify redundant rows and independent columns
        AutoConstraintMatrixAnalyzer analyzer;
        analyzer->analyze( constraintMatrix, m, n );

        log( Stringf( "Number of redundant rows: %u out of %u",
                      analyzer->getRedundantRows().size(), m ) );

        // Step 3: remove any equations corresponding to redundant rows
        Set<unsigned> redundantRows = analyzer->getRedundantRows();

        if ( !redundantRows.empty() )
        {
            _preprocessedQuery.removeEquationsByIndex( redundantRows );
            m = equations.size();

            // Adjust A for the missing rows
            std::fill_n( constraintMatrix, n*m, 0.0 );

            unsigned equationIndex = 0;
            for ( const auto &equation : equations )
            {
                for ( const auto &addend : equation._addends )
                    constraintMatrix[equationIndex*n + addend._variable] = addend._coefficient;

                ++equationIndex;
            }
        }

        // Step 4: keep the independent columns for later basis initialization
        List<unsigned> independentColumns = analyzer->getIndependentColumns();
        ASSERT( independentColumns.size() == m );
        /* Done analyzing the constraint matrix for detetign dependencies */

        _tableau->setDimensions( m, n );

        adjustWorkMemorySize();

        equationIndex = 0;
        for ( const auto &equation : equations )
        {
            _tableau->setRightHandSide( equationIndex, equation._scalar );
            ++equationIndex;
        }

        _tableau->setConstraintMatrix( constraintMatrix );
        delete[] constraintMatrix;

        for ( unsigned i = 0; i < n; ++i )
        {
            _tableau->setLowerBound( i, _preprocessedQuery.getLowerBound( i ) );
            _tableau->setUpperBound( i, _preprocessedQuery.getUpperBound( i ) );
        }

        _tableau->registerToWatchAllVariables( _rowBoundTightener );
        _tableau->registerResizeWatcher( _rowBoundTightener );

        _tableau->registerToWatchAllVariables( _constraintBoundTightener );
        _tableau->registerResizeWatcher( _constraintBoundTightener );

        // Placeholder: better constraint matrix analysis as part
        // of the preprocessing phase.
        // Register the constraint bound tightener to all the PL constraints
        for ( auto &plConstraint : _preprocessedQuery.getPiecewiseLinearConstraints() )
            plConstraint->registerConstraintBoundTightener( _constraintBoundTightener );

        _plConstraints = _preprocessedQuery.getPiecewiseLinearConstraints();
        for ( const auto &constraint : _plConstraints )
        {
            constraint->registerAsWatcher( _tableau );
            constraint->setFactTracker( &_factTracker );
            constraint->setStatistics( &_statistics );
            _plConstraintFromID[constraint->getID()] = constraint;
        }
        _tableau->initializeTableau( independentColumns );

        _costFunctionManager->initialize();
        _tableau->registerCostFunctionManager( _costFunctionManager );
        _activeEntryStrategy->initialize( _tableau );

        _statistics.setNumPlConstraints( _plConstraints.size() );

        _factTracker.initializeFromTableau(*_tableau);

        for(unsigned i=0; i<_tableau->getN(); i++){
          ASSERT(_factTracker.hasFactAffectingBound(i, FactTracker::LB));
          ASSERT(_factTracker.hasFactAffectingBound(i, FactTracker::UB));
        }
        for(unsigned var=0; var<_tableau->getN(); var++){
          ASSERT(_factTracker.hasFactAffectingBound(var, FactTracker::LB));
          ASSERT(_factTracker.hasFactAffectingBound(var, FactTracker::UB));
        }
        for(unsigned id=0; id<_tableau->getM(); id++){
          ASSERT(_factTracker.hasFactAffectingEquation(id));
        }

        _rowBoundTightener->setDimensions();
        _constraintBoundTightener->setDimensions();

        if ( _preprocessedQuery._sbt )
            _symbolicBoundTightener = _preprocessedQuery._sbt;

        struct timespec end = TimeUtils::sampleMicro();
        _statistics.setPreprocessingTime( TimeUtils::timePassed( start, end ) );

        log( "processInputQuery done\n" );
    }
    catch ( const InfeasibleQueryException & )
    {
        _exitCode = Engine::UNSAT;
        return false;
    }

    _smtCore.storeDebuggingSolution( _preprocessedQuery._debuggingSolution );

    return true;
}

void Engine::extractSolution( InputQuery &inputQuery )
{
    for ( unsigned i = 0; i < inputQuery.getNumberOfVariables(); ++i )
    {
        if ( _preprocessingEnabled )
        {
            // Fixed variables are easy: return the value they've been fixed to.
            if ( _preprocessor.variableIsFixed( i ) )
            {
                inputQuery.setSolutionValue( i, _preprocessor.getFixedValue( i ) );
                continue;
            }

            // Has the variable been merged into another?
            unsigned variable = i;
            if ( _preprocessor.variableIsMerged( i ) )
                variable = _preprocessor.getMergedIndex( i );

            // We know which variable to look for, but it may have been assigned
            // a new index, due to variable elimination
            unsigned finalIndex = _preprocessor.getNewIndex( variable );

            // Finally, set the assigned value
            inputQuery.setSolutionValue( i, _tableau->getValue( finalIndex ) );
        }
        else
        {
            inputQuery.setSolutionValue( i, _tableau->getValue( i ) );
        }
    }
}

bool Engine::allVarsWithinBounds() const
{
    return !_tableau->existsBasicOutOfBounds();
}

void Engine::collectViolatedPlConstraints()
{
    _violatedPlConstraints.clear();
    for ( const auto &constraint : _plConstraints )
    {
        if ( constraint->isActive() && !constraint->satisfied() )
            _violatedPlConstraints.append( constraint );
    }
}

bool Engine::allPlConstraintsHold()
{
    return _violatedPlConstraints.empty();
}

void Engine::selectViolatedPlConstraint()
{
    ASSERT( !_violatedPlConstraints.empty() );

    _plConstraintToFix = _smtCore.chooseViolatedConstraintForFixing( _violatedPlConstraints );

    ASSERT( _plConstraintToFix );
}

void Engine::reportPlViolation()
{
    _smtCore.reportViolatedConstraint( _plConstraintToFix );
}

void Engine::storeState( EngineState &state, bool storeAlsoTableauState ) const
{
    if ( storeAlsoTableauState )
    {
        _tableau->storeState( state._tableauState );
        state._tableauStateIsStored = true;
    }
    else
        state._tableauStateIsStored = false;

    for ( const auto &constraint : _plConstraints )
        state._plConstraintToState[constraint] = constraint->duplicateConstraint();

    state._numPlConstraintsDisabledByValidSplits = _numPlConstraintsDisabledByValidSplits;
}

void Engine::restoreState( const EngineState &state )
{
    log( "Restore state starting" );

    if ( !state._tableauStateIsStored )
        throw ReluplexError( ReluplexError::RESTORING_ENGINE_FROM_INVALID_STATE );

    log( "\tRestoring tableau state" );
    _tableau->restoreState( state._tableauState );

    log( "\tRestoring constraint states" );
    for ( auto &constraint : _plConstraints )
    {
        if ( !state._plConstraintToState.exists( constraint ) )
            throw ReluplexError( ReluplexError::MISSING_PL_CONSTRAINT_STATE );

        constraint->restoreState( state._plConstraintToState[constraint] );
    }

    _numPlConstraintsDisabledByValidSplits = state._numPlConstraintsDisabledByValidSplits;

    // Make sure the data structures are initialized to the correct size
    for(unsigned var=0; var<_tableau->getN(); var++){
      ASSERT(_factTracker.hasFactAffectingBound(var, FactTracker::LB));
      ASSERT(_factTracker.hasFactAffectingBound(var, FactTracker::UB));
    }
    for(unsigned id=0; id<_tableau->getM(); id++){
      ASSERT(_factTracker.hasFactAffectingEquation(id));
    }
    _rowBoundTightener->setDimensions();
    _constraintBoundTightener->setDimensions();
    adjustWorkMemorySize();
    _activeEntryStrategy->resizeHook( _tableau );
    _costFunctionManager->initialize();

    // Reset the violation counts in the SMT core
    _smtCore.resetReportedViolations();
}

void Engine::setNumPlConstraintsDisabledByValidSplits( unsigned numConstraints )
{
    _numPlConstraintsDisabledByValidSplits = numConstraints;
}

bool Engine::attemptToMergeVariables( unsigned x1, unsigned x2 )
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

void Engine::applySplit( const PiecewiseLinearCaseSplit &split )
{
    log( "" );
    log( "Applying a split. " );

    DEBUG( _tableau->verifyInvariants() );

    List<Tightening> bounds = split.getBoundTightenings();
    List<Equation> equations = split.getEquations();
    for ( auto &equation : equations )
    {
        /*
          First, adjust the equation if any variables have been merged.
          E.g., if the equation is x1 + x2 + x3 = 0, and x1 and x2 have been
          merged, the equation becomes 2x1 + x3 = 0
        */
        for ( auto &addend : equation._addends )
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
        bool isMergingEquation = equation.isVariableMergingEquation( x1, x2 );
        bool canMergeColumns =
            // Only if the flag is on
            GlobalConfiguration::USE_COLUMN_MERGING_EQUATIONS &&
            // Only if the equation has the correct form
            isMergingEquation &&
            // And only if the variables are not out of bounds
            ( !_tableau->isBasic( x1 ) ||
              !_tableau->basicOutOfBounds( _tableau->variableToIndex( x1 ) ) )
            &&
            ( !_tableau->isBasic( x2 ) ||
              !_tableau->basicOutOfBounds( _tableau->variableToIndex( x2 ) ) );

        bool columnsSuccessfullyMerged = false;
        double x1_lb, x2_lb, x1_ub, x2_ub;
        if ( canMergeColumns )
        {
            x1_lb = _tableau->getLowerBound( x1 );
            x2_lb = _tableau->getLowerBound( x2 );
            x1_ub = _tableau->getUpperBound( x1 );
            x2_ub = _tableau->getUpperBound( x2 );
            columnsSuccessfullyMerged = attemptToMergeVariables( x1, x2 );
        }
        if ( columnsSuccessfullyMerged )
        {
          // MAJOR TODO: fact tracking when equations merged
          // probably; when x_1 and x_2 are merged and now referred to as x_1
          // create a new fact corresponding to the equation, whose explanation is
          // the old fact of x_1 and the old fact of x_2
          // Get bounds before merge
          ASSERT( _factTracker.hasFactAffectingBound( x2, FactTracker::UB ) );
          ASSERT( _factTracker.hasFactAffectingBound( x2, FactTracker::LB ) );

          const Fact* fact_x2_lb = _factTracker.getFactAffectingBound( x2, FactTracker::LB );
          const Fact* fact_x2_ub = _factTracker.getFactAffectingBound( x2, FactTracker::UB );

            if ( x1_lb < x2_lb )
            {
                Tightening x1_new_lb( x1, x2_lb, Tightening::LB );
                List<const Fact*> explanations = fact_x2_lb->getExplanations();
                for ( const Fact* explanation : explanations )
                {
                    x1_new_lb.addExplanation( explanation );
                }
                _factTracker.addBoundFact( x1, x1_new_lb );
            }

            if ( x1_ub > x2_ub )
            {
                Tightening x1_new_ub( x1, x2_ub, Tightening::UB );
                List<const Fact*> explanations = fact_x2_ub->getExplanations();
                for ( const Fact* explanation: explanations )
                {
                    x1_new_ub.addExplanation( explanation );
                }
                _factTracker.addBoundFact( x1, x1_new_ub );
            }

            // Even though x2 is merged, facts about x2 are still useful in fact tracker.
        }
        if ( !columnsSuccessfullyMerged )
        {
            // General case: add a new equation to the tableau
            for(unsigned var=0; var<_tableau->getN(); var++){
              ASSERT(_factTracker.hasFactAffectingBound(var, FactTracker::LB));
              ASSERT(_factTracker.hasFactAffectingBound(var, FactTracker::UB));
            }
            for(unsigned id=0; id<_tableau->getM(); id++){
              ASSERT(_factTracker.hasFactAffectingEquation(id));
            }

            unsigned auxVariable = _tableau->addEquation( equation );
            _activeEntryStrategy->resizeHook( _tableau );

            Tightening newAuxBound(0,0,Tightening::LB);
            switch ( equation._type )
            {
            // in general, new aux variable bound setCausingSplitInfo should be same
            // as current equation that is causing it
            case Equation::GE:
                newAuxBound = Tightening( auxVariable, 0.0, Tightening::UB );
                newAuxBound.setCausingSplitInfo( equation.getCausingConstraintID(), equation.getCausingSplitID(), equation.getSplitLevelCausing() );
                bounds.append( newAuxBound );
                break;

            case Equation::LE:
                newAuxBound = Tightening( auxVariable, 0.0, Tightening::LB );
                newAuxBound.setCausingSplitInfo( equation.getCausingConstraintID(), equation.getCausingSplitID(), equation.getSplitLevelCausing() );
                bounds.append( newAuxBound );
                break;

            case Equation::EQ:
                newAuxBound = Tightening( auxVariable, 0.0, Tightening::UB );
                newAuxBound.setCausingSplitInfo( equation.getCausingConstraintID(), equation.getCausingSplitID(), equation.getSplitLevelCausing() );
                bounds.append( newAuxBound );
                newAuxBound = Tightening( auxVariable, 0.0, Tightening::LB );
                newAuxBound.setCausingSplitInfo( equation.getCausingConstraintID(), equation.getCausingSplitID(), equation.getSplitLevelCausing() );
                bounds.append( newAuxBound );
                break;

            default:
                ASSERT( false );
                break;
            }
        }
    }

    adjustWorkMemorySize();
    for(unsigned var=0; var<_tableau->getN(); var++){
      ASSERT(_factTracker.hasFactAffectingBound(var, FactTracker::LB));
      ASSERT(_factTracker.hasFactAffectingBound(var, FactTracker::UB));
    }
    for(unsigned id=0; id<_tableau->getM(); id++){
      ASSERT(_factTracker.hasFactAffectingEquation(id));
    }
    _rowBoundTightener->resetBounds();
    _constraintBoundTightener->resetBounds();

    for ( auto &bound : bounds )
    {
        unsigned variable = _tableau->getVariableAfterMerging( bound._variable );
        if ( bound._type == Tightening::LB )
        {
            log( Stringf( "x%u: lower bound set to %.3lf", variable, bound._value ) );
            // add fact for this bound only if it is indeed tighter
            if( _tableau->tightenLowerBound( variable, bound._value ) )
                _factTracker.addBoundFact( variable, bound );
        }
        else
        {
            log( Stringf( "x%u: upper bound set to %.3lf", variable, bound._value ) );
            // add fact for this bound only if it is indeed tighter
            if( _tableau->tightenUpperBound( variable, bound._value ) )
                _factTracker.addBoundFact( variable, bound );
        }
    }

    DEBUG( _tableau->verifyInvariants() );
    log( "Done with split\n" );
}

void Engine::applyAllRowTightenings()
{
    List<Tightening> rowTightenings;
    _rowBoundTightener->getRowTightenings( rowTightenings );

    for ( const auto &tightening : rowTightenings )
    {
        if ( tightening._type == Tightening::LB )
        {
            if ( _tableau->tightenLowerBound( tightening._variable, tightening._value ) )
                _factTracker.addBoundFact( tightening._variable, tightening );
        }
        else
        {
            if( _tableau->tightenUpperBound( tightening._variable, tightening._value ) )
                _factTracker.addBoundFact( tightening._variable, tightening );
        }
    }
}

void Engine::applyAllConstraintTightenings()
{
    List<Tightening> entailedTightenings;

    _constraintBoundTightener->getConstraintTightenings( entailedTightenings );

    for ( const auto &tightening : entailedTightenings )
    {
        _statistics.incNumBoundsProposedByPlConstraints();

        if ( tightening._type == Tightening::LB )
        {
            if ( _tableau->tightenLowerBound( tightening._variable, tightening._value ) )
                _factTracker.addBoundFact( tightening._variable, tightening );
        }
        else
        {
            if( _tableau->tightenUpperBound( tightening._variable, tightening._value ) )
                _factTracker.addBoundFact( tightening._variable, tightening );
        }
    }
}

void Engine::applyAllBoundTightenings()
{
    struct timespec start = TimeUtils::sampleMicro();

    applyAllRowTightenings();
    applyAllConstraintTightenings();

    struct timespec end = TimeUtils::sampleMicro();
    _statistics.addTimeForApplyingStoredTightenings( TimeUtils::timePassed( start, end ) );
}

bool Engine::applyAllValidConstraintCaseSplits()
{
    struct timespec start = TimeUtils::sampleMicro();

    bool appliedSplit = false;
    for ( auto &constraint : _plConstraints )
        if ( applyValidConstraintCaseSplit( constraint ) )
            appliedSplit = true;

    struct timespec end = TimeUtils::sampleMicro();
    _statistics.addTimeForValidCaseSplit( TimeUtils::timePassed( start, end ) );

    return appliedSplit;
}

bool Engine::applyValidConstraintCaseSplit( PiecewiseLinearConstraint *constraint )
{
    if ( constraint->isActive() && constraint->phaseFixed() )
    {
        String constraintString;
        constraint->dump( constraintString );
        log( Stringf( "A constraint has become valid. Dumping constraint: %s",
                      constraintString.ascii() ) );

        constraint->setActiveConstraint( false );
        PiecewiseLinearCaseSplit validSplit = constraint->getValidCaseSplit();
        _smtCore.recordImpliedValidSplit( validSplit );
        applySplit( validSplit );
        ++_numPlConstraintsDisabledByValidSplits;

        return true;
    }

    return false;
}

bool Engine::shouldCheckDegradation()
{
    return _statistics.getNumMainLoopIterations() %
        GlobalConfiguration::DEGRADATION_CHECKING_FREQUENCY == 0 ;
}

bool Engine::highDegradation()
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

void Engine::tightenBoundsOnConstraintMatrix()
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

void Engine::explicitBasisBoundTightening()
{
    struct timespec start = TimeUtils::sampleMicro();

    bool saturation = GlobalConfiguration::EXPLICIT_BOUND_TIGHTENING_UNTIL_SATURATION;

    _statistics.incNumBoundTighteningsOnExplicitBasis();

    // Junyao: always use COMPUTE_INVERTED_BASIS_MATRIX for CDCL
    switch ( GlobalConfiguration::EXPLICIT_BASIS_BOUND_TIGHTENING_TYPE )
    {
    case GlobalConfiguration::COMPUTE_INVERTED_BASIS_MATRIX:
        _rowBoundTightener->examineInvertedBasisMatrix( saturation );
        break;

    case GlobalConfiguration::USE_IMPLICIT_INVERTED_BASIS_MATRIX:
        _rowBoundTightener->examineImplicitInvertedBasisMatrix( saturation );
        break;
    }

    struct timespec end = TimeUtils::sampleMicro();
    _statistics.addTimeForExplicitBasisBoundTightening( TimeUtils::timePassed( start, end ) );
}

void Engine::performPrecisionRestoration( PrecisionRestorer::RestoreBasics restoreBasics )
{
    struct timespec start = TimeUtils::sampleMicro();

    // debug
    double before = _degradationChecker.computeDegradation( *_tableau );
    //

    _precisionRestorer.restorePrecision( *this, *_tableau, _smtCore, restoreBasics );
    struct timespec end = TimeUtils::sampleMicro();
    _statistics.addTimeForPrecisionRestoration( TimeUtils::timePassed( start, end ) );

    _statistics.incNumPrecisionRestorations();
    for(unsigned var=0; var<_tableau->getN(); var++){
      ASSERT(_factTracker.hasFactAffectingBound(var, FactTracker::LB));
      ASSERT(_factTracker.hasFactAffectingBound(var, FactTracker::UB));
    }
    for(unsigned id=0; id<_tableau->getM(); id++){
      ASSERT(_factTracker.hasFactAffectingEquation(id));
    }
    _rowBoundTightener->resetBounds();
    _constraintBoundTightener->resetBounds();

    // debug
    double after = _degradationChecker.computeDegradation( *_tableau );
    printf( "Performing precision restoration. Degradation before: %.15lf. After: %.15lf\n",
            before,
            after );
    //

    if ( highDegradation() && ( restoreBasics == PrecisionRestorer::RESTORE_BASICS ) )
    {
        // First round, with basic restoration, still resulted in high degradation.
        // Try again!
        start = TimeUtils::sampleMicro();
        _precisionRestorer.restorePrecision( *this, *_tableau, _smtCore,
                                             PrecisionRestorer::DO_NOT_RESTORE_BASICS );
        end = TimeUtils::sampleMicro();
        _statistics.addTimeForPrecisionRestoration( TimeUtils::timePassed( start, end ) );
        _statistics.incNumPrecisionRestorations();
        for(unsigned var=0; var<_tableau->getN(); var++){
          ASSERT(_factTracker.hasFactAffectingBound(var, FactTracker::LB));
          ASSERT(_factTracker.hasFactAffectingBound(var, FactTracker::UB));
        }
        for(unsigned id=0; id<_tableau->getM(); id++){
          ASSERT(_factTracker.hasFactAffectingEquation(id));
        }
        _rowBoundTightener->resetBounds();
        _constraintBoundTightener->resetBounds();

        // debug
        double afterSecond = _degradationChecker.computeDegradation( *_tableau );
        printf( "Performing 2nd precision restoration. Degradation before: %.15lf. After: %.15lf\n",
                after,
                afterSecond );

        if ( highDegradation() )
            throw ReluplexError( ReluplexError::RESTORATION_FAILED_TO_RESTORE_PRECISION );
    }
}

void Engine::storeInitialEngineState()
{
    _precisionRestorer.storeInitialEngineState( *this );
}

bool Engine::basisRestorationNeeded() const
{
    return
        _basisRestorationRequired == Engine::STRONG_RESTORATION_NEEDED ||
        _basisRestorationRequired == Engine::WEAK_RESTORATION_NEEDED;
}

const Statistics *Engine::getStatistics() const
{
    return &_statistics;
}

Statistics *Engine::getStatisticsForTest()
{
    return &_statistics;
}

SmtCore *Engine::getSmtCoreForTest()
{
    return &_smtCore;
}

FactTracker *Engine::getFactTrackerForTest()
{
    return &_factTracker;
}

void Engine::checkAllBoundsValidForTest( unsigned &failureVar )
{
    ASSERT( !_tableau->allBoundsValid() );
    failureVar = _tableau->getInvalidBoundsVariable();
}

void Engine::examineConstraintMatrixForTest()
{
    _rowBoundTightener->examineConstraintMatrix( true );
}

void Engine::applyAllBoundTighteningsForTest()
{
    struct timespec start = TimeUtils::sampleMicro();

    applyAllRowTightenings();
    applyAllConstraintTightenings();

    struct timespec end = TimeUtils::sampleMicro();
    _statistics.addTimeForApplyingStoredTightenings( TimeUtils::timePassed( start, end ) );
}

void Engine::log( const String &message )
{
    if ( GlobalConfiguration::ENGINE_LOGGING )
        printf( "Engine: %s\n", message.ascii() );
}

void Engine::checkBoundCompliancyWithDebugSolution()
{
    if ( _smtCore.checkSkewFromDebuggingSolution() )
    {
        // The stack is compliant, we should not have learned any non-compliant bounds
        for ( const auto &var : _preprocessedQuery._debuggingSolution )
        {
            // printf( "Looking at var %u\n", var.first );

            if ( FloatUtils::gt( _tableau->getLowerBound( var.first ), var.second, 1e-5 ) )
            {
                printf( "Error! The stack is compliant, but learned an non-compliant bound: "
                        "Solution for %u is %.15lf, but learned lower bound %.15lf\n",
                        var.first,
                        var.second,
                        _tableau->getLowerBound( var.first ) );
                throw ReluplexError( ReluplexError::DEBUGGING_ERROR );
            }

            if ( FloatUtils::lt( _tableau->getUpperBound( var.first ), var.second, 1e-5 ) )
            {
                printf( "Error! The stack is compliant, but learned an non-compliant bound: "
                        "Solution for %u is %.15lf, but learned upper bound %.15lf\n",
                        var.first,
                        var.second,
                        _tableau->getUpperBound( var.first ) );

                throw ReluplexError( ReluplexError::DEBUGGING_ERROR );
            }
        }
    }
}

void Engine::quitSignal()
{
    _quitRequested = true;
}

Engine::ExitCode Engine::getExitCode() const
{
    return _exitCode;
}

void Engine::performSymbolicBoundTightening()
{
    if ( ( !GlobalConfiguration::USE_SYMBOLIC_BOUND_TIGHTENING ) ||
         ( !_symbolicBoundTightener ) )
        return;

    struct timespec start = TimeUtils::sampleMicro();

    unsigned numTightenedBounds = 0;

    // Clear any previously stored information
    _symbolicBoundTightener->clearReluStatuses();

    // Step 1: tell the SBT about input bounds; maybe they were tightened
    unsigned inputVariableIndex = 0;
    for ( const auto &inputVariable : _preprocessedQuery.getInputVariables() )
    {
        // We assume the input variables are the first variables
        if ( inputVariable != inputVariableIndex )
        {
            throw ReluplexError( ReluplexError::SYMBOLIC_BOUND_TIGHTENER_FAULTY_INPUT,
                                 Stringf( "Sanity check failed, input variable %u with unexpected index %u", inputVariableIndex, inputVariable ).ascii() );
        }
        ++inputVariableIndex;

        double min = _tableau->getLowerBound( inputVariable );
        double max = _tableau->getUpperBound( inputVariable );

        _symbolicBoundTightener->setInputLowerBound( inputVariable, min );
        _symbolicBoundTightener->setInputUpperBound( inputVariable, max );
    }

    // Step 2: tell the SBT about the state of the ReLU constraints
    for ( const auto &constraint : _plConstraints )
    {
        if ( !constraint->supportsSymbolicBoundTightening() )
            throw ReluplexError( ReluplexError::SYMBOLIC_BOUND_TIGHTENER_UNSUPPORTED_CONSTRAINT_TYPE );

        ReluConstraint *relu = (ReluConstraint *)constraint;
        unsigned b = relu->getB();
        SymbolicBoundTightener::NodeIndex nodeIndex = _symbolicBoundTightener->nodeIndexFromB( b );
        _symbolicBoundTightener->setReluStatus( nodeIndex._layer, nodeIndex._neuron, relu->getPhaseStatus() );
    }

    // Step 3: perfrom the bound tightening
    _symbolicBoundTightener->run();

    // Stpe 4: extract any tighter bounds that were discovered
    for ( const auto &pair : _symbolicBoundTightener->getNodeIndexToFMapping() )
    {
        unsigned layer = pair.first._layer;
        unsigned neuron = pair.first._neuron;
        unsigned var = pair.second;

        double lb = _symbolicBoundTightener->getLowerBound( layer, neuron );
        double ub = _symbolicBoundTightener->getUpperBound( layer, neuron );

        double currentLb = _tableau->getLowerBound( var );
        double currentUb = _tableau->getUpperBound( var );

        _tableau->tightenLowerBound( var, lb );
        _tableau->tightenUpperBound( var, ub );

        if ( FloatUtils::lt( ub, currentUb ) )
            ++numTightenedBounds;

        if ( FloatUtils::gt( lb, currentLb ) )
            ++numTightenedBounds;
    }

    struct timespec end = TimeUtils::sampleMicro();
    _statistics.addTimeForSymbolicBoundTightening( TimeUtils::timePassed( start, end ) );
    _statistics.incNumTighteningsFromSymbolicBoundTightening( numTightenedBounds );
}

bool Engine::shouldExitDueToTimeout( unsigned timeout ) const
{
    enum {
        MILLISECONDS_TO_SECONDS = 1000,
    };

    // A timeout value of 0 means no time limit
    if ( timeout == 0 )
        return false;

    return _statistics.getTotalTime() / MILLISECONDS_TO_SECONDS > timeout;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
