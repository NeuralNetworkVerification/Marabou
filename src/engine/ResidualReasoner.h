#ifndef __RESIDUALREASONER_H__
#define __RESIDUALREASONER_H__

#include "ISmtListener.h"
#include "SortedContainer.h"
#include <fstream>
#include <sstream>
#include <boost/algorithm/string.hpp>

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

class Literal
{
public:
     Literal(Variable variable, Phase phase = Phase::UNDECIDED);

     Variable variable() const;
     Phase phase() const;

     bool operator<(Literal const& o) const;

private:
     Variable _variable;
     Phase _phase;
};


class DisjunctionClause
{
public:
     DisjunctionClause add(Literal literal) const;

     SortedContainer<Literal> const& literals() const;

private:
     SortedContainer<Literal> _literals;
};

class ClausesTable
{
public:
     List<DisjunctionClause> _clauses;

     bool update(Variable variable, Phase phase);
     void add(DisjunctionClause caluse);
     List<Literal> forcedLiterals(DisjunctionClause clause) const;
};

inline void writeClauseTable(Path const &path, ClausesTable const &table)
{
     std::ofstream ofs{path};
     for (auto const &clause : table._clauses)
     {
          for (auto const &literal : clause.literals())
          {
               ofs << literal.variable() << ":" << static_cast<int>(literal.phase()) << ",";
          }
          ofs << std::endl;
     }
}

inline ClausesTable readClauseTable(Path const &path)
{
     ClausesTable table;

     // std::ifstream ifs{path};
     // std::string line;
     // while (std::getline(ifs, line))
     // {
     //      // each line is a clause
     //      DisjunctionClause clause;

     //      std::vector<std::string> variableToken; // #2: Search for tokens
     //      boost::split(variableToken, line, [](char c)
     //                   { return c == ','; });
     //      for (auto &&varToken : variableToken)
     //      {
     //           std::istringstream iss(varToken);
     //           Variable variable;
     //           char _;
     //           Phase phase;
     //           iss >> variable >> _ >> phase;
     //           std::cout << variable << " " << phase << std::endl;
     //           clause._literals.add(Literal{variable, phase});
     //      }

     //      table._clauses.append(clause);
     // }

     // return table;
}

class ResidualReasoner : public ISmtListener
{

public:
     ResidualReasoner();
     ResidualReasoner(ClausesTable gammaUnsat);

     List<PiecewiseLinearCaseSplit> impliedSplits(SmtCore &smtCore) const;
     void splitOccurred(SplitInfo const &splitInfo);
     void unsat();

     ClausesTable _gammaUnsatClausesTable;
     ClausesTable _currentRunUnsatClausesTable;
     DisjunctionClause _currentBranchClause;
};

#endif // __RESIDUALREASONER_H__
