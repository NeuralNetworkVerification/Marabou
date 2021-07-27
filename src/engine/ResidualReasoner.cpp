#include "ResidualReasoner.h"
#include "SmtCore.h"
#include "Debug.h"

#pragma GCC push_options
#pragma GCC optimize("O0")

Literal::Literal(Variable variable, Phase phase)
    : _variable(variable), _phase(phase)
{
}

void Literal::set(Phase phase)
{
    ASSERT(_phase == Phase::UNDECIDED);
    _phase = phase;
}

bool Clause::update(Variable variable, Phase phase)
{
    bool anyUpdated = false;
    for (auto &literal : _literals)
    {
        if (literal._variable == variable)
        {
            literal.set(phase);
            anyUpdated = true;
        }
    }
    return anyUpdated;
}

void Clause::add(Variable variable, Phase phase) {
     ASSERT(std::find_if(_literals.begin(), _literals.end(), [variable](Literal const& literal) {return literal._variable == variable;}) == _literals.end());

    _literals.append(Literal(variable, phase));
}

bool ClausesTable::update(Variable variable, Phase phase)
{
    bool anyUpdated = false;
    for (auto &caluse : _clauses)
    {
        anyUpdated |= caluse.update(variable, phase);
    }
    return anyUpdated;
}

void ClausesTable::add(Clause clause) {
     _clauses.append(clause);
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
    // find the variable splitted on, and it's phase
    auto const variablePhase = splitVariable(split);
    std::cout << "var split " << variablePhase.first() << " phase " << variablePhase.second() << std::endl;

    // update the table
    _gammaUnsatClausesTable.update(variablePhase.first(), variablePhase.second());
    _currentBranchClause.add(variablePhase.first(), variablePhase.second());
}

void ResidualReasoner::unsat() {
    printf("got unsat\n"); 
    _currentRunUnsatClausesTable.add(_currentBranchClause);
    _currentBranchClause = Clause();
}

#pragma GCC pop_options