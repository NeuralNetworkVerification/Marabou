#include "SmtStackManager.h"
#include "MarabouError.h"
#include "StdCompletion.h"
#include "MStringf.h"

SmtStackManager::SmtStackManager( IEngine* engine, std::shared_ptr<SplitProvidersManager> const& splitProvidersManager )
    : _statistics( NULL )
    , _engine( engine )
    , _stateId( 0 )
    , _splitProvidersManager( splitProvidersManager )
{
}

void SmtStackManager::reset() {
    for ( const auto& stackEntry : _stack )
    {
        delete stackEntry->_engineState;
        delete stackEntry;
    }

    _stack.clear();
    _stateId = 0;
}

void SmtStackManager::performSplit( PiecewiseLinearCaseSplit const& split ) {

    struct timespec start = TimeUtils::sampleMicro();

    if ( _statistics )
    {
        _statistics->incNumSplits();
        _statistics->incNumVisitedTreeStates();
    }

    // Obtain the current state of the engine
    EngineState* stateBeforeSplits = new EngineState;
    stateBeforeSplits->_stateId = _stateId;
    ++_stateId;
    _engine->storeState( *stateBeforeSplits, true );

    auto stackEntry = new SmtStackEntry;
    // Perform the first split: add bounds and equations
    _engine->applySplit( split );
    stackEntry->_activeSplit = split;
    stackEntry->_pastSplits.append( split );
    stackEntry->_engineState = stateBeforeSplits;

    _stack.append( stackEntry );
    if ( _statistics )
    {
        _statistics->setCurrentStackDepth( getStackDepth() );
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->addTimeSmtCore( TimeUtils::timePassed( start, end ) );
    }

    _splitProvidersManager->onSplitPerformed( SplitInfo( split ) );
}

void SmtStackManager::replaySmtStackEntry( SmtStackEntry* stackEntry )
{
    struct timespec start = TimeUtils::sampleMicro();

    if ( _statistics )
    {
        _statistics->incNumSplits();
        _statistics->incNumVisitedTreeStates();
    }

    // Obtain the current state of the engine
    EngineState* stateBeforeSplits = new EngineState;
    stateBeforeSplits->_stateId = _stateId;
    ++_stateId;
    _engine->storeState( *stateBeforeSplits, true );
    stackEntry->_engineState = stateBeforeSplits;

    // Apply all the splits
    _engine->applySplit( stackEntry->_activeSplit );
    for ( const auto& impliedSplit : stackEntry->_impliedValidSplits )
        _engine->applySplit( impliedSplit );

    auto stackEntryDup = stackEntry->duplicateSmtStackEntry();
    _stack.append( stackEntryDup );

    if ( _statistics )
    {
        _statistics->setCurrentStackDepth( getStackDepth() );
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->addTimeSmtCore( TimeUtils::timePassed( start, end ) );
    }
}


void SmtStackManager::storeSmtState( SmtState& smtState )
{
    smtState._impliedValidSplitsAtRoot = _impliedValidSplitsAtRoot;

    for ( auto& stackEntry : _stack )
        smtState._stack.append( stackEntry->duplicateSmtStackEntry() );

    smtState._stateId = _stateId;
}

void SmtStackManager::recordImpliedValidSplit( PiecewiseLinearCaseSplit& validSplit )
{
    _stack.back()->_impliedValidSplits.append( validSplit );

    checkSkewFromDebuggingSolution();
}


void SmtStackManager::allSplitsSoFar( List<PiecewiseLinearCaseSplit>& result ) const
{
    result.clear();

    for ( const auto& it : _stack )
    {
        result.append( it->_activeSplit );
        // TODO figure out if it's needed
        for ( const auto& impliedSplit : it->_impliedValidSplits )
            result.append( impliedSplit );
    }
}


bool SmtStackManager::popSplit() {
    SMT_LOG( "Performing a pop" );

    if ( _stack.empty() )
        return false;

    struct timespec start = TimeUtils::sampleMicro();

    if ( _statistics )
    {
        _statistics->incNumPops();
        // A pop always sends us to a state that we haven't seen before - whether
        // from a sibling split, or from a lower level of the tree.
        _statistics->incNumVisitedTreeStates();
    }

    delete _stack.back()->_engineState;
    auto const poppedSplit = _stack.back()->_activeSplit;
    _stack.popBack();

    if ( _stack.empty() )
        return false;

    if ( checkSkewFromDebuggingSolution() )
    {
        // Pops should not occur from a compliant stack!
        printf( "Error! Popping from a compliant stack\n" );
        throw MarabouError( MarabouError::DEBUGGING_ERROR );
    }

    auto const& stackEntry = _stack.back();

    // Restore the state of the engine
    SMT_LOG( "\tRestoring engine state..." );
    _engine->restoreState( *( stackEntry->_engineState ) );
    SMT_LOG( "\tRestoring engine state - DONE" );

    if ( _statistics )
    {
        _statistics->setCurrentStackDepth( getStackDepth() );
        struct timespec end = TimeUtils::sampleMicro();
        _statistics->addTimeSmtCore( TimeUtils::timePassed( start, end ) );
    }

    checkSkewFromDebuggingSolution();

    List<PiecewiseLinearCaseSplit> allSplitsSoFar;
    this->allSplitsSoFar( allSplitsSoFar );
    _splitProvidersManager->onStackPopPerformed( PopInfo( allSplitsSoFar, poppedSplit ));

    return true;
}

unsigned SmtStackManager::getStackDepth() const {
    return _stack.size();
}

SmtStack const& SmtStackManager::getStack() const {
    return _stack;
}

void SmtStackManager::setStatistics( Statistics* statistics ) {
    _statistics = statistics;
}

void SmtStackManager::storeDebuggingSolution( const Map<unsigned, double>& debuggingSolution ) {
    _debuggingSolution = debuggingSolution;
}

bool SmtStackManager::checkSkewFromDebuggingSolution() {
    if ( _debuggingSolution.empty() )
        return false;

    String error;

    // Now go over the stack from oldest to newest and check that each level is compliant
    for ( const auto& stackEntry : _stack )
    {

        // Did we learn any valid splits that are non-compliant?
        for ( auto const& split : stackEntry->_impliedValidSplits )
        {
            if ( !splitAllowsStoredSolution( split, error ) )
            {
                printf( "Error with one of the splits implied at this stack level:\n\t%s\n",
                    error.ascii() );
                throw MarabouError( MarabouError::DEBUGGING_ERROR );
            }
        }
    }

    // No problems were detected, the stack is compliant with the stored solution
    return true;
}

bool SmtStackManager::splitAllowsStoredSolution( const PiecewiseLinearCaseSplit& split, String& error ) const {
    // False if the split prevents one of the values in the stored solution, true otherwise.
    error = "";
    if ( _debuggingSolution.empty() )
        return true;

    for ( const auto& bound : split.getBoundTightenings() )
    {
        unsigned variable = bound._variable;

        // If the possible solution doesn't care about this variable,
        // ignore it
        if ( !_debuggingSolution.exists( variable ) )
            continue;

        // Otherwise, check that the bound is consistent with the solution
        double solutionValue = _debuggingSolution[variable];
        double boundValue = bound._value;

        if ( ( bound._type == Tightening::LB ) && FloatUtils::gt( boundValue, solutionValue ) )
        {
            error = Stringf( "Variable %u: new LB is %.5lf, which contradicts possible solution %.5lf",
                variable,
                boundValue,
                solutionValue );
            return false;
        }
        else if ( ( bound._type == Tightening::UB ) && FloatUtils::lt( boundValue, solutionValue ) )
        {
            error = Stringf( "Variable %u: new UB is %.5lf, which contradicts possible solution %.5lf",
                variable,
                boundValue,
                solutionValue );
            return false;
        }
    }

    return true;
}
