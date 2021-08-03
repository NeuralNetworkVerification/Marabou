#include "SmtCoreSplitProvider.h"
#include "Debug.h"
#include "TimeUtils.h"
#include "GlobalConfiguration.h"
#include "Options.h"

SmtCoreSplitProvider::SmtCoreSplitProvider( IEngine* engine )
    : _engine( engine )
    , _constraintViolationThreshold( Options::get()->getInt( Options::CONSTRAINT_VIOLATION_THRESHOLD ) )
    , _constraintForSplitting( nullptr )
{ }

void SmtCoreSplitProvider::thinkBeforeSplit( List<SmtStackEntry*> stack ) {
    // We already have some splits in our backlog
    if ( !_alternativeSplits.empty() )
    {
        if ( !_currentSplit ) {
            _currentSplit = _alternativeSplits.peak();
            _alternativeSplits.pop();
        }
        return;
    }

    if ( _constraintForSplitting == nullptr ) return;

    //  Maybe the constraint has already become inactive - if so, ignore
    if ( !_constraintForSplitting->isActive() )
    {
        _needToSplit = false;
        _constraintToViolationCount[_constraintForSplitting] = 0;
        _constraintForSplitting = NULL;
        return;
    }

    // Before storing the state of the engine, we:
   //   1. Obtain the splits.
   //   2. Disable the constraint, so that it is marked as disabled in the EngineState.
    List<PiecewiseLinearCaseSplit> splits = _constraintForSplitting->getCaseSplits();
    ASSERT( !splits.empty() );
    ASSERT( splits.size() >= 2 ); // Not really necessary, can add code to handle this case.
    _constraintForSplitting->setActiveConstraint( false );

    for ( auto const& split : splits )
        _alternativeSplits.push( split );
    _currentSplit = _alternativeSplits.peak();
    _alternativeSplits.pop();
}

Optional<PiecewiseLinearCaseSplit> SmtCoreSplitProvider::needToSplit() const {
    return _currentSplit;
}

void SmtCoreSplitProvider::onSplitPerformed( SplitInfo const& splitInfo ) {
    auto const bound = *splitInfo.theSplit.getBoundTightenings().begin();
    std::cout << std::boolalpha << "on split " << bound._variable << " " << ( _currentSplit ? *_currentSplit == splitInfo.theSplit : false ) << std::endl;
    if(_currentSplit && *_currentSplit == splitInfo.theSplit){
        // it was my split that was performed, so we need to reset it for the next split request
        _currentSplit = nullopt;
    }
}

void SmtCoreSplitProvider::onStackPopPerformed( PopInfo const& ) {
    std::cout << "on pop" << std::endl;
}

void SmtCoreSplitProvider::onUnsatReceived() {
    std::cout << "on unsat" << std::endl;
}

void SmtCoreSplitProvider::reportViolatedConstraint( PiecewiseLinearConstraint* constraint ) {
    std::cout << "on violated constraint" << std::endl;
    if ( !_constraintToViolationCount.exists( constraint ) )
        _constraintToViolationCount[constraint] = 0;

    ++_constraintToViolationCount[constraint];

    if ( _constraintToViolationCount[constraint] >=
        _constraintViolationThreshold )
    {
        _needToSplit = true;
        if ( !pickSplitPLConstraint() )
            // If pickSplitConstraint failed to pick one, use the native
            // relu-violation based splitting heuristic.
            _constraintForSplitting = constraint;
    }
}

bool SmtCoreSplitProvider::pickSplitPLConstraint()
{
    if ( _needToSplit )
        _constraintForSplitting = _engine->pickSplitPLConstraint();
    return _constraintForSplitting != NULL;
}

unsigned SmtCoreSplitProvider::getViolationCounts( PiecewiseLinearConstraint* constraint ) const {
    if ( !_constraintToViolationCount.exists( constraint ) )
        return 0;

    return _constraintToViolationCount[constraint];
}

PiecewiseLinearConstraint* SmtCoreSplitProvider::chooseViolatedConstraintForFixing( List<PiecewiseLinearConstraint*>& _violatedPlConstraints ) const {
    ASSERT( !_violatedPlConstraints.empty() );

    if ( !GlobalConfiguration::USE_LEAST_FIX )
        return *_violatedPlConstraints.begin();

    PiecewiseLinearConstraint* candidate;

    // Apply the least fix heuristic
    auto it = _violatedPlConstraints.begin();

    candidate = *it;
    unsigned minFixes = getViolationCounts( candidate );

    PiecewiseLinearConstraint* contender;
    unsigned contenderFixes;
    while ( it != _violatedPlConstraints.end() )
    {
        contender = *it;
        contenderFixes = getViolationCounts( contender );
        if ( contenderFixes < minFixes )
        {
            minFixes = contenderFixes;
            candidate = contender;
        }

        ++it;
    }

    return candidate;
}

void SmtCoreSplitProvider::resetReportedViolations()
{
    _constraintToViolationCount.clear();
    _needToSplit = false;
}
