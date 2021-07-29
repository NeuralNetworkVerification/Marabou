#ifndef __ISMTSPLITPROVIDER_H__
#define __ISMTSPLITPROVIDER_H__

#include "List.h"
#include "PiecewiseLinearCaseSplit.h"

struct SplitInfo
{
};

struct PopInfo
{
};

class ISmtSplitProvider
{
public:
    virtual List<PiecewiseLinearCaseSplit> needToSplit() const = 0;
    virtual void onSplitPerformed( SplitInfo const& ) = 0;
    virtual void onStackPopPerformed( PopInfo const& ) = 0;
    virtual void onUnsatReceived() = 0;
};

#endif // __ISMTSPLITPROVIDER_H__
