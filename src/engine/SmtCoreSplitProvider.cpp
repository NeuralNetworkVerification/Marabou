#include "SmtCoreSplitProvider.h"
#include "Debug.h"
#include "TimeUtils.h"
#include "GlobalConfiguration.h"
#include "Options.h"

#pragma GCC push_options
#pragma GCC optimize ("O0")

SmtCoreSplitProvider::SmtCoreSplitProvider( IEngine* engine )
    : _engine( engine )
    , _constraintViolationThreshold( Options::get()->getInt( Options::CONSTRAINT_VIOLATION_THRESHOLD ) )
    , _constraintForSplitting( nullptr )
{ }

bool SmtCoreSplitProvider::searchForAlternatives()
{
    // printf("searching for alternatives\n");
    if ( _currentSuggestedSplit ) return true;

    if ( !_currentSuggestedSplitAlternatives.empty() ) {
        // printf("takeing from alternatives\n");
        _currentSuggestedSplit = _currentSuggestedSplitAlternatives.front();
        _currentSuggestedSplitAlternatives.popFront();
        return true;
    }
    // printf("no alternatives found\n");

    return false;
}

void SmtCoreSplitProvider::thinkBeforeSplit( List<SmtStackEntry *> const &stack ) {

    // We already have some splits in our backlog
    if ( searchForAlternatives() ) return;

    if ( !_needToSplit ) return;
    
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

    _currentSuggestedSplit = splits.front();

    splits.popFront();
    _currentSuggestedSplitAlternatives = splits;
    _currentSuggestedAlternativeSplit = nullopt;
}

Optional<PiecewiseLinearCaseSplit> SmtCoreSplitProvider::needToSplit() const {
    return _currentSuggestedSplit;
}

void SmtCoreSplitProvider::thinkBeforeSuggestingAlternative( List<SmtStackEntry *> const &stack ) {
    auto const& latestSplit = stack.back()->_activeSplit;
    auto const& currentTopSplit = _splitsStack.back();
    if ( currentTopSplit.first() != latestSplit ) {
        _currentSuggestedAlternativeSplit = nullopt;
    }

    if ( currentTopSplit.second().empty() )
    {
        // no alternatives
        _currentSuggestedAlternativeSplit = nullopt;
    }
    else
    {
        // has alternatives
        _currentSuggestedAlternativeSplit = currentTopSplit.second().front();
    }

}

Optional<PiecewiseLinearCaseSplit> SmtCoreSplitProvider::alternativeSplitOnCurrentStack() const {
    return _currentSuggestedAlternativeSplit;
}

void SmtCoreSplitProvider::onSplitPerformed( SplitInfo const& splitInfo ) {

    _needToSplit = false;
    if ( _currentSuggestedSplit && *_currentSuggestedSplit == splitInfo.theSplit ) {
        // it was my split that was performed, so we need to reset it for the next split request
        _splitsStack.emplace_back( splitInfo.theSplit, _currentSuggestedSplitAlternatives );
        _currentSuggestedSplit = nullopt;
        _currentSuggestedSplitAlternatives.clear();
    }
    else if ( _currentSuggestedAlternativeSplit && *_currentSuggestedAlternativeSplit == splitInfo.theSplit )
    {
        _splitsStack.back().first() = splitInfo.theSplit;
        _splitsStack.back().second().popFront();
        _currentSuggestedAlternativeSplit = nullopt;
    }
    else {
        // it some other provider split, we don't have alternatives for it
        _splitsStack.emplace_back( splitInfo.theSplit, List<PiecewiseLinearCaseSplit>{} );
    }
}

void SmtCoreSplitProvider::onStackPopPerformed( PopInfo const& popInfo ) {
    // debug: verify it the corrent split
    auto& currentTopSplit = _splitsStack.back();

    if ( currentTopSplit.second().empty() )
    {
        // no alternatives
        _splitsStack.popBack();
    }
    else
    {
        // has alternatives
        _currentSuggestedSplit = currentTopSplit.second().front();
        currentTopSplit.second().popFront();
        _currentSuggestedSplitAlternatives = currentTopSplit.second();
    }
}

void SmtCoreSplitProvider::onUnsatReceived(List<SmtStackEntry *> const &stack) {
}

void SmtCoreSplitProvider::reportViolatedConstraint( PiecewiseLinearConstraint* constraint ) {
    // std::cout << "on violated constraint" << std::endl;
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

