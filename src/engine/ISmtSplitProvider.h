#ifndef __ISMTSPLITPROVIDER_H__
#define __ISMTSPLITPROVIDER_H__

#include "Optional.h"
#include "PiecewiseLinearCaseSplit.h"
#include "SmtStackEntry.h"

struct SplitInfo
{
    explicit SplitInfo( PiecewiseLinearCaseSplit const& theSplit );
    PiecewiseLinearCaseSplit theSplit;
};

struct PopInfo
{
    PopInfo( List<PiecewiseLinearCaseSplit> const& splitsSoFar, PiecewiseLinearCaseSplit const& thePoppedSplit );
    PiecewiseLinearCaseSplit thePoppedSplit;
    List<PiecewiseLinearCaseSplit> splitsSoFar;
};

class ISmtSplitProvider
{
public:
    virtual void thinkBeforeSplit( List<SmtStackEntry*> stack ) = 0;
    virtual Optional<PiecewiseLinearCaseSplit> needToSplit() const = 0;
    virtual void onSplitPerformed( SplitInfo const& ) = 0;
    virtual void onStackPopPerformed( PopInfo const& ) = 0;
    virtual void onUnsatReceived() = 0;
};

inline PopInfo::PopInfo( List<PiecewiseLinearCaseSplit> const& splitsSoFar, PiecewiseLinearCaseSplit const& thePoppedSplit )
    : thePoppedSplit( thePoppedSplit ),
    splitsSoFar( splitsSoFar )
{ }

inline SplitInfo::SplitInfo( PiecewiseLinearCaseSplit const& theSplit )
    : theSplit( theSplit )
{ }

#endif // __ISMTSPLITPROVIDER_H__
