/*********************                                                        */
/*! \file BoundsExplanator.cpp
 ** \verbatim
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/


#ifndef __UNSATCertificate_h__
#define __UNSATCertificate_h__

#include "PiecewiseLinearConstraint.h"
#include "BoundsExplanator.h"
#include <assert.h>
#include "ReluConstraint.h"

// Contains an explanation for a row addition during a run (i.e from ReLU phase-fixing)
// For now only relevant to ReLU constraints
struct PLCExplanation
{
	unsigned _explainedVar;
	unsigned _affectedVar;
	SingleVarBoundsExplanator* _explanation;
	PiecewiseLinearConstraint* _constraint;
};

struct Contradiction
{
	unsigned var;
	SingleVarBoundsExplanator explanation;
};


class CertificateNode
{
public:

	/*
	 * Constructor for the root
	 */
	CertificateNode(const std::vector<std::vector<double>> &_initialTableau, const std::vector<double> &groundUBs, const std::vector<double> &groundLBs );

	/*
	 * Constructor for a regular node
	 */
	CertificateNode( CertificateNode* parent, List<PiecewiseLinearCaseSplit> splits );

	~CertificateNode();

	/*
 	* Certifies the tree is indeed a proof of unsatisfiability;
 	*/
	bool certify();

	/*
	 * Sets the leaf certificate as input
	 */
	void setContradiction( Contradiction* certificate );

	/*
	 * Adds a child to the tree
	 */
	void addChild( CertificateNode* child );

	/*
	 * Gets the leaf certificate of the node
	 */
	const Contradiction& getContradiction() const;

	/*
	 * Gets the constraint defining the node
	 */
	const List<PiecewiseLinearCaseSplit>& getSplits() const;

	/*
	 * Gets the children of the node
	 */
	std::list<CertificateNode*> getChildren() const;

	/*
	 * Certifies a contradiction
	 */
	bool certifyContradiction() const;

	/*
	 * Computes a bound according to an explanation
	 */
	double explainBound( unsigned var, bool isUpper, const SingleVarBoundsExplanator& boundsExplanation ) const;

	/*
	 * Adds an PLC explanation to the list
	 */
	void addPLCExplanation( PLCExplanation& expl);

	/*
 	* Adds an a problem constraint to the list
 	*/
	void addProblemConstraints( ReluConstraint& con);

	/*
 	* Return true iff the splits are created from a valid PLC
 	*/
	bool certifyReLUSplits( List<PiecewiseLinearCaseSplit> splits) const;

private:

	List<PiecewiseLinearCaseSplit> _splits;
	std::list<CertificateNode*> _children;
	std::list<ReluConstraint*> _problemConstraints;
	CertificateNode* _parent;
	std::list<PLCExplanation> _PLCExplanations;
	Contradiction* _contradiction;

	std::vector<std::vector<double>> _initialTableau;
	std::vector<double> _groundUpperBounds;
	std::vector<double> _groundLowerBounds;

	/*
	 * Copies initial tableau and ground bounds
	 */
	void copyInitials (const std::vector<std::vector<double>> &_initialTableau, const std::vector<double> &groundUBs, const std::vector<double> &groundLBs );

	/*
	 * Inherits the initialTableau and ground bounds from parent, if exists
	 */
	void inheritInitials();

	/*
	 * Checks if the node is a valid leaf
	 */
	bool isValidLeaf() const;

	/*
	 * Checks if the node is a valid none-leaf
	 */
	bool isValidNoneLeaf() const;

	/*
	 * Clear initial tableau and ground bounds
	 */
	void clearInitials();

};

class UNSATCertificateUtils
{
public:
	static double computeBound( unsigned var, bool isUpper, const SingleVarBoundsExplanator& boundsExplanation,
							   const std::vector<std::vector<double>> &initialTableau, const std::vector<double> &groundUBs, const std::vector<double> &groundLBs )
	{
		ASSERT( groundLBs.size() == groundUBs.size() );
		ASSERT( initialTableau.size() == boundsExplanation.getLength() );
		ASSERT( groundLBs.size() == initialTableau[0].size() - 1 );
		ASSERT( groundLBs.size() == initialTableau[initialTableau.size() - 1].size() - 1 );

		double derived_bound = 0, scalar = 0, c, temp;
		unsigned n = groundUBs.size(), m = boundsExplanation.getLength();
		std::vector<double> expl ( m );
		boundsExplanation.getVarBoundExplanation( expl, isUpper );

		// If explanation is all zeros, return original bound
		bool allZeros = true;
		for( unsigned i = 0; i < expl.size(); ++i )
			if ( !FloatUtils::isZero( expl[i] ) )
				allZeros = false;
		if ( allZeros )
			return isUpper ? groundUBs[var] : groundLBs[var];

		// Create linear combination of original rows implied from explanation
		std::vector<double> explanationRowsCombination = std::vector<double>( n, 0 );

		for ( unsigned i = 0; i < m; ++i )
		{
			for ( unsigned j = 0; j < n; ++j )
				explanationRowsCombination[j] += initialTableau[i][j] * expl[i];

			scalar += initialTableau[i][n] * expl[i];
		}

		// Isolate var in the linear combination - compute its coefficient and divide by -c.
		// Then erase the coefficient of var
		c = explanationRowsCombination[var];

		ASSERT( !FloatUtils::isZero( c ) );

		for ( unsigned i = 0; i < n; ++i )
			explanationRowsCombination[i] /= -c;
		explanationRowsCombination[var] = 0;
		scalar /= -c;

		// Set the bound derived from the linear combination, using original bounds.
		for ( unsigned i = 0; i < n; ++i )
		{
			temp = explanationRowsCombination[i];
			if ( !FloatUtils::isZero( temp ) )
			{
				if ( isUpper )
					temp *= FloatUtils::isPositive( explanationRowsCombination[i] ) ? groundUBs[i] : groundLBs[i];
				else
					temp *= FloatUtils::isPositive( explanationRowsCombination[i] ) ? groundLBs[i] : groundUBs[i];

				if ( !FloatUtils::isZero( temp ) )
					derived_bound += temp;
			}
		}

		derived_bound += scalar;
		explanationRowsCombination.clear();
		expl.clear();
		return derived_bound;
	}
};


#endif //__UNSATCertificate_h__
