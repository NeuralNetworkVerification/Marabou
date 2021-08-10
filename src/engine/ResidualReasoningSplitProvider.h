#ifndef __SMTCORESPLITPROVIDER_H__
#define __SMTCORESPLITPROVIDER_H__

#include "ISmtSplitProvider.h"
#include "IEngine.h"
#include "PiecewiseLinearConstraint.h"
#include "GammaUnsat.h"

class ResidualReasoningSplitProvider : public ISmtSplitProvider
{

public:
    explicit ResidualReasoningSplitProvider( IEngine * );

    void thinkBeforeSplit( List<SmtStackEntry*> ) override;
    Optional<PiecewiseLinearCaseSplit> needToSplit() const override;
    void onSplitPerformed( SplitInfo const& ) override;
    void onStackPopPerformed( PopInfo const& ) override;
    void onUnsatReceived( IEngine* ) override;
private:
    IEngine* _engine;
    GammaUnsat _gammaUnsat;
    // list of splits that were already activated/inactivated
    List<PiecewiseLinearCaseSplit> _pastSplits;
    // map from split to all splits that was derived from it
    Map<PiecewiseLinearCaseSplit, List<PiecewiseLinearCaseSplit>> _split2derivedSplits;
    // derive what are the required splits, after a given split was performed
    Map<PiecewiseLinearCaseSplit, GammaUnsat::ActivationType> deriveRequiredSplits( PiecewiseLinearCaseSplit );
    bool isActiveSplit( PiecewiseLinearCaseSplit );
};

#endif // __SMTCORESPLITPROVIDER_H__
