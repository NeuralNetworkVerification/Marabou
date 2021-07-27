#include "ResidualReasoner.h"
#include "SmtCore.h"
#include "Debug.h"
#include "Either.h"

#pragma GCC push_options
#pragma GCC optimize("O0")

Literal::Literal(Variable variable, Phase phase)
    : _variable(variable), _phase(phase)
{
}

Variable Literal::variable() const {
     return _variable;
}

Phase Literal::phase() const {
     return _phase;
}

bool Literal::operator<(Literal const& o) const {
     return _variable < o._variable;
}

bool ClausesTable::update(Variable variable, Phase phase)
{
//     bool anyUpdated = false;
//     for (auto &caluse : _clauses)
//     {
//         anyUpdated |= caluse.update(variable, phase);
//     }
//     return anyUpdated;
}

void ClausesTable::add(DisjunctionClause clause)
{
    _clauses.append(clause);
}

List<Literal> ClausesTable::forcedLiterals(DisjunctionClause clause) const
{
}

ResidualReasoner::ResidualReasoner() = default;

ResidualReasoner::ResidualReasoner(ClausesTable gammaUnsat)
    : _gammaUnsatClausesTable(gammaUnsat)
{
}

List<PiecewiseLinearCaseSplit> ResidualReasoner::impliedSplits(SmtCore &smtCore) const
{

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

Pair<Variable, Phase> splitVariable(PiecewiseLinearCaseSplit split)
{
    // check if active or not
    // is_active = there is at least one LB in _bounds
    // because there are two cases in Relu split:
    // active: b>=0 (LB), f-b<=0 (UB)
    // inactive b<=0 (UB), f-b<=0 (UB)

    // I'm assuming the first bound is the variable we want to look at, and the second one is the aux.
    // If it isn't always true, we need to find a way to know which variable we want to look at.
    auto const bound = split.getBoundTightenings().front();
    switch (bound._type)
    {
    case Tightening::BoundType::LB:
        return {bound._variable, Phase::ACTIVE};
    case Tightening::BoundType::UB:
        return {bound._variable, Phase::INACTIVE};
    default:
        return {bound._variable, Phase::UNDECIDED};
    }
}

void ResidualReasoner::splitOccurred(SplitInfo const &splitInfo)
{
    // printf("%s\n", splitInfo.msg.ascii());

    auto const split = splitInfo.split;
    DisjunctionClause clause;
    for (auto &&stackEntry : splitInfo.smtStack)
    {
        auto const split = stackEntry->_activeSplit;
        // find the variable splitted on, and it's phase
        auto const variablePhase = splitVariable(split);
        std::cout << "split " << variablePhase.first() << ":" << variablePhase.second() << ",";
        clause.add(Literal(variablePhase.first(), variablePhase.second()));
    }

    // search the table for implied splits

    // update the table
    // _gammaUnsatClausesTable.update(variablePhase.first(), variablePhase.second());
    // _currentBranchClause.add(variablePhase.first(), variablePhase.second());
}

void ResidualReasoner::unsat()
{
    printf("got unsat\n");
    _currentRunUnsatClausesTable.add(_currentBranchClause);
    _currentBranchClause = DisjunctionClause();
}

#pragma GCC pop_options