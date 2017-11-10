/*! \file Engine.cpp
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
#include "Engine.h"
#include "EngineState.h"
#include "FreshVariables.h"
#include "InfeasibleQueryException.h"
#include "InputQuery.h"
#include "MStringf.h"
#include "MalformedBasisException.h"
#include "PiecewiseLinearConstraint.h"
#include "Preprocessor.h"
#include "ReluplexError.h"
#include "TableauRow.h"
#include "TimeUtils.h"

Engine::Engine()
    : _smtCore( this )
    , _numPlConstraintsDisabledByValidSplits( 0 )
    , _preprocessingEnabled( false )
    , _work( NULL )
    , _restorationStatus( RESTORATION_NOT_NEEDED )
{
    _smtCore.setStatistics( &_statistics );
    _tableau->setStatistics( &_statistics );
    _rowBoundTightener->setStatistics( &_statistics );
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

bool Engine::solve()
{
    storeInitialEngineState();

    printf( "\nEngine::solve: Initial statistics\n" );
    mainLoopStatistics();
    printf( "\n---\n" );

    struct timespec mainLoopStart = TimeUtils::sampleMicro();
    while ( true )
    {
        struct timespec mainLoopEnd = TimeUtils::sampleMicro();
        _statistics.addTimeMainLoop( TimeUtils::timePassed( mainLoopStart, mainLoopEnd ) );
        mainLoopStart = mainLoopEnd;

        try
        {
            DEBUG( _tableau->verifyInvariants() );

            mainLoopStatistics();

            // Possible restoration due to degradation or malformed tableau
            checkDegradation();
            if ( _restorationStatus == Engine::RESTORATION_NEEDED )
                performPrecisionRestoration();
            else
                _restorationStatus = Engine::RESTORATION_NOT_NEEDED;

            if ( _tableau->basisMatrixAvailable() )
                explicitBasisBoundTightening();

            // Perform any SmtCore-initiated case splits
            if ( _smtCore.needToSplit() )
            {
                _smtCore.performSplit();
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

                // If all constraints are satisfied, we are done
                if ( allPlConstraintsHold() )
                {
                    printf( "\nEngine::solve: SAT assignment found\n" );
                    _statistics.print();
                    return true;
                }

                // We have violated piecewise-linear constraints.
                performConstraintFixingStep();

                // Finally, take this opporunity to tighten any bounds
                // and perform any valid case splits.
                tightenBoundsOnConstraintMatrix();
                applyAllBoundTightenings();
                applyAllValidConstraintCaseSplits();

                continue;
            }

            // We have out-of-bounds variables.
            performSimplexStep();
            continue;
        }
        catch ( const MalformedBasisException & )
        {
            if ( _restorationStatus == Engine::RESTORATION_JUST_PERFORMED )
                throw ReluplexError( ReluplexError::CANNOT_RESTORE_TABLEAU );

            _restorationStatus = Engine::RESTORATION_NEEDED;
        }
        catch ( const InfeasibleQueryException & )
        {
            // The current query is unsat, and we need to pop.
            // If we're at level 0, the whole query is unsat.
            if ( !_smtCore.popSplit() )
            {
                printf( "\nEngine::solve: UNSAT query\n" );
                _statistics.print();
                return false;
            }
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

    if ( _tableau->getCostFunctionStatus() == ITableau::COST_FUNCTION_INVALID )
        _tableau->computeCostFunction();

    bool haveCandidate = false;
    unsigned bestEntering = 0;
    double bestPivotEntry = 0.0;
    unsigned tries = GlobalConfiguration::MAX_SIMPLEX_PIVOT_SEARCH_ITERATIONS;
    Set<unsigned> excludedEnteringVariables;
    unsigned bestLeaving = 0;

    while ( tries > 0 )
    {
        --tries;

        // Attempt to pick the best entering variable from the available candidates
        if ( !_activeEntryStrategy->select( _tableau, excludedEnteringVariables ) )
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
            memcpy( _work, _tableau->getChangeColumn(), sizeof(double) * _tableau->getM() );
            break;
        }

        // Is the newly found pivot better than the stored one?
        unsigned leavingIndex = _tableau->getLeavingVariableIndex();
        double pivotEntry = FloatUtils::abs( _tableau->getChangeColumn()[leavingIndex] );
        if ( FloatUtils::gt( pivotEntry, bestPivotEntry ) )
        {
            bestEntering = _tableau->getEnteringVariableIndex();
            bestPivotEntry = pivotEntry;
            bestLeaving = leavingIndex;
            memcpy( _work, _tableau->getChangeColumn(), sizeof(double) * _tableau->getM() );
        }

        // If the pivot is greater than the sought-after threshold, we
        // are done.
        if ( FloatUtils::gte( bestPivotEntry, GlobalConfiguration::ACCEPTABLE_SIMPLEX_PIVOT_THRESHOLD ) )
            break;
        else
            _statistics.incNumSimplexPivotSelectionsIgnoredForStability();
    }

    // If we don't have any candidates, this simplex step has failed.
    if ( !haveCandidate )
    {
        if ( _tableau->getCostFunctionStatus() != ITableau::COST_FUNCTION_JUST_COMPUTED )
        {
            // This failure might have resulted from a corrupt cost function.
            ASSERT( _tableau->getCostFunctionStatus() == ITableau::COST_FUNCTION_UPDATED );
            _tableau->setCostFunctionStatus( ITableau::COST_FUNCTION_INVALID );
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

    bool fakePivot = _tableau->performingFakePivot();

    if ( !fakePivot &&
         FloatUtils::lt( bestPivotEntry, GlobalConfiguration::ACCEPTABLE_SIMPLEX_PIVOT_THRESHOLD ) )
        _statistics.incNumSimplexUnstablePivots();

    if ( !fakePivot )
        _tableau->computePivotRow();

    // Perform the actual pivot
    _activeEntryStrategy->prePivotHook( _tableau, fakePivot );
    _tableau->performPivot();
    _activeEntryStrategy->postPivotHook( _tableau, fakePivot );

    // Tighten
    if ( !fakePivot )
        _rowBoundTightener->examinePivotRow( _tableau );

    struct timespec end = TimeUtils::sampleMicro();
    _statistics.addTimeSimplexSteps( TimeUtils::timePassed( start, end ) );
}

void Engine::fixViolatedPlConstraintIfPossible()
{
    ASSERT( !_violatedPlConstraints.empty() );
    PiecewiseLinearConstraint *violated = *_violatedPlConstraints.begin();
    ASSERT( violated );

    List<PiecewiseLinearConstraint::Fix> fixes = violated->getPossibleFixes();

    // First, see if we can fix without pivoting
    for ( const auto &fix : fixes )
    {
        if ( !_tableau->isBasic( fix._variable ) )
        {
			if ( FloatUtils::gte( fix._value, _tableau->getLowerBound( fix._variable ) ) &&
                 FloatUtils::lte( fix._value, _tableau->getUpperBound( fix._variable ) ) )
			{
            	_tableau->setNonBasicAssignment( fix._variable, fix._value, true );
            	return;
			}
        }
    }

    // No choice, have to pivot
	List<PiecewiseLinearConstraint::Fix>::iterator it = fixes.begin();
	while ( it != fixes.end() && !_tableau->isBasic( it->_variable ) &&
			( !FloatUtils::gte( it->_value, _tableau->getLowerBound( it->_variable ) ) ||
              !FloatUtils::lte( it->_value, _tableau->getUpperBound( it->_variable ) ) ) )
	{
		++it;
	}

    // If we couldn't find an eligible fix, give up
    if ( it == fixes.end() )
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

    ASSERT( !FloatUtils::isZero( bestValue ) );

    // Switch between nonBasic and the variable we need to fix
    _tableau->setEnteringVariableIndex( _tableau->variableToIndex( bestCandidate ) );
    _tableau->setLeavingVariableIndex( _tableau->variableToIndex( fix._variable ) );

    // Make sure the change column and pivot row are up-to-date - strategies
    // such as projected steepest edge need these for their internal updates.
    _tableau->computeChangeColumn();
    _tableau->computePivotRow();

    _activeEntryStrategy->prePivotHook( _tableau, false );
    _tableau->performDegeneratePivot();
    _activeEntryStrategy->prePivotHook( _tableau, false );

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

        // If processing is enabled, invoke the preprocessor
        _preprocessingEnabled = preprocess;
        if ( _preprocessingEnabled )
            _preprocessedQuery = _preprocessor.preprocess
                ( inputQuery, GlobalConfiguration::PREPROCESSOR_ELIMINATE_VARIABLES );
        else
            _preprocessedQuery = inputQuery;

        unsigned infiniteBounds = _preprocessedQuery.countInfiniteBounds();
        if ( infiniteBounds != 0 )
            throw ReluplexError( ReluplexError::UNBOUNDED_VARIABLES_NOT_YET_SUPPORTED,
                                 Stringf( "Error! Have %u infinite bounds", infiniteBounds ).ascii() );

        _degradationChecker.storeEquations( _preprocessedQuery );

        const List<Equation> equations( _preprocessedQuery.getEquations() );

        unsigned m = equations.size();
        unsigned n = _preprocessedQuery.getNumberOfVariables();
        _tableau->setDimensions( m, n );

        adjustWorkMemorySize();

        // Current variables are [0,..,n-1], so the next variable is n.
        FreshVariables::setNextVariable( n );

        unsigned equationIndex = 0;
        for ( const auto &equation : equations )
        {
            _tableau->markAsBasic( equation._auxVariable );
            _tableau->setRightHandSide( equationIndex, equation._scalar );

            for ( const auto &addend : equation._addends )
                _tableau->setEntryValue( equationIndex, addend._variable, addend._coefficient );

            _tableau->assignIndexToBasicVariable( equation._auxVariable, equationIndex );
            ++equationIndex;
        }

        for ( unsigned i = 0; i < n; ++i )
        {
            _tableau->setLowerBound( i, _preprocessedQuery.getLowerBound( i ) );
            _tableau->setUpperBound( i, _preprocessedQuery.getUpperBound( i ) );
        }

        _tableau->registerToWatchAllVariables( _rowBoundTightener );

        _rowBoundTightener->initialize( _tableau );

        _plConstraints = _preprocessedQuery.getPiecewiseLinearConstraints();
        for ( const auto &constraint : _plConstraints )
        {
            constraint->registerAsWatcher( _tableau );
            constraint->setStatistics( &_statistics );
        }

        _tableau->initializeTableau();
        _activeEntryStrategy->initialize( _tableau );

        _statistics.setNumPlConstraints( _plConstraints.size() );

        struct timespec end = TimeUtils::sampleMicro();
        _statistics.setPreprocessingTime( TimeUtils::timePassed( start, end ) );

        log( "processInputQuery done\n" );
    }
    catch ( const InfeasibleQueryException & )
    {
        return false;
    }

    return true;
}

void Engine::extractSolution( InputQuery &inputQuery )
{
    for ( unsigned i = 0; i < inputQuery.getNumberOfVariables(); ++i )
    {
        if ( _preprocessingEnabled )
        {
            if ( _preprocessor.variableIsFixed( i ) )
                inputQuery.setSolutionValue( i, _preprocessor.getFixedValue( i ) );
            else
                inputQuery.setSolutionValue( i, _tableau->getValue( _preprocessor.getNewIndex( i ) ) );
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
        if ( constraint->isActive() && !constraint->satisfied() )
            _violatedPlConstraints.append( constraint );
}

bool Engine::allPlConstraintsHold()
{
    return _violatedPlConstraints.empty();
}

void Engine::selectViolatedPlConstraint()
{
    _plConstraintToFix = *_violatedPlConstraints.begin();
}

void Engine::reportPlViolation()
{
    _smtCore.reportViolatedConstraint( _plConstraintToFix );
}

void Engine::storeState( EngineState &state ) const
{
    _tableau->storeState( state._tableauState );
    for ( const auto &constraint : _plConstraints )
        state._plConstraintToState[constraint] = constraint->duplicateConstraint();

    state._numPlConstraintsDisabledByValidSplits = _numPlConstraintsDisabledByValidSplits;

    state._nextAuxVariable = FreshVariables::getNextVariable();
    FreshVariables::setNextVariable( state._nextAuxVariable );
}

void Engine::restoreState( const EngineState &state )
{
    log( "Restore state starting" );

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
    _rowBoundTightener->initialize( _tableau );
    adjustWorkMemorySize();
    _activeEntryStrategy->resizeHook( _tableau );

    // Reset the violation counts in the SMT core
    _smtCore.resetReportedViolations();

    FreshVariables::setNextVariable( state._nextAuxVariable );
}

void Engine::applySplit( const PiecewiseLinearCaseSplit &split )
{
    log( "" );
    log( "Applying a split. " );

    DEBUG( _tableau->verifyInvariants() );

    List<Tightening> bounds = split.getBoundTightenings();
    List<Pair<Equation, PiecewiseLinearCaseSplit::EquationType>> equations = split.getEquations();
    for ( auto &it : equations )
    {
        Equation equation = it.first();
        unsigned auxVariable = FreshVariables::getNextVariable();
        equation.addAddend( -1, auxVariable );
        equation.markAuxiliaryVariable( auxVariable );

        _tableau->addEquation( equation );
        _activeEntryStrategy->resizeHook( _tableau );

        PiecewiseLinearCaseSplit::EquationType type = it.second();
        if ( type != PiecewiseLinearCaseSplit::GE )
            bounds.append( Tightening( auxVariable, 0.0, Tightening::UB ) );
        if ( type != PiecewiseLinearCaseSplit::LE )
            bounds.append( Tightening( auxVariable, 0.0, Tightening::LB ) );
    }

    adjustWorkMemorySize();

    _rowBoundTightener->initialize( _tableau );

    for ( auto &bound : bounds )
    {
        if ( bound._type == Tightening::LB )
        {
            log( Stringf( "x%u: lower bound set to %.3lf", bound._variable, bound._value ) );
            _tableau->tightenLowerBound( bound._variable, bound._value );
        }
        else
        {
            log( Stringf( "x%u: upper bound set to %.3lf", bound._variable, bound._value ) );
            _tableau->tightenUpperBound( bound._variable, bound._value );
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
        _statistics.incNumBoundsProposedByRowTightener();

        if ( tightening._type == Tightening::LB )
            _tableau->tightenLowerBound( tightening._variable, tightening._value );
        else
            _tableau->tightenUpperBound( tightening._variable, tightening._value );
    }
}

void Engine::applyAllConstraintTightenings()
{
    List<Tightening> entailedTightenings;
    for ( auto &constraint : _plConstraints )
        constraint->getEntailedTightenings( entailedTightenings );

    for ( const auto &tightening : entailedTightenings )
    {
        _statistics.incNumBoundsProposedByPlConstraints();

        if ( tightening._type == Tightening::LB )
            _tableau->tightenLowerBound( tightening._variable, tightening._value );
        else
            _tableau->tightenUpperBound( tightening._variable, tightening._value );
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

void Engine::applyAllValidConstraintCaseSplits()
{
    for ( auto &constraint : _plConstraints )
        applyValidConstraintCaseSplit( constraint );
}

void Engine::applyValidConstraintCaseSplit( PiecewiseLinearConstraint *constraint )
{
    if ( constraint->isActive() && constraint->phaseFixed() )
    {
        struct timespec start = TimeUtils::sampleMicro();

        constraint->setActiveConstraint( false );
        PiecewiseLinearCaseSplit validSplit = constraint->getValidCaseSplit();
        _smtCore.registerImpliedValidSplit( validSplit );
        applySplit( validSplit );
        ++_numPlConstraintsDisabledByValidSplits;

        String constraintString;
        constraint->dump( constraintString );
        log( Stringf( "A constraint has become valid. Dumping constraint: %s",
                      constraintString.ascii() ) );

        struct timespec end = TimeUtils::sampleMicro();
        _statistics.addTimeForValidCaseSplit( TimeUtils::timePassed( start, end ) );
    }
}

void Engine::checkDegradation()
{
    struct timespec start = TimeUtils::sampleMicro();

    if ( _statistics.getNumMainLoopIterations() % GlobalConfiguration::DEGRADATION_CHECKING_FREQUENCY != 0 )
        return;

    double degradation = _degradationChecker.computeDegradation( *_tableau );
    _statistics.setCurrentDegradation( degradation );

    if ( FloatUtils::gt( degradation, GlobalConfiguration::DEGRADATION_THRESHOLD ) )
         _restorationStatus = Engine::RESTORATION_NEEDED;

    struct timespec end = TimeUtils::sampleMicro();
    _statistics.addTimeForDegradationChecking( TimeUtils::timePassed( start, end ) );
}

void Engine::tightenBoundsOnConstraintMatrix()
{
    struct timespec start = TimeUtils::sampleMicro();

    if ( _statistics.getNumMainLoopIterations() %
         GlobalConfiguration::BOUND_TIGHTING_ON_CONSTRAINT_MATRIX_FREQUENCY == 0 )
    {
        _rowBoundTightener->examineConstraintMatrix( _tableau, true );
        _statistics.incNumBoundTighteningOnConstraintMatrix();
    }

    struct timespec end = TimeUtils::sampleMicro();
    _statistics.addTimeForConstraintMatrixBoundTightening( TimeUtils::timePassed( start, end ) );
}

void Engine::explicitBasisBoundTightening()
{
    struct timespec start = TimeUtils::sampleMicro();

    if ( GlobalConfiguration::EXPLICIT_BASIS_BOUND_TIGHTENING_INVERT_BASIS )
        _rowBoundTightener->examineInvertedBasisMatrix( _tableau, false );
    else
        _rowBoundTightener->examineBasisMatrix( _tableau, false );

    struct timespec end = TimeUtils::sampleMicro();
    _statistics.addTimeForExplicitBasisBoundTightening( TimeUtils::timePassed( start, end ) );
}

void Engine::performPrecisionRestoration()
{
    // Two consecutive restorations are now allowed
    if ( _restorationStatus == Engine::RESTORATION_JUST_PERFORMED )
        throw ReluplexError( ReluplexError::CANNOT_RESTORE_TABLEAU );

    struct timespec start = TimeUtils::sampleMicro();
    _precisionRestorer.restorePrecision( *this, *_tableau, _smtCore );
    struct timespec end = TimeUtils::sampleMicro();
    _statistics.addTimeForPrecisionRestoration( TimeUtils::timePassed( start, end ) );

    _statistics.incNumPrecisionRestorations();

    _restorationStatus = Engine::RESTORATION_JUST_PERFORMED;
}

void Engine::storeInitialEngineState()
{
    _precisionRestorer.storeInitialEngineState( *this );
}

const Statistics *Engine::getStatistics() const
{
    return &_statistics;
}

void Engine::log( const String &message )
{
    if ( GlobalConfiguration::ENGINE_LOGGING )
        printf( "Engine: %s\n", message.ascii() );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
