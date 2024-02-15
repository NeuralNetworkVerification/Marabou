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
#include "PiecewiseLinearFunctionType.h"
#include "PlcLemma.h"
#include "ReluConstraint.h"
#include "SmtLibWriter.h"
#include "UnsatCertificateUtils.h"

enum DelegationStatus : unsigned {
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
    UnsatCertificateNode( UnsatCertificateNode *parent, PiecewiseLinearCaseSplit split );
    ~UnsatCertificateNode();

    /*
      Sets the leaf contradiction certificate as input
    */
    void setContradiction( Contradiction *certificate );

    /*
      Returns the leaf contradiction certificate of the node
    */
    const Contradiction *getContradiction() const;

    /*
      Returns the parent of a node
    */
    UnsatCertificateNode *getParent() const;

    /*
      Returns the head split of a node
    */
    const PiecewiseLinearCaseSplit &getSplit() const;

    /*
      Returns the head split of a node
    */
    const List<UnsatCertificateNode *> &getChildren() const;

    /*
      Returns the list of PLC lemmas of the node
    */
    const List<std::shared_ptr<PLCLemma>> &getPLCLemmas() const;

    /*
      Adds an PLC explanation to the list
    */
    void addPLCLemma( std::shared_ptr<PLCLemma> &explanation );

    /*
      Returns a pointer to a child by a head split, or NULL if not found
    */
    UnsatCertificateNode *getChildBySplit( const PiecewiseLinearCaseSplit &split ) const;

    /*
     Gets value of _hasSATSolution
    */
    bool getSATSolutionFlag() const;

    /*
      Sets value of _hasSATSolution to be true
    */
    void setSATSolutionFlag();

    /*
      Sets value of _wasVisited to be true
    */
    bool getVisited() const;

    /*
      Sets value of _wasVisited to be true
    */
    void setVisited();

    /*
      Gets delegation status of a node
    */
    DelegationStatus getDelegationStatus() const;

    /*
      Sets delegation status of a node
    */
    void setDelegationStatus( DelegationStatus saveToFile );

    /*
     Removes all PLC explanations
    */
    void deletePLCExplanations();

    /*
      Deletes all offsprings of the node and makes it a leaf
    */
    void makeLeaf();

    /*
     Checks if the node is a valid leaf
    */
    bool isValidLeaf() const;

    /*
      Checks if the node is a valid none-leaf
    */
    bool isValidNonLeaf() const;

private:
    List<UnsatCertificateNode *> _children;
    UnsatCertificateNode *_parent;
    List<std::shared_ptr<PLCLemma>> _PLCExplanations;
    Contradiction *_contradiction;
    PiecewiseLinearCaseSplit _headSplit;

    // Enables certifying correctness of UNSAT leaves in SAT queries
    bool _hasSATSolution;
    bool _wasVisited;

    DelegationStatus _delegationStatus;
};

#endif //__UnsatCertificateNode_h__
