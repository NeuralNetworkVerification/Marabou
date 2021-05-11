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

#include "UNSATCertificate.h"

CertificateNode::CertificateNode( CertificateNode* parent, const PiecewiseLinearCaseSplit& constraint )
	:_constraint( constraint )
	,_children ( 0 )
	,_leafCertificate ( 0 )
	,_parent( parent )
{

}

void CertificateNode::setCertificate( SingleVarBoundsExplanator* certificate )
{
	_leafCertificate = certificate;
}


void CertificateNode::addChild( CertificateNode* child )
{
	_children.push_back( child );
}

SingleVarBoundsExplanator& CertificateNode::getCertificate()
{
	return *_leafCertificate;
}

const PiecewiseLinearCaseSplit& CertificateNode::getConstraint()
{
	return _constraint;
}

std::list<CertificateNode*> CertificateNode::getChildren()
{
	return _children;
}

bool CertificateNode::sanityCheck ( CertificateNode* root )
{
	if ( root->_children.empty() ^ ( root->_leafCertificate == NULL ) )
		return false;

	for ( auto child : root->_children )
		sanityCheck( child );

	return true;
}


bool CertificateNode::certifyTree( CertificateNode* root )
{
	//TODO complete
	return true;
}