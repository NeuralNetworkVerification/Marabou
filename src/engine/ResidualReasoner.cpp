#include "ResidualReasoner.h"
#include "SmtCore.h"


List<PiecewiseLinearCaseSplit> ResidualReasoner::impliedSplits(SmtCore & smtCore) const {

    if (!smtCore._constraintForSplitting->isActive())
    {
        smtCore._needToSplit = false;
        smtCore._constraintToViolationCount[smtCore._constraintForSplitting] = 0;
        smtCore._constraintForSplitting = NULL;
        return {};
    }


    ASSERT(smtCore._constraintForSplitting->isActive());

    smtCore._constraintForSplitting->setActiveConstraint(false);

    if (smtCore._statistics)
    {
        smtCore._statistics->incNumSplits();
        smtCore._statistics->incNumVisitedTreeStates();
    }

    // Before storing the state of the engine, we:
    //   1. Obtain the splits.
    //   2. Disable the constraint, so that it is marked as disabled in the EngineState.
    List<PiecewiseLinearCaseSplit> splits = smtCore._constraintForSplitting->getCaseSplits();
    return splits;
}

void ResidualReasoner::splitOccurred(SplitInfo const &splitInfo) {
    printf("%s\n", splitInfo.msg.ascii());
}