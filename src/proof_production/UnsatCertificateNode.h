/*********************                                                        */
/*! \file UnsatCertificateNode.h
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

#ifndef __UnsatCertificateNode_h__
#define __UnsatCertificateNode_h__

#include "BoundExplainer.h"
#include "Contradiction.h"
#include "UnsatCertificateProblemConstraint.h"
#include "SmtLibWriter.h"
#include "PiecewiseLinearFunctionType.h"
#include "PlcExplanation.h"
#include "ReluConstraint.h"
#include "UnsatCertificateUtils.h"

enum DelegationStatus : unsigned
{
    DONT_DELEGATE = 0,
    DELEGATE_DONT_SAVE = 1,
    DELEGATE_SAVE = 2
};

/*
  A certificate node in the tree representing the UNSAT certificate
*/
class UnsatCertificateNode
{
public:
    /*
      Constructor for the root
    */
    UnsatCertificateNode( Vector<Vector<double>> *_initialTableau, Vector<double> &groundUpperBounds, Vector<double> &groundLowerBounds );

    /*
      Constructor for a regular node
    */
    UnsatCertificateNode( UnsatCertificateNode *parent, PiecewiseLinearCaseSplit split );

    ~UnsatCertificateNode();

    /*
      Certifies the tree is indeed a correct proof of unsatisfiability;
    */
    bool certify();

    /*
      Sets the leaf contradiction certificate as input
    */
    void setContradiction( Contradiction *certificate );

    /*
      Returns the leaf contradiction certificate of the node
    */
    Contradiction *getContradiction() const;

    /*
     Returns the parent of a node
    */
    UnsatCertificateNode *getParent() const;

    /*
      Returns the head split of a node
    */
    const PiecewiseLinearCaseSplit &getSplit() const;

    /*
     Returns the list of PLC explanations of the node
    */
    const List<std::shared_ptr<PLCExplanation>> &getPLCExplanations() const;

    /*
     Sets  the list of PLC explanations of the node
    */
    void setPLCExplanations( const List<std::shared_ptr<PLCExplanation>> &explanations );

    /*
      Adds an PLC explanation to the list
    */
    void addPLCExplanation( std::shared_ptr<PLCExplanation> &explanation );

    /*
     Adds an a problem constraint to the list
    */
    void addProblemConstraint( PiecewiseLinearFunctionType type, List<unsigned> constraintVars, PhaseStatus status );

    /*
      Returns a pointer to a child by a head split, or NULL if not found
    */
    UnsatCertificateNode *getChildBySplit( const PiecewiseLinearCaseSplit &split ) const;

    /*
      Sets value of _hasSATSolution to be true
    */
    void setSATSolution();

    /*
      Sets value of _wasVisited to be true
    */
    void setVisited();

    /*
      Sets value of _shouldDelegate to be true
      Saves delegation to file iff saveToFile is true
    */
    void shouldDelegate( unsigned delegationNumber, DelegationStatus saveToFile );

    /*
     Removes all PLC explanations
    */
    void deletePLCExplanations();

    /*
      Deletes all offsprings of the node and makes it a leaf
    */
    void makeLeaf();

    /*
      Removes all PLCExplanations above a certain decision level WITHOUT deleting them
    */
    void removePLCExplanationsBelowDecisionLevel( unsigned decisionLevel );

private:
    List<UnsatCertificateNode*> _children;
    List<UnsatCertificateProblemConstraint> _problemConstraints;
    UnsatCertificateNode *_parent;
    List<std::shared_ptr<PLCExplanation>> _PLCExplanations;
    Contradiction *_contradiction;
    PiecewiseLinearCaseSplit _headSplit;

    // Enables certifying correctness of UNSAT leaves in SAT queries
    bool _hasSATSolution;
    bool _wasVisited;

    DelegationStatus _delegationStatus;
    unsigned _delegationNumber;

    Vector<Vector<double>> *_initialTableau;
    Vector<double> _groundUpperBounds;
    Vector<double> _groundLowerBounds;

    /*
      Copies initial tableau and ground bounds
    */
    void copyGroundBounds( Vector<double> &groundUpperBounds, Vector<double> &groundLowerBounds );

    /*
      Inherits the initialTableau pointer, the ground bounds and the problem constraint from parent, if exists.
      Fixes the phase of the constraint that corresponds to the head split
    */
    void passChangesToChildren( UnsatCertificateProblemConstraint *childrenSplitConstraint );

    /*
      Checks if the node is a valid leaf
    */
    bool isValidLeaf() const;

    /*
      Checks if the node is a valid none-leaf
    */
    bool isValidNonLeaf() const;

    /*
      Write a leaf marked to delegate to a smtlib file format
    */
    void writeLeafToFile();

    /*
      Return true iff a list of splits represents a splits over a single variable
    */
    bool certifySingleVarSplits( const List<PiecewiseLinearCaseSplit> &splits ) const;

    /*
      Return true iff the changes in the ground bounds are certified, with tolerance to errors with at most size epsilon
    */
    bool certifyAllPLCExplanations( double epsilon );

    /*
      Return a pointer to the problem constraint representing the split
    */
    UnsatCertificateProblemConstraint *getCorrespondingReLUConstraint( const List<PiecewiseLinearCaseSplit> &splits );

    /*
      Certifies a contradiction
    */
    bool certifyContradiction();

    /*
      Computes a bound according to an explanation
    */
    double explainBound( unsigned var, bool isUpper, Vector<double> &explanation );
};

#endif //__UnsatCertificateNode_h__