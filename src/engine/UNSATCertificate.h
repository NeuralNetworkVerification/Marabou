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
#include "Tightening.h"

// Contains an explanation for a row addition during a run (i.e from ReLU phase-fixing)
struct NewRowExplanation
{
	unsigned var;
	SingleVarBoundsExplanator& explanation;
	Tightening::BoundType type;
	PiecewiseLinearConstraint& constraint;
};


class CertificateNode
{
public:

	CertificateNode( CertificateNode* parent, const PiecewiseLinearCaseSplit& constraint );

	/*
	* Checks that only leaves have a certificate
	*/
	bool sanityCheck ( CertificateNode* root );


	/*
 	* Certifies the tree is indeed a proof of unsatisfiability;
 	*/
	bool certifyTree( CertificateNode* root );

	/*
	 * Sets the leaf certificate as input
	 */
	void setCertificate( SingleVarBoundsExplanator* certificate );

	/*
	 * Adds a child to the tree
	 */
	void addChild( CertificateNode* child );

	/*
	 * Gets the leaf certificate of the node
	 */
	SingleVarBoundsExplanator& getCertificate();

	/*
	 * Gets the constraint defining the node
	 */
	const PiecewiseLinearCaseSplit& getConstraint();

	/*
	 * Gets the children of the node
	 */
	std::list<CertificateNode*> getChildren();

private:

	const PiecewiseLinearCaseSplit& _constraint;
	std::list<CertificateNode*> _children;
	SingleVarBoundsExplanator* _leafCertificate;
	CertificateNode* _parent;
	std::list<NewRowExplanation> _newRowsExplanations;

};


#endif //__UNSATCertificate_h__
