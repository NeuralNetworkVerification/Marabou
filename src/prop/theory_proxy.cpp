/******************************************************************************
 * Top contributors (to current version):
 *   Andrew Reynolds, Haniel Barbosa, Tim King
 *
 * This file is part of the cvc5 project.
 *
 * Copyright (c) 2009-2022 by the authors listed in the file AUTHORS
 * in the top-level source directory and their institutional affiliations.
 * All rights reserved.  See the file COPYING in the top-level source
 * directory for licensing information.
 * ****************************************************************************
 *
 * [[ Add one-line brief description here ]]
 *
 * [[ Add lengthier description here ]]
 * \todo document this file
 */
#include "theory_proxy.h"

#include "context/context.h"

#include "Engine.h"

namespace prop {

TheoryProxy::TheoryProxy( Engine* theoryEngine )
  : d_theoryEngine(theoryEngine)//,
    //d_queue(CVC4::context::Context()),
    //d_stopSearch(false, CVC4::context::UserContext())
{
}

TheoryProxy::~TheoryProxy() {
  /* nothing to do for now */
}

  void TheoryProxy::variableNotify(SatVariable /*var*/) {
  //  d_theoryEngine->preRegister(getNode(SatLiteral(var)));
}

  void TheoryProxy::theoryCheck(Theory::Effort /*effort*/) {
  // while (!d_queue.empty()) {
  //   TNode assertion = d_queue.front();
  //   d_queue.pop();
  //   // now, assert to theory engine
  //   d_theoryEngine->assertFact(assertion);
  // }
  // if (!d_stopSearch.get())
  // {
  //   d_theoryEngine->check(effort);
  // }
}

  void TheoryProxy::theoryPropagate(std::vector<SatLiteral>& /*output*/) {
  // // Get the propagated literals
  // std::vector<TNode> outputNodes;
  // d_theoryEngine->getPropagatedLiterals(outputNodes);
  // for (unsigned i = 0, i_end = outputNodes.size(); i < i_end; ++ i) {
  //   Trace("prop-explain") << "theoryPropagate() => " << outputNodes[i] << std::endl;
  //   output.push_back(d_cnfStream->getLiteral(outputNodes[i]));
  // }
}

  void TheoryProxy::explainPropagation(SatLiteral /*l*/, SatClause& /*explanation*/) {
  // TNode lNode = d_cnfStream->getNode(l);
  // Trace("prop-explain") << "explainPropagation(" << lNode << ")" << std::endl;

  // TrustNode tte = d_theoryEngine->getExplanation(lNode);
  // Node theoryExplanation = tte.getNode();
  // if (d_env.isSatProofProducing())
  // {
  //   Assert(options().smt.proofMode != options::ProofMode::FULL
  //          || tte.getGenerator());
  //   d_propEngine->getProofCnfStream()->convertPropagation(tte);
  // }
  // Trace("prop-explain") << "explainPropagation() => " << theoryExplanation
  //                       << std::endl;
  // explanation.push_back(l);
  // if (theoryExplanation.getKind() == kind::AND)
  // {
  //   for (const Node& n : theoryExplanation)
  //   {
  //     explanation.push_back(~d_cnfStream->getLiteral(n));
  //   }
  // }
  // els
  // {
  //   explanation.push_back(~d_cnfStream->getLiteral(theoryExplanation));
  // }
  // if (TraceIsOn("sat-proof"))
  // {
  //   std::stringstream ss;
  //   ss << "TheoryProxy::explainPropagation: clause for lit is ";
  //   for (unsigned i = 0, size = explanation.size(); i < size; ++i)
  //   {
  //     ss << explanation[i] << " [" << d_cnfStream->getNode(explanation[i])
  //        << "] ";
  //   }
  //   Trace("sat-proof") << ss.str() << "\n";
  // }
}

  void TheoryProxy::enqueueTheoryLiteral(const SatLiteral& /*l*/) {
  // Node literalNode = d_cnfStream->getNode(l);
  // Trace("prop") << "enqueueing theory literal " << l << " " << literalNode << std::endl;
  // Assert(!literalNode.isNull());
  // d_queue.push(literalNode);
}

SatLiteral TheoryProxy::getNextTheoryDecisionRequest() {
  // TNode n = d_theoryEngine->getNextDecisionRequest();
  //return n.isNull() ? undefSatLiteral : d_cnfStream->getLiteral(n);
  return undefSatLiteral;
}

bool TheoryProxy::theoryNeedCheck() const {
  // if (d_stopSearch.get())
  // {
  //   return false;
  // }
  // return d_theoryEngine->needCheck();
  return false;
}

void TheoryProxy::notifyRestart() {
  //d_theoryEngine->notifyRestart();
}

}  // namespace prop
