#include "ResidualReasoningSplitProvider.h"
#include "Debug.h"
#include "Engine.h"
#include "TimeUtils.h"
#include "GlobalConfiguration.h"
#include "Options.h"

ResidualReasoningSplitProvider::ResidualReasoningSplitProvider( IEngine* engine )
    : _engine( engine ),
    _gammaUnsat(),
    _pastSplits()
{ }

void ResidualReasoningSplitProvider::thinkBeforeSplit( List<SmtStackEntry*> stack ) {
}

Optional<PiecewiseLinearCaseSplit> ResidualReasoningSplitProvider::needToSplit() const {
    if ( !_split2derivedSplits.empty() ) {
        return *_split2derivedSplits.keys().begin();
    }
}

void ResidualReasoningSplitProvider::onSplitPerformed( SplitInfo const& splitInfo ) {
    Map<PiecewiseLinearCaseSplit, unsigned> requiredSplits = deriveRequiredSplits(splitInfo.theSplit);
    for ( auto requiredSplit : requiredSplits ) {
        List<PiecewiseLinearCaseSplit> value = _split2derivedSplits.get(splitInfo.theSplit);
        if ( value == NULL ) {
            List<PiecewiseLinearCaseSplit> derivedSplits;
            _split2derivedSplits.insert(splitInfo.theSplit, derivedSplits);
        }
        _split2derivedSplits[splitInfo.theSplit].append(requiredSplit.first);
    }
}

void ResidualReasoningSplitProvider::onStackPopPerformed( PopInfo const& popInfo ) {
    List<PiecewiseLinearCaseSplit> derivedSplits = _split2derivedSplits[popInfo.poppedSplit];
    for ( PiecewiseLinearCaseSplit derivedSplit : derivedSplits ) {
        _engine->popSplit(derivedSplit);
    }
}

void ResidualReasoningSplitProvider::onUnsatReceived( IEngine* engine ) {
    _gammaUnsat.addUnsatSequence( engine->getUnsatSeq() );
}

Map<PiecewiseLinearCaseSplit, GammaUnsat::ActivationType> ResidualReasoningSplitProvider::deriveRequiredSplits(PiecewiseLinearCaseSplit split){
  // derives all required splits
  // a required split is derived if it is part of any unsat clause and all other parts
  // in the clause are mapped the same as they are mapped in currentSplits (the current mapping)
  // for example, if (notation is: active=1, inactive=-1)
  // unsatClauses = [{c1:1, c2:1}, {c1:1, c3:-1}, {c2:-1,c3:-1}] and
  // currentSplits = {c1:1}
  // then 2 splits: {c2:-1, c3:1} are derived from the 2 first clauses
  // (and no split is derived from the 3'rd clause)
  
  // implementation:
  // iterate the current splits, and for each one, assign all satisfied splits in any clause
  // if at last there are clauses with one remaining split, a required split with the oppose
  // activation will be added to the result
  // TBD: use watch literals
  
      Map<PiecewiseLinearCaseSplit, GammaUnsat::ActivationType> derived;
      // maps from clause index to clause's map (for each clause) 
      // clause's map is map from split index in clause to its activation type
      // if the split in this index in clause is satisfied
      Map<unsigned, Map<unsigned int, bool>> satisfiedSplits;
      const PiecewiseLinearCaseSplit currentSplit = split;
      unsigned clause_index, split_index;
      for ( auto unsatClause : _gammaUnsat.getUnsatSequences() ) {
          clause_index = 0;
          for ( auto split : unsatClause.activations ) {
              split_index = 0;
              // if split is satisfied, assign it in satisfiedSplits
              bool isSameSplit = _engine->getSplitVariable(split) == _engine->getSplitVariable(currentSplit);
              bool isSameActivation = _engine->isActiveSplit(split) == _engine->isActiveSplit(currentSplit);
              if ( isSameSplit && isSameActivation ) {
                  satisfiedSplits[clause_index][split_index] = true;
              }
              ++split_index;
          }
          ++clause_index;
      }
      // for all clauses with exactly one unsatisfied split, derive oppose activation to this split
      for ( auto (clause_index_and_clause) : satisfiedSplits ) {
          // get the 2 parts of the pair...
          unsigned int clause_index = clause_index_and_clause.first;
          Map<unsigned, bool> clause = clause_index_and_clause.second;
          
          // count the number of unsatisfied split. if 1, this one is a required split
          unsigned int unsatisfiedCounter = 0;
          unsigned int unsatisfiedSplitIndex = -1;
          for ( auto split_index_and_isSplitSatisfied : clause ) {
              // get the 2 parts of the pair...
              unsigned split_index = split_index_and_isSplitSatisfied.first;
              bool isSplitSatisfied = split_index_and_isSplitSatisfied.second;
              
              if ( !isSplitSatisfied ) {
                  unsatisfiedCounter += 1;
                  unsatisfiedSplitIndex = split_index;
                  if (unsatisfiedCounter > 1 ) {
                      break;
                  }
              }
          }

          // derive activation to the split in unsatisfiedSplitIndex (the oppose to _gammaUnsat)
          GammaUnsat::UnsatSequence gammaClause;
          if (unsatisfiedCounter == 1) {
              // get the relevant clause at _gammaUnsat
              unsigned i = 0;
              List<GammaUnsat::UnsatSequence> unsatClauses = _gammaUnsat.getUnsatSequences();
              unsigned i = 0;
              for (auto it = unsatClauses.begin(); it != unsatClauses.end(); ++it ) {
                  ++i;
                  if (i >= clause_index) {
                      gammaClause = *it;
                      break;
                  }
              }

              GammaUnsat::ActivationType activeType = gammaClause.activations[unsatisfiedSplitIndex];
              // should assign active if unsat is imactive and vice versa
              GammaUnsat::ActivationType opposeActivation = GammaUnsat::ActivationType::ACTIVE ? activeType == GammaUnsat::ActivationType::INACTIVE : GammaUnsat::ActivationType::INACTIVE;
              derived.insert(split, opposeActivation);
          }
          ++clause_index;
      }
      return derived;
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
  }

