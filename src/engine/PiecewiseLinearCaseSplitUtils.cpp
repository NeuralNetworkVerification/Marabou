#include "PiecewiseLinearCaseSplitUtils.h"

unsigned int getSplitVariable(const PiecewiseLinearCaseSplit & currentSplit) {
    // TODO::ask Guy about defining that aux is always the last and front is always the f variable
    unsigned int var = currentSplit.getBoundTightenings().front()._variable;
    return var;
}

bool isActiveSplit(PiecewiseLinearCaseSplit split)
{
    // check if active or not
    // is_active = there is at least one LB in _bounds
    // because there are tw cases in Relu split:
    // active: b>=0 (LB), f-b<=0 (UB)
    // inactive b<=0 (UB), f-b<=0 (UB)
    for (auto bound : split.getBoundTightenings())
    {
        if (bound._type == Tightening::BoundType::LB)
        {
            return true;
        }
    }
    return false;
};
