/*********************                                                        */
/*! \file Checker.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Omri Isac, Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2022 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]
 **/

#ifndef __Checker_h__
#define __Checker_h__

#include "AbsoluteValueConstraint.h"
#include "DisjunctionConstraint.h"
#include "LeakyReluConstraint.h"
#include "MaxConstraint.h"
#include "Set.h"
#include "Stack.h"
#include "Tightening.h"
#include "UnsatCertificateNode.h"

/*
  A class responsible to certify the UnsatCertificate
*/
class Checker
{
public:
    Checker( const UnsatCertificateNode *root,
             unsigned proofSize,
             const SparseMatrix *initialTableau,
             const Vector<double> &groundUpperBounds,
             const Vector<double> &groundLowerBounds,
             const List<PiecewiseLinearConstraint *> &_problemConstraints );

    /*
      Checks if the tree is indeed a correct proof of unsatisfiability.
      If called from a certificate of a satisfiable query, checks that all proofs for bound
      propagations and unsatisfiable leaves are correct
    */
    bool check();

private:
    // The root of the tree to check
    const UnsatCertificateNode *_root;
    unsigned _proofSize;

    // Additional context data
    const SparseMatrix *_initialTableau;

    Vector<double> _groundUpperBounds;
    Vector<double> _groundLowerBounds;

    List<PiecewiseLinearConstraint *> _problemConstraints;

    unsigned _delegationCounter;

    // Keeps track of bounds changes, so only stored bounds will be reverted when traversing the
    // tree
    Stack<Set<unsigned>> _upperBoundChanges;
    Stack<Set<unsigned>> _lowerBoundChanges;

    /*
      Checks a node in the certificate tree
    */
    bool checkNode( const UnsatCertificateNode *node );

    /*
      Return true iff all changes in the ground bounds are certified, with tolerance to errors with
      at most size epsilon
    */
    bool checkAllPLCExplanations( const UnsatCertificateNode *node, double epsilon );

    /*
      Return a change in the ground bounds caused by a ReLU constraint.
    */
    double
    checkReluLemma( const PLCLemma &expl, PiecewiseLinearConstraint &constraint, double epsilon );

    /*
      Return a change in the ground bounds caused by a Sign constraint.
    */
    double
    checkSignLemma( const PLCLemma &expl, PiecewiseLinearConstraint &constraint, double epsilon );

    /*
      Return a change in the ground bounds caused by a Absolute Value constraint.
    */
    double
    checkAbsLemma( const PLCLemma &expl, PiecewiseLinearConstraint &constraint, double epsilon );

    /*
      Return a change in the ground bounds caused by a Max constraint.
    */
    double checkMaxLemma( const PLCLemma &expl, PiecewiseLinearConstraint &constraint );
    /*
      Return a change in the ground bounds caused by a ReLU constraint.
    */
    double checkLeakyReluLemma( const PLCLemma &expl,
                                PiecewiseLinearConstraint &constraint,
                                double epsilon );
    /*
      Checks a contradiction
    */
    bool checkContradiction( const UnsatCertificateNode *node ) const;

    /*
      Computes a bound according to an explanation
    */
    double explainBound( unsigned var, bool isUpper, const SparseUnsortedList &explanation ) const;

    /*
      Write the data marked to delegate to a smtlib file format
    */
    void writeToFile();

    /*
      Return a pointer to the problem constraint representing the split
    */
    PiecewiseLinearConstraint *
    getCorrespondingConstraint( const List<PiecewiseLinearCaseSplit> &splits );

    /*
      Return a pointer to a ReLU problem constraint representing the split
    */
    PiecewiseLinearConstraint *
    getCorrespondingReluConstraint( const List<PiecewiseLinearCaseSplit> &splits );

    /*
      Return a pointer to a sign problem constraint representing the split
    */
    PiecewiseLinearConstraint *
    getCorrespondingSignConstraint( const List<PiecewiseLinearCaseSplit> &splits );

    /*
      Return a pointer to a absolute value problem constraint representing the split
    */
    PiecewiseLinearConstraint *
    getCorrespondingAbsConstraint( const List<PiecewiseLinearCaseSplit> &splits );

    /*
      Return a pointer to a max problem constraint representing the split
    */
    PiecewiseLinearConstraint *
    getCorrespondingMaxConstraint( const List<PiecewiseLinearCaseSplit> &splits );

    /*
      Return a pointer to a disjunction problem constraint representing the split
    */
    PiecewiseLinearConstraint *
    getCorrespondingDisjunctionConstraint( const List<PiecewiseLinearCaseSplit> &splits );

    /*
     Return a pointer to a LeakyReLU problem constraint representing the split
   */
    PiecewiseLinearConstraint *
    getCorrespondingLeakyReluConstraint( const List<PiecewiseLinearCaseSplit> &splits );

    /*
      Return true iff a list of splits represents a splits over a single variable
    */
    bool checkSingleVarSplits( const List<PiecewiseLinearCaseSplit> &splits );

    /*
      Fix phase of the child split
    */
    void fixChildSplitPhase( UnsatCertificateNode *child,
                             PiecewiseLinearConstraint *childrenSplitConstraint );
};

#endif //__Checker_h__