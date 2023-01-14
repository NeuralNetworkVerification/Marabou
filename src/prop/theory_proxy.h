/******************************************************************************
 * Top contributors (to current version):
 *   Andrew Reynolds, Haniel Barbosa, Dejan Jovanovic
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
 */

#ifndef CVC5__PROP__SAT_H
#define CVC5__PROP__SAT_H

#include <unordered_set>

#include "context/cdqueue.h"
#include "context/cdo.h"
#include "sat_solver_types.h"

  class Theory {
  public:
  /**
   * Subclasses of Theory may add additional efforts.  DO NOT CHECK
   * equality with one of these values (e.g. if STANDARD xxx) but
   * rather use range checks (or use the helper functions below).
   * Normally we call QUICK_CHECK or STANDARD; at the leaves we call
   * with FULL_EFFORT.
   */
  enum Effort
  {
    /**
     * Standard effort where theory need not do anything
     */
    EFFORT_STANDARD = 50,
    /**
     * Full effort requires the theory make sure its assertions are
     * satisfiable or not
     */
    EFFORT_FULL = 100,
    /**
     * Last call effort, called after theory combination has completed with
     * no lemmas and a model is available.
     */
    EFFORT_LAST_CALL = 200
  }; /* enum Effort */
  };

using namespace CVC4::context;

namespace prop {

  class TheoryEngine;
  class TNode;
  class Node;
/**
 * The proxy class that allows the SatSolver to communicate with the theories
 */
class TheoryProxy
{
  //using NodeSet = CDHashSet<Node>;

 public:
  TheoryProxy(TheoryEngine* theoryEngine);

  ~TheoryProxy();
  void theoryCheck(Theory::Effort effort);

  /** Get an explanation for literal `l` and save it on clause `explanation`. */
  void explainPropagation(SatLiteral l, SatClause& explanation);
  /** Notify that current propagation inserted at lower level than current.
   *
   * This method should be called by the SAT solver when the explanation of the
   * current propagation is added at lower level than the current user level.
   * It'll trigger a call to the ProofCnfStream to notify it that the proof of
   * this propagation should be saved in case it's needed after this user
   * context is popped.
   */
  void notifyCurrPropagationInsertedAtLevel(int explLevel);
  /** Notify that added clause was inserted at lower level than current.
   *
   * As above, but for clauses asserted into the SAT solver. This cannot be done
   * in terms of "current added clause" because the clause added at a lower
   * level could be for example a lemma derived at a prior moment whose
   * assertion the SAT solver delayed.
   */
  void notifyClauseInsertedAtLevel(const SatClause& clause, int clLevel);

  void theoryPropagate(SatClause& output);

  void enqueueTheoryLiteral(const SatLiteral& l);

  SatLiteral getNextTheoryDecisionRequest();

  bool theoryNeedCheck() const;

  /**
   * Notifies of a new variable at a decision level.
   */
  void variableNotify(SatVariable var);

  TNode getNode(SatLiteral lit);

  void notifyRestart();

  bool isDecisionEngineDone();

  bool isDecisionRelevant(SatVariable var);

  SatValue getDecisionPolarity(SatVariable var);

   private:
  /** The theory engine we are using. */
  TheoryEngine* d_theoryEngine;

  /** Queue of asserted facts */
  //CDQueue<TNode> d_queue;

  /** Whether we have been requested to stop the search */
  //CDO<bool> d_stopSearch;
}; /* class TheoryProxy */

}  // namespace prop

#endif
