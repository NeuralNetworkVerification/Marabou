#ifndef __SMTCORERESIDUALREASONERSPLITPROVIDER_H__
#define __SMTCORERESIDUALREASONERSPLITPROVIDER_H__

#include "IEngine.h"
#include "PiecewiseLinearConstraint.h"
#include "GammaUnsat.h"
#include "Pair.h"

class ResidualReasoningSplitProvider
{

public:
    explicit ResidualReasoningSplitProvider( GammaUnsat gammaUnsat );

    List<PiecewiseLinearCaseSplit> getImpliedSplits( List<PiecewiseLinearCaseSplit> allSplitsSoFar ) const;

    GammaUnsat gammaUnsat() const;
    void onUnsatReceived( List<PiecewiseLinearCaseSplit> const& allSplitsSoFar );

private:
    GammaUnsat _gammaUnsat;
    // derive what are the required splits, after a given split was performed
    List<PLCaseSplitRawData> deriveRequiredSplits( List<PiecewiseLinearCaseSplit> const& allSplitsSoFar ) const;
};

#endif // __SMTCORERESIDUALREASONERSPLITPROVIDER_H__
