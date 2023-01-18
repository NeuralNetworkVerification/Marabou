/******************************************************************************
 * Top contributors (to current version):
 *   Mathias Preiner, Liana Hadarean, Dejan Jovanovic
 *
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2022 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * SAT Solver.
 *
 * Implementation of the minisat interface for cvc5.
 */

#pragma once

#include "minisat/simp/SimpSolver.h"

#include "Statistics.h"

template <class Solver>
prop::SatLiteral toSatLiteral(typename Solver::TLit lit);

template <class Solver>
void toSatClause(const typename Solver::TClause& minisat_cl,
                 prop::SatClause& sat_cl);

namespace prop {

class MinisatSatSolver
{
 public:
  MinisatSatSolver(Statistics *statistics);
  ~MinisatSatSolver() ;

  static SatVariable     toSatVariable(Minisat::Var var);
  static Minisat::Lit    toMinisatLit(SatLiteral lit);
  static SatLiteral      toSatLiteral(Minisat::Lit lit);
  static SatValue        toSatLiteralValue(Minisat::lbool res);
  static Minisat::lbool  toMinisatlbool(SatValue val);
  //(Commented because not in use) static bool            tobool(SatValue val);

  static void  toMinisatClause(SatClause& clause, Minisat::vec<Minisat::Lit>& minisat_clause);
  static void  toSatClause    (const Minisat::Clause& clause, SatClause& sat_clause);
  void initialize(CVC4::context::Context* context,
                  TheoryProxy* theoryProxy,
                  CVC4::context::UserContext* userContext) ;

  void addClause(SatClause& clause, bool removable) ;
  void addXorClause(SatClause& /*clause*/, bool /*rhs*/, bool /*removable*/)
  {
    CVC4::Unreachable() << "Minisat does not support native XOR reasoning";
  }

  SatVariable newVar(bool isTheoryAtom,
                     bool preRegister,
                     bool canErase) ;
  SatVariable trueVar()  { return d_minisat->trueVar(); }
  SatVariable falseVar()  { return d_minisat->falseVar(); }

  SatValue solve() ;
  SatValue solve(long unsigned int&) ;
  SatValue solve(const std::vector<SatLiteral>& assumptions) ;
  void getUnsatAssumptions(std::vector<SatLiteral>& unsat_assumptions) ;

  bool ok() const ;

  void interrupt() ;

  SatValue value(SatLiteral l) ;

  SatValue modelValue(SatLiteral l) ;

  bool properExplanation(SatLiteral lit, SatLiteral expl) const ;

  /** Incremental interface */

  unsigned getAssertionLevel() const ;

  void push() ;

  void pop() ;

  void resetTrail() ;

  void requirePhase(SatLiteral lit) ;

  bool isDecision(SatVariable decn) const ;

  /** Return the list of current list of decisions that have been made by the
   * solver at the point when this function is called.
   */
  std::vector<SatLiteral> getDecisions() const ;

  /** Return the order heap.
   */
  std::vector<Node> getOrderHeap() const ;

  /** Return decision level at which `lit` was decided on. */
  int32_t getDecisionLevel(SatVariable v) const ;

  /**
   * Return user level at which `lit` was introduced.
   *
   * Note: The user level is tracked independently in the SAT solver and does
   * not query the user-context for the user level. The user level in the SAT
   * solver starts at level 0 and does not include the global push/pop in
   * the SMT engine.
   */
  int32_t getIntroLevel(SatVariable v) const ;

  /** Retrieve a pointer to the underlying solver. */
  Minisat::SimpSolver* getSolver() { return d_minisat; }

 private:

  /** The SatSolver used */
  Minisat::SimpSolver* d_minisat;

  /** Context we will be using to synchronize the sat solver */
  CVC4::context::Context* d_context;

  /**
   * Stores assumptions passed via last solve() call.
   *
   * It is used in getUnsatAssumptions() to determine which of the literals in
   * the final conflict clause are assumptions.
   */
  std::unordered_set<SatLiteral, SatLiteralHashFunction> d_assumptions;

  Statistics *d_statistics;

  void setupOptions();

}; /* class MinisatSatSolver */

}  // namespace prop
