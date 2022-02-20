/*********************                                                        */
/*! \file BoundExplainer.cpp
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


#ifndef __UNSATCertificate_h__
#define __UNSATCertificate_h__

#include "BoundExplainer.h"
#include "SmtLibWriter.h"
#include "PiecewiseLinearFunctionType.h"
#include "ReluConstraint.h"

/*
  Contains all necessary info of a ground bound update during a run (i.e from ReLU phase-fixing)
*/
struct PLCExplanation
{
	unsigned _causingVar;
	unsigned _affectedVar;
	double _bound;
	bool _isCausingBoundUpper;
	bool _isAffectedBoundUpper;
	double *_explanation;
	PiecewiseLinearFunctionType _constraintType;
	unsigned _decisionLevel;

	~PLCExplanation()
	{
		if ( _explanation )
		{
			delete [] _explanation;
			_explanation = NULL;
		}
	}
};

/*
  Contains all ino relevant for a simple Marabou contradiction - i.e. explanations of contradicting bounds of a variable
*/
struct Contradiction
{
	unsigned _var;
	double *_upperBoundExplanation;
	double *_lowerBoundExplanation;

	~Contradiction()
	{
		if ( _upperBoundExplanation )
		{
			delete [] _upperBoundExplanation;
			_upperBoundExplanation = NULL;
		}

		if ( _lowerBoundExplanation )
		{
			delete [] _lowerBoundExplanation;
			_lowerBoundExplanation = NULL;
		}
	}
};

/*
  A smaller representation of a problem constraint
*/
struct ProblemConstraint
{
	PiecewiseLinearFunctionType _type;
	List<unsigned> _constraintVars;
	PhaseStatus _status;

	bool operator==( const ProblemConstraint other ) const
	{
		return _type == other._type && _constraintVars == other._constraintVars;
	}
};

enum DelegationStatus : unsigned
{
	DONT_DELEGATE = 0,
	DELEGATE_DONT_SAVE = 1,
	DELEGATE_SAVE = 2
};

/*
  A certificate node in the tree representing the UNSAT certificate
*/
class CertificateNode
{
public:
	/*
	  Constructor for the root
	*/
	CertificateNode( Vector<Vector<double>> *_initialTableau, Vector<double> &groundUpperBounds, Vector<double> &groundLowerBounds );

	/*
	  Constructor for a regular node
	*/
	CertificateNode( CertificateNode *parent, PiecewiseLinearCaseSplit split );

	~CertificateNode();

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
	CertificateNode *getParent() const;

	/*
	  Returns the head split of a node
	*/
	const PiecewiseLinearCaseSplit &getSplit() const;

	/*
	 Returns the list of PLC explanations of the node
	*/
	const List<PLCExplanation*> &getPLCExplanations() const;

	/*
	  Adds an PLC explanation to the list
	*/
	void addPLCExplanation( PLCExplanation *explanation );

	/*
	 Adds an a problem constraint to the list
	*/
	void addProblemConstraint( PiecewiseLinearFunctionType type, List<unsigned int> constraintVars, PhaseStatus status );

	/*
	  Returns a pointer to a child by a head split, or NULL if not found
	*/
	CertificateNode *getChildBySplit( const PiecewiseLinearCaseSplit &split ) const;

	/*
	  Sets value of _hasSATSolution to be true
	*/
	void hasSATSolution();

	/*
	  Sets value of _wasVisited to be true
	*/
	void wasVisited();

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
	 Removes all PLC explanations from a certain point
	*/
	void resizePLCExplanationsList( unsigned newSize );

	/*
	  Deletes all offsprings of the node and makes it a leaf
	*/
	void makeLeaf();

	/*
	  Removes all PLCExplanations above a certain decision level WITHOUT deleting them
	  ASSUMPTION - explanations pointers are kept elsewhere before removal
	*/
	void removePLCExplanationsBelowDecisionLevel( unsigned decisionLevel );

private:
	List<CertificateNode*> _children;
	List<ProblemConstraint> _problemConstraints;
	CertificateNode* _parent;
	List<PLCExplanation*> _PLCExplanations;
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
	void passChangesToChildren( ProblemConstraint *childrenSplitConstraint );

	/*
	  Checks if the node is a valid leaf
	*/
	bool isValidLeaf() const;

	/*
	  Checks if the node is a valid none-leaf
	*/
	bool isValidNoneLeaf() const;

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
	ProblemConstraint *getCorrespondingReLUConstraint( const List<PiecewiseLinearCaseSplit> &splits );

	/*
	  Certifies a contradiction
	*/
	bool certifyContradiction();

	/*
	  Computes a bound according to an explanation
	*/
	double explainBound( unsigned var, bool isUpper, Vector<double> &explanation );
};

class UNSATCertificateUtils
{
public:
	/*
	  Use explanation to compute a bound (aka explained bound)
	  Given a variable, an explanation, initial tableau and ground bounds.
	*/
	static double computeBound( unsigned var, bool isUpper, const Vector<double> &explanation,
							  const Vector<Vector<double>> &initialTableau, const Vector<double> &groundUpperBounds, const Vector<double> &groundLowerBounds );

	/*
	  Given a var, a tableau and a column vector, create a linear combination used to explain a bound
	*/
	static void getExplanationRowCombination( unsigned var, Vector<double> &explanationRowCombination, const Vector<double> &explanation,
											 const Vector<Vector<double>> &initialTableau );
};
#endif //__UNSATCertificate_h__
