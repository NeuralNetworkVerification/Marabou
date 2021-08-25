#ifndef __SMTCORERESIDUALREASONERSPLITPROVIDER_H__
#define __SMTCORERESIDUALREASONERSPLITPROVIDER_H__

#include "ISmtSplitProvider.h"
#include "IEngine.h"
#include "PiecewiseLinearConstraint.h"
#include "GammaUnsat.h"
#include "Pair.h"

class ResidualReasoningSplitProvider : public ISmtSplitProvider
{

public:
    explicit ResidualReasoningSplitProvider( GammaUnsat gammaUnsat );

    void thinkBeforeSplit( List<SmtStackEntry*> const& stack ) override;
    Optional<PiecewiseLinearCaseSplit> needToSplit() const override;
    void thinkBeforeSuggestingAlternative( List<SmtStackEntry*> const& stack );
    Optional<PiecewiseLinearCaseSplit> alternativeSplitOnCurrentStack() const;
    void onSplitPerformed( SplitInfo const& ) override;
    void onStackPopPerformed( PopInfo const& ) override;
    void onUnsatReceived( List<SmtStackEntry *> const &stack ) override;
    List<PiecewiseLinearCaseSplit> const& getRequiredSplits();

    GammaUnsat gammaUnsat() const;
private:
    IEngine* _engine;
    GammaUnsat _gammaUnsat;
    // list of splits that were already activated/inactivated
    List<PiecewiseLinearCaseSplit> _pastSplits;
    // list of splits that are required to be activated/inactivated
    List<PiecewiseLinearCaseSplit> _required_splits;
    // map from split to all splits that was derived from it
    Map<PiecewiseLinearCaseSplit, List<PiecewiseLinearCaseSplit>> _split2derivedSplits;
    // derive what are the required splits, after a given split was performed
    List<PLCaseSplitRawData> deriveRequiredSplits() const;
};

#endif // __SMTCORERESIDUALREASONERSPLITPROVIDER_H__
