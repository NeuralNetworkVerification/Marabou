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
    , _numPlConstraintsDisbaledByValidSplits( 0 )
{
    _smtCore.setStatistics( &_statistics );
    _tableau->setStatistics( &_statistics );
    _boundTightener.setStatistics( &_statistics );

    // _activeEntryStrategy = &_nestedDantzigsRule;
    _activeEntryStrategy = &_steepestEdgeRule;
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

        // Apply any pending bound tightenings
        _boundTightener.tighten( _tableau );
        // TODO: apply any constraint-entailed bound tightening

        // Perform any pending case splits, valid or SmtCore-initiated
        applyAllValidConstraintCaseSplits();
        if ( _smtCore.needToSplit() )
            _smtCore.performSplit();

        // Compute the current assignment and basic status
        _tableau->computeAssignment();
        _tableau->computeBasicStatus();

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
            if ( !fixViolatedPlConstraint() )
            {
                _statistics.print();
                needToPop = true;
            }
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
    _statistics.setNumPlValidSplits( _numPlConstraintsDisbaledByValidSplits );
    _statistics.setNumPlSMTSplits( _plConstraints.size() -
                                   activeConstraints - _numPlConstraintsDisbaledByValidSplits );
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
    _tableau->computeD();
    _tableau->pickLeavingVariable();

    unsigned enteringVariable = _tableau->getEnteringVariable();

    // Perform the actual pivot
    _tableau->performPivot();

    // Tighten
    _boundTightener.deriveTightenings( _tableau, enteringVariable );

    timeval end = TimeUtils::sampleMicro();
    _statistics.addTimeSimplexSteps( TimeUtils::timePassed( start, end ) );
    return true;
}

bool Engine::fixViolatedPlConstraint()
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
            	return true;
			}
        }
    }

    // No choice, have to pivot
	List<PiecewiseLinearConstraint::Fix>::iterator it = fixes.begin();
    PiecewiseLinearConstraint::Fix fix = *fixes.begin();

	while ( it != fixes.end() && !_tableau->isBasic( fix._variable ) &&
			( !FloatUtils::gte( fix._value, _tableau->getLowerBound( fix._variable ) ) ||
              !FloatUtils::lte( fix._value, _tableau->getUpperBound( fix._variable ) ) ) )
	{
		++it;
		fix = *it;
	}

	//if( it == fixes.end() ) return false;

    ASSERT( _tableau->isBasic( fix._variable ) );

    TableauRow row( _tableau->getN() - _tableau->getM() );
    _tableau->getTableauRow( _tableau->variableToIndex( fix._variable ), &row );

	unsigned j = 0;
    while ( ( j < _tableau->getN() - _tableau->getM() ) )
		++j;

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

   /* for ( unsigned i = 0; i < _tableau->getM(); ++i )
   {
        printf( "Extracting tableau row %u\n", i );
        TableauRow row ( _tableau->getN() - _tableau->getM() );
        _tableau->getTableauRow( i, &row );
        row.dump();
    }*/
    ASSERT( done );

    // Switch between nonBasic and the variable we need to fix
    _tableau->performDegeneratePivot( _tableau->variableToIndex( nonBasic ),
                                      _tableau->variableToIndex( fix._variable ) );

    ASSERT( !_tableau->isBasic( fix._variable ) );
    _tableau->setNonBasicAssignment( fix._variable, fix._value );
    return true;
}

void Engine::processInputQuery( InputQuery &inputQuery )
{
    processInputQuery( inputQuery, GlobalConfiguration::PREPROCESS_INPUT_QUERY );
}

void Engine::processInputQuery( InputQuery &inputQuery, bool preprocess )
{
    if ( preprocess )
    {
        log( Stringf( "Number of infinite bounds in the input query before preprocessing: %u",
                      inputQuery.countInfiniteBounds() ) );
        Preprocessor().tightenBounds( inputQuery );
        log( Stringf( "Number of infinite bounds in the input query after preprocessing: %u",
                      inputQuery.countInfiniteBounds() ) );
    }

    if ( inputQuery.countInfiniteBounds() != 0 )
        throw ReluplexError( ReluplexError::UNBOUNDED_VARIABLES_NOT_YET_SUPPORTED );

    const List<Equation> equations( inputQuery.getEquations() );

    unsigned m = equations.size();
    unsigned n = inputQuery.getNumberOfVariables();
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
        _tableau->setLowerBound( i, inputQuery.getLowerBound( i ) );
        _tableau->setUpperBound( i, inputQuery.getUpperBound( i ) );
    }

    _plConstraints = inputQuery.getPiecewiseLinearConstraints();
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
        PiecewiseLinearConstraintState *constraintState = constraint->allocateState();
        constraint->storeState( *constraintState );
        state._plConstraintToState[constraint] = constraintState;
    }

    state._numPlConstraintsDisbaledByValidSplits = _numPlConstraintsDisbaledByValidSplits;

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
    for ( const auto &constraint : _plConstraints )
    {
        if ( !state._plConstraintToState.exists( constraint ) )
            throw ReluplexError( ReluplexError::MISSING_PL_CONSTRAINT_STATE );

        constraint->restoreState( *state._plConstraintToState[constraint] );
    }

    _numPlConstraintsDisbaledByValidSplits = state._numPlConstraintsDisbaledByValidSplits;

    FreshVariables::setNextVariable( state._nextAuxVariable );
}

void Engine::applySplit( const PiecewiseLinearCaseSplit &split )
{
    for ( const auto &equation : split.getEquations() )
        _tableau->addEquation( equation ); // addNewEquation( equation );

    List<Tightening> bounds = split.getBoundTightenings();
    for ( const auto &bound : bounds )
    {
        if ( bound._type == Tightening::LB )
            _tableau->tightenLowerBound( bound._variable, bound._value );
        else
            _tableau->tightenUpperBound( bound._variable, bound._value );
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
        ++_numPlConstraintsDisbaledByValidSplits;
    }
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
