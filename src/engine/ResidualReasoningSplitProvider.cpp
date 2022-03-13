#include "ResidualReasoningSplitProvider.h"
#include "Debug.h"
#include "Engine.h"
#include "TimeUtils.h"
#include "GlobalConfiguration.h"
#include "Options.h"
#include "Statistics.h"
#include "PiecewiseLinearCaseSplitUtils.h"
#include "Optional.h"

ResidualReasoningSplitProvider::ResidualReasoningSplitProvider( GammaUnsat gammaUnsat )
    : _gammaUnsat( gammaUnsat )
{ }

List<PiecewiseLinearCaseSplit> ResidualReasoningSplitProvider::getImpliedSplits( List<PiecewiseLinearCaseSplit> allSplitsSoFar ) const {
    bool continue_deriving = true;
    // continue deriveing as long as possible (like BCP at DPLL)
    unsigned int loop_counter = 0;
    List<PiecewiseLinearCaseSplit> impliedSplits;
    while ( continue_deriving )
    {
        loop_counter += 1;
        List<PLCaseSplitRawData> requiredSplits = deriveRequiredSplits( allSplitsSoFar );
        if ( requiredSplits.empty() )
        {
            continue_deriving = false;
        }
        for ( auto const& requiredSplit : requiredSplits )
        {
            ReluConstraint relu( requiredSplit._b, requiredSplit._f );
            if ( requiredSplit._activation == ActivationType::ACTIVE )
            {
                auto const split =  relu.getActiveSplit();
                allSplitsSoFar.append( split );
                impliedSplits.append(split);
            }
            else
            {
                auto const split =  relu.getInactiveSplit();
                allSplitsSoFar.append( split );
                impliedSplits.append( split );
            }
        }
    }
    return impliedSplits;
}

GammaUnsat ResidualReasoningSplitProvider::gammaUnsat() const {
    return _gammaUnsat;
}

void ResidualReasoningSplitProvider::onUnsatReceived( List<PiecewiseLinearCaseSplit> const& allSplitsSoFar )
{
    // convert allSplitSoFar to GammaUnsat::UnsatSequence
    GammaUnsat::UnsatSequence unsatSeq;
    for ( auto const& split : allSplitsSoFar )
    {
        auto const reluRawData = split.reluRawData();
        if ( reluRawData )
            unsatSeq.activations.append( *reluRawData );
    }
    _gammaUnsat.addUnsatSequence( unsatSeq );
}

List<PLCaseSplitRawData> ResidualReasoningSplitProvider::deriveRequiredSplits( List<PiecewiseLinearCaseSplit> const& allSplitsSoFar ) const
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

    // printf("in deriveRequiredSplits\n");
    List<PLCaseSplitRawData> pastSplitsRaw;
    for ( auto const& split : allSplitsSoFar )
    {
        if (split.reluRawData()) {
            pastSplitsRaw.append( *split.reluRawData() );
        }
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
        return nullopt;
    };

    List<PLCaseSplitRawData> derived;
    unsigned clause_index = 0;
    for ( auto const& gammaSeq : _gammaUnsat.getUnsatSequences() )
    {
        auto const maybeDerivedSplit = getUnsatisfied( gammaSeq );
        if ( maybeDerivedSplit ) {
            ActivationType opposeActivation = maybeDerivedSplit->_activation == ActivationType::ACTIVE ? ActivationType::INACTIVE : ActivationType::ACTIVE;
            PLCaseSplitRawData requiredSplit = PLCaseSplitRawData( maybeDerivedSplit->_b, maybeDerivedSplit->_f, opposeActivation );
            PLCaseSplitRawData opposeSplit = PLCaseSplitRawData( maybeDerivedSplit->_b, maybeDerivedSplit->_f, maybeDerivedSplit->_activation );
            // avoid duplicated derived splits
            bool doNotAppend = false;
            for ( auto const& split : allSplitsSoFar )
            {
                doNotAppend |= requiredSplit == *split.reluRawData();
                doNotAppend |= opposeSplit == *split.reluRawData();
            }
            for ( auto const& split : derived )
            {
                doNotAppend |= requiredSplit == split;
                doNotAppend |= opposeSplit == split;
            }
            if ( !doNotAppend ) {
                printf("split: b=%d, f=%d, a=%d add to derived\n", requiredSplit._b, requiredSplit._f, static_cast<int>(requiredSplit._activation));
                derived.append( requiredSplit );
            }
            // printf( "required from clause %d: variable=%u, activation=%s\n", clause_index,
            // requiredSplit._f,
            //     requiredSplit._activation == ActivationType::ACTIVE ? "active" : "inactive" );
        }
        clause_index += 1;
    }

    // for ( auto const& split : _pastSplits )
    // {
    //     derived.erase( *split.reluRawData() );
    // }
    // for ( auto const& split : _required_splits )
    // {
    //     derived.erase( *split.reluRawData() );
    // }

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
