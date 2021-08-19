#include "ResidualReasoningSplitProvider.h"
#include "Debug.h"
#include "Engine.h"
#include "TimeUtils.h"
#include "GlobalConfiguration.h"
#include "Options.h"
#include "PiecewiseLinearCaseSplitUtils.h"

ResidualReasoningSplitProvider::ResidualReasoningSplitProvider( GammaUnsat gammaUnsat )
    : _gammaUnsat( gammaUnsat ),
    _pastSplits(),
    _required_splits()
{
}

void ResidualReasoningSplitProvider::thinkBeforeSplit( List<SmtStackEntry*> const& stack )
{
}

Optional<PiecewiseLinearCaseSplit> ResidualReasoningSplitProvider::needToSplit() const
{
    if ( !_required_splits.empty() )
    {
        return _required_splits.back();
    }
    return nullopt;
}

List<PiecewiseLinearCaseSplit> const& ResidualReasoningSplitProvider::getRequiredSplits()
{
    return _required_splits;
}
#pragma GCC push_options
#pragma GCC optimize ("O0")

void ResidualReasoningSplitProvider::onSplitPerformed( SplitInfo const& splitInfo )
{
    if ( splitInfo.theSplit == _required_splits.back() )
    {
        _required_splits.popBack();
    }
    _pastSplits.append( splitInfo.theSplit );
    bool continue_deriving = true;
    // continue deriveing as long as possible (like BCP at DPLL)
    while ( continue_deriving )
    {
        List<PLCaseSplitRawData> requiredSplits = deriveRequiredSplits();
        if ( requiredSplits.empty() )
        {
            continue_deriving = false;
        }
        for ( auto requiredSplit : requiredSplits )
        {
            ReluConstraint relu( requiredSplit._b, requiredSplit._f );
            if ( requiredSplit._activation == ActivationType::ACTIVE )
            {
                _required_splits.append( relu.getActiveSplit() );
            }
            else
            {
                _required_splits.append( relu.getInactiveSplit() );
            }
        }
    }
}
#pragma GCC pop_options

void ResidualReasoningSplitProvider::thinkBeforeSuggestingAlternative( List<SmtStackEntry*> const& stack ) {}

Optional<PiecewiseLinearCaseSplit> ResidualReasoningSplitProvider::alternativeSplitOnCurrentStack() const {return nullopt;}

void ResidualReasoningSplitProvider::onStackPopPerformed( PopInfo const& popInfo )
{
    _pastSplits.erase( popInfo.thePoppedSplit );
    // clear all required splits - maybe they were derived using the popped split
    _required_splits.clear();
}

void ResidualReasoningSplitProvider::onUnsatReceived( List<SmtStackEntry *> const &stack )
{
    List<PiecewiseLinearCaseSplit> allSplitsSoFar;
    for(SmtStackEntry const * const entry: stack)
    {
        allSplitsSoFar.append(entry->_activeSplit);
    }
    // convert allSplitSoFar to GammaUnsat::UnsatSequence
    GammaUnsat::UnsatSequence unsatSeq;
    for ( auto const& split : allSplitsSoFar )
    {
        unsatSeq.activations.append( split.getRawData() );
    }
    _gammaUnsat.addUnsatSequence( unsatSeq );
}

List<PLCaseSplitRawData> ResidualReasoningSplitProvider::deriveRequiredSplits() const
{
    // derives all required splits
    // a split is required if it is part of any unsat clause in gammaUnsat and all other parts
    // in the clause are mapped the same as they are mapped in _pastSplits (the current mapping)
    // for example, if (notation is: active=1, inactive=-1)
    // gammaUnsat = [{(c1f, c1b) : 1 , (c2f, c2b):1}, {(c1f, c1b):1, (c3f, c3b):-1}, {(c2f, c2b):-1,(c3f, c3b):-1}] and
    // _pastSplits = {(c1f, c1b):1}
    // then 2 splits: {(c2f, c2b):-1, (c3f, c3b):1} are derived from the 2 first clauses
    // (and no split is derived from the 3'rd clause)

    // implementation:
    // iterate _pastSplits, and for each one, assign all satisfied splits in any clause.
    // if at last there are clauses with one remaining split, a required split with the oppose
    // activation will be added to the result
    // TBD: use watch literals

    List<PLCaseSplitRawData> pastSplitsRaw;
    for ( auto const& split : _pastSplits )
    {
        pastSplitsRaw.append( split.getRawData() );
    }

    auto const getUnsatisfied =
        [&pastSplitsRaw]( GammaUnsat::UnsatSequence const& seq ) -> Optional<PLCaseSplitRawData>
    {
        Map<PLCaseSplitRawData, bool> satisfactionState;
        for ( auto pastSplit : pastSplitsRaw )
        {
            for ( auto gammaSeqCell : seq.activations )
            {
                satisfactionState[gammaSeqCell] |= gammaSeqCell == pastSplit;
            }
        }

        List<PLCaseSplitRawData> unsatisfied;
        for ( auto const& pair : satisfactionState )
        {
            if ( !pair.second )
            {
                unsatisfied.append( pair.first );
            }
        }
        if ( unsatisfied.size() == 1 )
        {
            return unsatisfied.front();
        }
    };

    List<PLCaseSplitRawData> derived;
    for ( auto const& gammaSeq : _gammaUnsat.getUnsatSequences() )
    {
        auto const maybeDerivedSplit = getUnsatisfied( gammaSeq );
        if ( maybeDerivedSplit )
            derived.append( *maybeDerivedSplit );
    }
    return derived;

    // Map<unsigned int, GammaUnsat::ActivationType> derived;
    // // maps from clause index to clause's map (for each clause)
    // // clause's map is map from split index in clause to its activation type
    // // if the split in this index in clause is satisfied
    // Map<unsigned, Map<unsigned int, bool>> satisfiedSplits;
    // for ( auto pastSplit : _pastSplits ) {
    //     unsigned clause_index = 0;
    //     for ( auto unsatClause : _gammaUnsat.getUnsatSequences() ) {
    //         unsigned int split_index = 0;
    //         for ( auto var_index2activation : unsatClause.activations ) {
    //             // if split is satisfied, assign it in satisfiedSplits
    //             unsigned var_index = var_index2activation.first;
    //             bool activation = var_index2activation.second;
    //             bool isSameSplit = var_index == getSplitVariable(pastSplit);
    //             bool isSameActivation = activation == isActiveSplit(pastSplit);
    //             if ( isSameSplit && isSameActivation ) {
    //                 satisfiedSplits[clause_index][split_index] = true;
    //             }
    //             ++split_index;
    //         }
    //         ++clause_index;
    //     }
    // }
    // // for all clauses with exactly one unsatisfied split, derive oppose activation to this split
    // for ( auto clause_index_and_clause : satisfiedSplits ) {
    //     unsigned int clause_index = clause_index_and_clause.first;
    //     Map<unsigned, bool> clause = clause_index_and_clause.second;
    //     // count the number of unsatisfied split. if 1, this one is a required split
    //     unsigned int unsatisfiedCounter = 0;
    //     unsigned int unsatisfiedSplitIndex = -1;
    //     for ( auto split_index_and_isSplitSatisfied : clause ) {
    //         unsigned split_index = split_index_and_isSplitSatisfied.first;
    //         bool isSplitSatisfied = split_index_and_isSplitSatisfied.second;
    //         if ( !isSplitSatisfied ) {
    //             unsatisfiedCounter += 1;
    //             unsatisfiedSplitIndex = split_index;
    //             if (unsatisfiedCounter > 1 ) {
    //                 break;
    //             }
    //         }
    //     }

    //     // derive activation to the split in unsatisfiedSplitIndex (the oppose to _gammaUnsat)
    //     GammaUnsat::UnsatSequence gammaClause;
    //     if ( unsatisfiedCounter == 1 ) {
    //         // get the relevant clause and split at _gammaUnsat
    //         List<GammaUnsat::UnsatSequence> unsatClauses = _gammaUnsat.getUnsatSequences();
    //         unsigned i = 0;
    //         // relevant clause
    //         for (auto it = unsatClauses.begin(); it != unsatClauses.end(); ++it ) {
    //             if (i == clause_index) {
    //                 gammaClause = *it;
    //                 unsigned int j = 0;
    //                 // relevant split
    //                 for ( auto split_var_and_activation : gammaClause.activations ) {
    //                     unsigned split_var = split_var_and_activation.first;
    //                     GammaUnsat::ActivationType activeType = split_var_and_activation.second;

    //                     if ( j == unsatisfiedSplitIndex ) {
    //                         GammaUnsat::ActivationType opposeActivation;
    //                         opposeActivation = activeType == GammaUnsat::ActivationType::ACTIVE ? GammaUnsat::ActivationType::INACTIVE : GammaUnsat::ActivationType::ACTIVE;
    //                         derived.insert(split_var, opposeActivation);
    //                         break;
    //                     }
    //                     ++j;
    //                 }
    //                 break;
    //             }
    //             ++i;
    //         }
    //     }
    //     ++clause_index;
    // }
    // return derived;
}
