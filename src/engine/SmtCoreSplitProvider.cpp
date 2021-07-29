#include "SmtCoreSplitProvider.h"
#include "Debug.h"
#include "GlobalConfiguration.h"

SmtCoreSplitProvider::SmtCoreSplitProvider( IEngine* engine )
    : _engine( engine )
{ }

List<PiecewiseLinearCaseSplit> SmtCoreSplitProvider::needToSplit() const {

    ASSERT( _needToSplit );

    // Maybe the constraint has already become inactive - if so, ignore
    if ( !_constraintForSplitting->isActive() )
    {
        _needToSplit = false;
        _constraintToViolationCount[_constraintForSplitting] = 0;
        _constraintForSplitting = NULL;
        return {};
    }

    struct timespec start = TimeUtils::sampleMicro();

    ASSERT( _constraintForSplitting->isActive() );

    return {};
}

void SmtCoreSplitProvider::onSplitPerformed( SplitInfo const& ) {

}

void SmtCoreSplitProvider::onStackPopPerformed( PopInfo const& ) {

}

void SmtCoreSplitProvider::onUnsatReceived() {

}

void SmtCoreSplitProvider::reportViolatedConstraint( PiecewiseLinearConstraint* constraint ) {
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

