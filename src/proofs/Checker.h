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

#include "Set.h"
#include "Stack.h"
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
      If called from a certificate of a satisfiable query, checks that all proofs for bound propagations and unsatisfiable leaves are correct
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

    // Keeps track of bounds changes, so only stored bounds will be reverted when traveling the tree
    Stack<Set<unsigned>> _upperBoundChanges;
    Stack<Set<unsigned>> _lowerBoundChanges;


    /*
      Checks a node in the certificate tree
    */
    bool checkNode( const UnsatCertificateNode *node );

    /*
      Return true iff the changes in the ground bounds are certified, with tolerance to errors with at most size epsilon
    */
    bool checkAllPLCExplanations( const UnsatCertificateNode *node, double epsilon );

    /*
      Checks a contradiction
    */
    bool checkContradiction( const UnsatCertificateNode *node ) const;

    /*
      Computes a bound according to an explanation
    */
    double explainBound( unsigned var, bool isUpper, const double *explanation ) const;

    /*
      Write the data marked to delegate to a smtlib file format
    */
    void writeToFile();

    /*
      Return a pointer to the problem constraint representing the split
    */
    PiecewiseLinearConstraint *getCorrespondingReLUConstraint( const List<PiecewiseLinearCaseSplit> &splits );

    /*
      Return true iff a list of splits represents a splits over a single variable
    */
    bool checkSingleVarSplits( const List<PiecewiseLinearCaseSplit> &splits );
};

#endif //__Checker_h__