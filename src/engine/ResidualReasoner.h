#ifndef __RESIDUALREASONER_H__
#define __RESIDUALREASONER_H__

#include "ISmtListener.h"

class ResidualReasoner : public ISmtListener {

public:
     List<PiecewiseLinearCaseSplit> impliedSplits(SmtCore & smtCore) const;
     void splitOccurred(SplitInfo const& splitInfo);
};

#endif // __RESIDUALREASONER_H__
