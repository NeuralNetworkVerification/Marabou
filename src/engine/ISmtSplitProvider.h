#ifndef __ISMTSPLITPROVIDER_H__
#define __ISMTSPLITPROVIDER_H__

#include "Optional.h"
#include "PiecewiseLinearCaseSplit.h"
#include "SmtStackEntry.h"

struct SplitInfo
{
    PiecewiseLinearCaseSplit theSplit;
};

struct PopInfo
{
    PiecewiseLinearCaseSplit poppedSplit;
};

class ISmtSplitProvider
{
public:
    virtual void thinkBeforeSplit(List<SmtStackEntry*> stack) = 0; 
    virtual Optional<PiecewiseLinearCaseSplit> needToSplit() const = 0;
    virtual void onSplitPerformed( SplitInfo const& ) = 0;
    virtual void onStackPopPerformed( PopInfo const& ) = 0;
    virtual void onUnsatReceived( List<PiecewiseLinearCaseSplit>& ) = 0;
};

#endif // __ISMTSPLITPROVIDER_H__
