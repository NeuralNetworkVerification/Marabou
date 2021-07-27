#ifndef __RESIDUALREASONER_H__
#define __RESIDUALREASONER_H__

#include "ISmtListener.h"
#include <fstream>
#include <sstream>

using Path = std::string;

using Variable = unsigned;
enum class Phase
{
     UNDECIDED = 0,
     ACTIVE = 1,
     INACTIVE = -1,
};

inline std::istream &operator>>(std::istream &is, Phase &phase)
{
     int value;
     is >> value;
     phase = static_cast<Phase>(value);
     return is;
}

inline std::ostream &operator<<(std::ostream &os, Phase const &phase)
{
     switch (phase)
     {
     case Phase::UNDECIDED:
          os << "Undecided";
          break;
     case Phase::ACTIVE:
          os << "Active";
          break;
     case Phase::INACTIVE:
          os << "Inactive";
          break;
     }
     return os;
}

struct Literal
{
     Literal(Variable variable, Phase phase);

     Variable _variable;
     Phase _phase = Phase::UNDECIDED;

     void set(Phase phase);
};

class Clause
{
public:
     List<Literal> _literals;

     bool update(Variable variable, Phase phase);
     void add(Variable variable, Phase phase = Phase::UNDECIDED);
};

class ClausesTable
{
public:
     List<Clause> _clauses;

     bool update(Variable variable, Phase phase);
     void add(Clause caluse);
};

inline void writeClauseTable(Path const &path, ClausesTable const &table)
{
     std::ofstream ofs{path};
     for (auto const &clause : table._clauses)
     {
          for (auto const &literal : clause._literals)
          {
               ofs << literal._variable << ":" << literal._phase << ",";
          }
          ofs << std::endl;
     }
}

inline ClausesTable readClauseTable(Path const &path)
{
     ClausesTable table;

     std::ifstream ifs{path};
     std::string line;
     while (std::getline(ifs, line))
     {
          // each line is a clause
          Clause clause;

          std::istringstream iss(line);
          Variable variable;
          Phase phase;
          char phaseDelimiter, literalsDelimiter;
          while (iss >> variable >> phaseDelimiter >> phase >> literalsDelimiter)
          {
               clause._literals.append(Literal{variable, phase});
          }

          table._clauses.append(clause);
     }

     return table;
}

class ResidualReasoner : public ISmtListener
{

public:
     List<PiecewiseLinearCaseSplit> impliedSplits(SmtCore &smtCore) const;
     void splitOccurred(SplitInfo const &splitInfo);
     void unsat();

     ClausesTable _gammaUnsatClausesTable;
     ClausesTable _currentRunUnsatClausesTable;
     Clause _currentBranchClause;
};

#endif // __RESIDUALREASONER_H__
