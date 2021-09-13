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


CertificateNode::CertificateNode( const std::vector<std::vector<double>> &initialTableau, const std::vector<double> &groundUBs, const std::vector<double> &groundLBs )
	: _splits()
	, _children ( 0 )
	, _problemConstraints ( 0 )
	, _parent( NULL )
	, _PLCExplanations( 0 )
	, _contradiction ( NULL )
{
	copyInitials( initialTableau, groundUBs, groundLBs );
}

CertificateNode::CertificateNode( CertificateNode* parent, List<PiecewiseLinearCaseSplit> splits )
	: _splits( )
	, _children ( 0 )
	, _problemConstraints ( 0 )
	, _parent( parent )
	, _PLCExplanations( 0 )
	, _contradiction ( NULL )
	, _initialTableau( 0 )
	, _groundUpperBounds( 0 )
	, _groundLowerBounds( 0 )
{
	_splits.append( splits );
}

CertificateNode::~CertificateNode()
{
	for ( auto child : _children )
		if ( child )
		{
			delete child;
			child = NULL;
		}

	_splits.clear(); //TODO check if need to erase elements as well

	if ( !_PLCExplanations.empty() )
		_PLCExplanations.clear();

	if ( _contradiction )
	{
		delete _contradiction;
		_contradiction = NULL;
	}

	clearInitials();
}

void CertificateNode::setContradiction( Contradiction* contradiction )
{
	_contradiction = contradiction;
}


void CertificateNode::addChild( CertificateNode* child )
{
	_children.push_back( child );
}

const Contradiction& CertificateNode::getContradiction() const
{
	return *_contradiction;
}

const List<PiecewiseLinearCaseSplit>& CertificateNode::getSplits() const
{
	return _splits;
}

std::list<CertificateNode*> CertificateNode::getChildren() const
{
	return _children;
}

void CertificateNode::inheritInitials()
{
	if ( !_parent )
		return;

	copyInitials( _parent->_initialTableau, _parent->_groundUpperBounds, _parent->_groundLowerBounds );
}

bool CertificateNode::certify()
{
	bool answer;

	// Check if it is a leaf, and if so use contradiction to certify
	// return true iff it is certified
	if ( isValidLeaf() )
		answer = certifyContradiction();
	else
	{
		// Otherwise, assert there is a constraint and children
		// validate the constraint is ok
		// Certify all children
		// return true iff all children are certified
		answer = true;
		if ( !certifyReLUSplits( _splits ) )
			return false;

		ASSERT( isValidNoneLeaf() );
		for ( auto child : _children )
			if ( !child->certify() )
				answer = false;
	}

	clearInitials();
	return answer;
}

bool CertificateNode::certifyContradiction() const
{
	ASSERT( isValidLeaf() );
	unsigned var = _contradiction->var;
	SingleVarBoundsExplanator& expl = _contradiction->explanation;
	return ( FloatUtils::lt( explainBound( var, true, expl ), explainBound( var, false, expl ) ) );
}

double CertificateNode::explainBound( unsigned var, bool isUpper, const SingleVarBoundsExplanator& boundsExplanation ) const
{
	return UNSATCertificateUtils::computeBound(var, isUpper, boundsExplanation, _initialTableau, _groundUpperBounds, _groundLowerBounds);
}


void CertificateNode::copyInitials (const std::vector<std::vector<double>> &initialTableau, const std::vector<double> &groundUBs, const std::vector<double> &groundLBs )
{
	clearInitials();
	for ( auto& row : initialTableau )
	{
		auto rowCopy = std::vector<double> ( row.size() );
		std::copy( row.begin(), row.end(), rowCopy.begin() );
		_initialTableau.push_back( rowCopy );
	}

	_groundUpperBounds.resize(groundUBs.size() );
	_groundLowerBounds.resize(groundLBs.size() );

	std::copy(groundUBs.begin(), groundUBs.end(), _groundUpperBounds.begin() );
	std::copy(groundLBs.begin(), groundLBs.end(), _groundLowerBounds.begin() );

}

bool CertificateNode::isValidLeaf() const
{
	return _contradiction && _children.empty() && _splits.empty();
}

bool CertificateNode::isValidNoneLeaf() const
{
	return !_contradiction && !_children.empty() && !_splits.empty();
}

void CertificateNode::clearInitials()
{
	for ( auto& row : _initialTableau )
		row.clear();
	_initialTableau.clear();

	_groundUpperBounds.clear();
	_groundLowerBounds.clear();
}

void CertificateNode::addPLCExplanation( PLCExplanation& expl)
{
	_PLCExplanations.push_back( expl );
}


void CertificateNode::addProblemConstraints( ReluConstraint& con)
{
	_problemConstraints.push_back( &con );
}

bool CertificateNode::certifyReLUSplits( List<PiecewiseLinearCaseSplit> splits) const
{
	//TODO debug
	if ( splits.size() != 2 )
		return false;
	auto firstSplitTightenings = splits.front().getBoundTightenings(), secondSplitTightenings = splits.back().getBoundTightenings();
	if ( firstSplitTightenings.size() != 2 || secondSplitTightenings.size() != 2 )
		return false;

	// find the LB, it is b
	auto &activeSplit = firstSplitTightenings.back()._type == Tightening::LB ? firstSplitTightenings : secondSplitTightenings;
	auto &inactiveSplit = firstSplitTightenings.back()._type == Tightening::LB ? secondSplitTightenings : firstSplitTightenings;

	unsigned b = activeSplit.back()._variable;
	unsigned aux = activeSplit.front()._variable;
	unsigned f = inactiveSplit.front()._variable;

	if ( inactiveSplit.back()._variable != b ||  inactiveSplit.back()._type == Tightening::UB || inactiveSplit.front()._type == Tightening::UB || activeSplit.front()._type == Tightening::UB )
		return false;
	if ( inactiveSplit.back()._value != 0.0 || inactiveSplit.front()._value != 0.0 || activeSplit.back()._value != 0.0 || activeSplit.front()._value != 0.0 )
		return false;

	// certify that f = relu(b) + aux is in problem constraints
	bool foundConstraint = false;
	for ( auto &con : _problemConstraints )
	{
		assert( con->auxVariableInUse() );
		if ( con->getB() == b && con->getF() == f && con->getAux() == aux)
			foundConstraint = true;
	}

	return foundConstraint;
}