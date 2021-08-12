#include "PiecewiseLinearCaseSplitUtils.h"


unsigned int getSplitVariable(PiecewiseLinearCaseSplit currentSplit){
    // TODO::implement
    return 0;
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

