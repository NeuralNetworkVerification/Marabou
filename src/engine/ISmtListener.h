#ifndef __ISMTLISTENER_H__
#define __ISMTLISTENER_H__

#include "Optional.h"
#include "PiecewiseLinearCaseSplit.h"

struct ImpliedSplit {
    PiecewiseLinearCaseSplit split;
};
class SplitInfo {};

class ISmtListener {

public:
    virtual Optional<ImpliedSplit> isAnyImpliedSpilt() const = 0;
    virtual void SplitOccurred(SplitInfo const& splitInfo) = 0;

};

#endif // __ISMTLISTENER_H__
