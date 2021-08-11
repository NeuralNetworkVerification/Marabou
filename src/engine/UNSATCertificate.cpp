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
	, _parent( NULL )
	, _newRowsExplanations( 0 )
	, _contradiction ( NULL )
{
	copyInitials( initialTableau, groundUBs, groundLBs );
}

CertificateNode::CertificateNode( CertificateNode* parent, List<PiecewiseLinearCaseSplit> splits )
	: _splits( )
	, _children ( 0 )
	, _parent( parent )
	, _newRowsExplanations( 0 )
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

	if ( !_newRowsExplanations.empty() )
		_newRowsExplanations.clear();

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

void CertificateNode::updateInitialsWithNewRowsExplanations()
{
	inheritInitials();

	unsigned diffSize =  _newRowsExplanations.size();
	if ( !diffSize ) // Nothing to update
		return;

	for ( auto &initialRow : _initialTableau ) // Keeps scalar at last place
		initialRow.insert( initialRow.end() - 1, diffSize, 0 );

	// TODO certify explanations of new rows
	// Update explanation of all new rows?

	unsigned count = 0;
	for ( auto& expl : _newRowsExplanations )
	{
		std::vector<double>& row = expl.initialTableauRow;
		row.insert( row.end() - 1, diffSize - count, 0 );
		ASSERT( row.size() == _initialTableau[0].size() );
		_initialTableau.push_back( row );
		_groundUpperBounds.push_back( expl.upperBound );
		_groundLowerBounds.push_back( expl.lowerBound );
		++count;
	}

	ASSERT( _groundLowerBounds.size() == _groundUpperBounds.size() );
	ASSERT( _groundLowerBounds.size() == _initialTableau[0].size() - 1 );
	ASSERT( _groundLowerBounds.size() == _initialTableau.back().size() -1 );
}


bool CertificateNode::certify()
{
	bool answer;
	// Create the correct initial tableau and ground bounds by inheriting from parent and adding
	updateInitialsWithNewRowsExplanations();

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
		if ( !UNSATCertificateUtils::certifySplits( _splits ) )
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