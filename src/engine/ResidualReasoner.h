#ifndef __RESIDUALREASONER_H__
#define __RESIDUALREASONER_H__

#include "ISmtListener.h"

class ResidualReasoner : public ISmtListener {

public:
     Optional<ImpliedSplit> isAnyImpliedSpilt() const;
     void SplitOccurred(SplitInfo const& splitInfo);
};

#endif // __RESIDUALREASONER_H__
