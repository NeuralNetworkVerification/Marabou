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
#include "InputQuery.h"
#include "MStringf.h"
#include "PiecewiseLinearConstraint.h"
#include "Preprocessor.h"
#include "ReluplexError.h"
#include "TableauRow.h"
#include "TimeUtils.h"

Engine::Engine()
    : _smtCore( this )
    , _numPlConstraintsDisabledByValidSplits( 0 )
{
    _smtCore.setStatistics( &_statistics );
    _tableau->setStatistics( &_statistics );
    _boundTightener.setStatistics( &_statistics );

    // _activeEntryStrategy = &_nestedDantzigsRule;
    // _activeEntryStrategy = &_steepestEdgeRule;
    _activeEntryStrategy = &_projectedSteepestEdgeRule;
    // _activeEntryStrategy = &_dantzigsRule;
    // _activeEntryStrategy = &_blandsRule;
}

Engine::~Engine()
{
}

bool Engine::solve()
{
    while ( true )
    {
        mainLoopStatistics();
        checkDegradation();

        // Apply any pending bound tightenings
        _boundTightener.tighten( _tableau );

        // Apply constraint-entailed bound tightenings
        applyAllConstraintTightenings();

        // Perform any valid case splits
        applyAllValidConstraintCaseSplits();

        // Compute the current assignment and basic status
        _tableau->computeAssignment();
        _tableau->computeBasicStatus();

        // Perform any SmtCore-initiated case splits
        if ( _smtCore.needToSplit() )
        {
            _smtCore.performSplit();

            // TODO: We get wrong answers if we don't recompute. But, we also
            // want to compute before performSplit(), so that the corrent
            // assignment is stored with the state.
            _tableau->computeAssignment();
            _tableau->computeBasicStatus();
        }

        bool needToPop = false;
        if ( !_tableau->allBoundsValid() )
        {
            // Some variable bounds are invalid, so the query is unsat
            needToPop = true;
        }
        else if ( allVarsWithinBounds() )
        {
            // Check the status of the PL constraints
            collectViolatedPlConstraints();

            // If all constraints are satisfied, we are done
            if ( allPlConstraintsHold() )
            {
                _statistics.print();
                return true;
            }

            // We have violated piecewise-linear constraints.
            _statistics.incNumConstraintFixingSteps();

            // Select a violated constraint as the target
            selectViolatedPlConstraint();

            // Report the violated constraint to the SMT engine
            reportPlViolation();

            // Attempt to fix the constraint
            fixViolatedPlConstraintIfPossible();
        }
        else
        {
            // We have out-of-bounds variables.
            // If a simplex step fails, the query is unsat
            if ( !performSimplexStep() )
            {
                _statistics.print();
                needToPop = true;
            }
        }

        if ( needToPop )
        {
            // The current query is unsat, and we need to split.
            // If we're at level 0, the whole query is unsat.
            if ( !_smtCore.popSplit() )
                return false;
        }
    }
}

void Engine::mainLoopStatistics()
{
    if ( _statistics.getNumMainLoopIterations() % GlobalConfiguration::STATISTICS_PRINTING_FREQUENCY == 0 )
        _statistics.print();

    _statistics.incNumMainLoopIterations();

    unsigned activeConstraints = 0;
    for ( const auto &constraint : _plConstraints )
        if ( constraint->isActive() )
            ++activeConstraints;
    _statistics.setNumActivePlConstraints( activeConstraints );
    _statistics.setNumPlValidSplits( _numPlConstraintsDisabledByValidSplits );
    _statistics.setNumPlSMTSplits( _plConstraints.size() -
                                   activeConstraints - _numPlConstraintsDisabledByValidSplits );
}

bool Engine::performSimplexStep()
{
    // Debug
    // for ( unsigned i = 0; i < _tableau->getM(); ++i )
    // {
    //     printf( "Extracting tableau row %u\n", i );
    //     TableauRow row( _tableau->getN() - _tableau->getM() );
    //     _tableau->getTableauRow( i, &row );
    //     row.dump();
    // }
    //

    // Statistics
    _statistics.incNumSimplexSteps();
    timeval start = TimeUtils::sampleMicro();

    // Pick an entering variable
    if ( !_activeEntryStrategy->select( _tableau ) )
    {
        timeval end = TimeUtils::sampleMicro();
        _statistics.addTimeSimplexSteps( TimeUtils::timePassed( start, end ) );
        return false;
    }

    // Pick a leaving variable
    _tableau->computeChangeColumn();
    _tableau->pickLeavingVariable();

    bool fakePivot = _tableau->performingFakePivot();
    if ( !fakePivot )
        _tableau->computePivotRow();

    unsigned enteringVariable = _tableau->getEnteringVariable();

    // Perform the actual pivot
    _activeEntryStrategy->prePivotHook( _tableau, fakePivot );
    _tableau->performPivot();
    _activeEntryStrategy->postPivotHook( _tableau, fakePivot );

    // Tighten
    // TODO: reuse the already-computed row in bound tightener
    if ( !fakePivot )
        _boundTightener.deriveTightenings( _tableau, enteringVariable );

    timeval end = TimeUtils::sampleMicro();
    _statistics.addTimeSimplexSteps( TimeUtils::timePassed( start, end ) );
    return true;
}

void Engine::fixViolatedPlConstraintIfPossible()
{
    PiecewiseLinearConstraint *violated = NULL;
    for ( const auto &constraint : _plConstraints )
    {
        if ( !constraint->satisfied() )
            violated = constraint;
    }

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
            	_tableau->setNonBasicAssignment( fix._variable, fix._value );
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

	unsigned nonBasic;
    bool done = false;
    unsigned i = 0;
    while ( !done && ( i < _tableau->getN() - _tableau->getM() ) )
    {
        // TODO: numerical stability. Pick a good candidate.
        // TODO: guarantee that candidate does not participate in the
        // same PL constraint?
        if ( !FloatUtils::isZero( row._row[i]._coefficient ) )
        {
            done = true;
            nonBasic = row._row[i]._var;
        }
        ++i;
    }

    ASSERT( done );

    // Switch between nonBasic and the variable we need to fix
    _tableau->setEnteringVariable( _tableau->variableToIndex( nonBasic ) );
    _tableau->setLeavingVariable( _tableau->variableToIndex( fix._variable ) );

    _activeEntryStrategy->prePivotHook( _tableau, false );
    _tableau->performDegeneratePivot();
    _activeEntryStrategy->prePivotHook( _tableau, false );

    ASSERT( !_tableau->isBasic( fix._variable ) );
    _tableau->setNonBasicAssignment( fix._variable, fix._value );
}

void Engine::processInputQuery( InputQuery &inputQuery )
{
    processInputQuery( inputQuery, GlobalConfiguration::PREPROCESS_INPUT_QUERY );
}

void Engine::processInputQuery( InputQuery &inputQuery, bool preprocess )
{
	_processed = inputQuery;
    if ( preprocess )
    {
        log( Stringf( "Number of infinite bounds in the input query before preprocessing: %u",
                      inputQuery.countInfiniteBounds() ) );
		Preprocessor process( inputQuery );
		process.tightenEquationsAndPL();
		InputQuery preprocessed = process.getInputQuery();
        log( Stringf( "Number of infinite bounds in the input query after preprocessing: %u",
                      preprocessed.countInfiniteBounds() ) );
		_processed = preprocessed;
    }

    if ( _processed.countInfiniteBounds() != 0 )
        throw ReluplexError( ReluplexError::UNBOUNDED_VARIABLES_NOT_YET_SUPPORTED );

    _degradationChecker.storeEquations( _processed );

    const List<Equation> equations( _processed.getEquations() );

    unsigned m = equations.size();
    unsigned n = _processed.getNumberOfVariables();
    _tableau->setDimensions( m, n );

    // Current variables are [0,..,n-1], so the next variable is n.
    FreshVariables::setNextVariable( n );

    unsigned equationIndex = 0;
    for ( const auto &equation : equations )
    {
        _tableau->markAsBasic( equation._auxVariable );
        _tableau->setRightHandSide( equationIndex, equation._scalar );

        for ( const auto &addend : equation._addends )
            _tableau->setEntryValue( equationIndex, addend._variable, addend._coefficient );

        ++equationIndex;
    }

    for ( unsigned i = 0; i < n; ++i )
    {
        _tableau->setLowerBound( i, _processed.getLowerBound( i ) );
        _tableau->setUpperBound( i, _processed.getUpperBound( i ) );
    }

    _plConstraints = _processed.getPiecewiseLinearConstraints();
    for ( const auto &constraint : _plConstraints )
        constraint->registerAsWatcher( _tableau );

    _tableau->initializeTableau();
    _activeEntryStrategy->initialize( _tableau );

    _statistics.setNumPlConstraints( _plConstraints.size() );
}

void Engine::extractSolution( InputQuery &inputQuery )
{
    for ( unsigned i = 0; i < inputQuery.getNumberOfVariables(); ++i )
        inputQuery.setSolutionValue( i, _tableau->getValue( i ) );
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
    {
        state._plConstraintToState[constraint] = constraint->duplicateConstraint();
    }

    state._numPlConstraintsDisabledByValidSplits = _numPlConstraintsDisabledByValidSplits;

    state._nextAuxVariable = FreshVariables::getNextVariable();
    FreshVariables::setNextVariable( state._nextAuxVariable );
}

void Engine::restoreState( const EngineState &state )
{
    log( "Restore state starting" );

    _boundTightener.clearStoredTightenings();

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

    FreshVariables::setNextVariable( state._nextAuxVariable );
}

void Engine::applySplit( const PiecewiseLinearCaseSplit &split )
{
    log( "" );
    log( "Applying a split. " );

    List<Tightening> bounds = split.getBoundTightenings();
    List<Pair<Equation, PiecewiseLinearCaseSplit::EquationType> > equations = split.getEquations();
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
        {
            bounds.append( Tightening( auxVariable, 0.0, Tightening::UB ) );
        }
        if ( type != PiecewiseLinearCaseSplit::LE )
        {
            bounds.append( Tightening( auxVariable, 0.0, Tightening::LB ) );
        }
    }

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

    log( "Done with split\n" );
}

void Engine::applyAllConstraintTightenings()
{
    for ( auto &constraint : _plConstraints )
    {
        Queue<Tightening> &entailedTightenings = constraint->getEntailedTightenings();
        while ( !entailedTightenings.empty() )
        {
            entailedTightenings.peak().tighten( _tableau );
            entailedTightenings.pop();
            // TODO: do we want statistics to track this bound tightening?
        }
    }
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
        constraint->setActiveConstraint( false );
        applySplit( constraint->getValidCaseSplit() );
        ++_numPlConstraintsDisabledByValidSplits;

        String constraintString;
        constraint->dump( constraintString );
        log( Stringf( "A constraint has become valid. Dumping constraint: %s",
                      constraintString.ascii() ) );
    }
}

void Engine::checkDegradation()
{
    if ( _statistics.getNumMainLoopIterations() % GlobalConfiguration::DEGRADATION_CHECKING_FREQUENCY != 0 )
        return;

    double degradation = _degradationChecker.computeDegradation( *_tableau );
    _statistics.setCurrentDegradation( degradation );
}

void Engine::log( const String &message )
{
    if ( GlobalConfiguration::ENGINE_LOGGING )
        printf( "Engine: %s\n", message.ascii() );
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
