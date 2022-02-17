/*********************                                                        */
/*! \file BoundsExplainer.cpp
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

#include "BoundsExplainer.h"

BoundsExplainer::BoundsExplainer( const unsigned varsNum, const unsigned rowsNum )
	:_varsNum( varsNum )
	,_rowsNum( rowsNum )
	,_upperBoundsExplanations( _varsNum, std::vector<double>( 0 ) )
	,_lowerBoundsExplanations( _varsNum,  std::vector<double>( 0 ) )
{
}

BoundsExplainer::~BoundsExplainer()
{
	for ( unsigned i = 0; i < _varsNum; ++i )
	{
		_upperBoundsExplanations[i].clear();
		_lowerBoundsExplanations[i].clear();
	}

	_upperBoundsExplanations.clear();
	_lowerBoundsExplanations.clear();
}

unsigned BoundsExplainer::getRowsNum() const
{
	return _rowsNum;
}

unsigned BoundsExplainer::getVarsNum() const
{
	return _varsNum;
}

BoundsExplainer& BoundsExplainer::operator=( const BoundsExplainer& other )
{
	if ( this == &other )
		return *this;

	if ( _varsNum != other._varsNum )
	{
		_upperBoundsExplanations.resize( other._varsNum );
		_upperBoundsExplanations.shrink_to_fit();

		_lowerBoundsExplanations.resize( other._varsNum );
		_lowerBoundsExplanations.shrink_to_fit();
	}

	_rowsNum = other._rowsNum;
	_varsNum = other._varsNum;

	for ( unsigned i = 0; i < _varsNum; ++i )
	{
		if ( _upperBoundsExplanations[i].size() != other._upperBoundsExplanations[i].size() )
		{
			_upperBoundsExplanations[i].clear();
			_upperBoundsExplanations[i].resize( other._upperBoundsExplanations[i].size() );
			_upperBoundsExplanations[i].shrink_to_fit();
		}

		if ( _lowerBoundsExplanations[i].size() != other._lowerBoundsExplanations[i].size() )
		{
			_lowerBoundsExplanations[i].clear();
			_lowerBoundsExplanations[i].resize( other._lowerBoundsExplanations[i].size() );
			_lowerBoundsExplanations[i].shrink_to_fit();
		}

		std::copy( other._upperBoundsExplanations[i].begin(), other._upperBoundsExplanations[i].end(), _upperBoundsExplanations[i].begin() );
		std::copy( other._lowerBoundsExplanations[i].begin(), other._lowerBoundsExplanations[i].end(), _lowerBoundsExplanations[i].begin() );
	}

	return *this;
}

const std::vector<double>& BoundsExplainer::getExplanation( const unsigned var, const bool isUpper )
{
	ASSERT ( var < _varsNum );
	return isUpper ? _upperBoundsExplanations[var] : _lowerBoundsExplanations[var];
}

void BoundsExplainer::updateBoundExplanation( const TableauRow& row, const bool isUpper )
{
	if ( !row._size )
		return;
	updateBoundExplanation( row, isUpper, row._lhs );
}

void BoundsExplainer::updateBoundExplanation( const TableauRow& row, const bool isUpper, const unsigned var )
{
	if ( !row._size )
		return;

	ASSERT ( var < _varsNum );
	double curCoefficient, ci = 0, realCoefficient;
	bool tempUpper;
	unsigned curVar;
	if ( var == row._lhs )
		ci = -1;
	else
	{
		for ( unsigned i = 0; i < row._size; ++i )
			if ( row._row[i]._var == var )
			{
				ci = row._row[i]._coefficient;
				break;
			}
	}

	ASSERT( !FloatUtils::isZero( ci ) );
	std::vector<double> rowCoefficients = std::vector<double>( _rowsNum, 0 );
	std::vector<double> sum = std::vector<double>( _rowsNum, 0 );
	std::vector<double> tempBound;

	for ( unsigned i = 0; i < row._size; ++i )
	{
		curVar = row._row[i]._var;
		curCoefficient = row._row[i]._coefficient;
		if ( FloatUtils::isZero( curCoefficient ) || curVar == var ) // If coefficient is zero then nothing to add to the sum, also skip var
			continue;

		realCoefficient = curCoefficient / -ci;
		if ( FloatUtils::isZero( realCoefficient ) )
			continue;

		tempUpper = ( isUpper && realCoefficient > 0 ) || ( !isUpper &&  realCoefficient < 0 ); // If coefficient of lhs and var are different, use same bound
		tempBound = tempUpper ? _upperBoundsExplanations[curVar] : _lowerBoundsExplanations[curVar];
		addVecTimesScalar( sum, tempBound, realCoefficient );
	}

	// Include lhs as well, if needed
	if ( var != row._lhs )
	{
		realCoefficient = 1 / ci;
		if ( !FloatUtils::isZero( realCoefficient ) )
		{
			tempUpper = ( isUpper && realCoefficient > 0 ) || ( !isUpper &&  realCoefficient < 0 ); // If coefficient of lhs and var are different, use same bound
			tempBound = tempUpper ? _upperBoundsExplanations[row._lhs] : _lowerBoundsExplanations[row._lhs];
			addVecTimesScalar( sum, tempBound, realCoefficient );
		}
	}

	extractRowCoefficients( row, rowCoefficients, ci ); // Update according to row coefficients
	addVecTimesScalar( sum, rowCoefficients, 1 );
	injectExplanation( sum, var, isUpper );

	rowCoefficients.clear();
	sum.clear();
}

void BoundsExplainer::updateBoundExplanationSparse( const SparseUnsortedList& row, const bool isUpper, const unsigned var )
{
	if ( row.empty() )
		return;

	ASSERT( var < _varsNum );
	bool tempUpper;
	double curCoefficient, ci = 0, realCoefficient;
	for ( const auto& entry : row )
		if ( entry._index == var )
		{
			ci = entry._value;
			break;
		}

	ASSERT( !FloatUtils::isZero( ci ) );
	std::vector<double> rowCoefficients = std::vector<double>( _rowsNum, 0 );
	std::vector<double> sum = std::vector<double>( _rowsNum, 0 );
	std::vector<double> tempBound;

	for ( const auto& entry : row )
	{
		curCoefficient = entry._value;
		if ( FloatUtils::isZero( curCoefficient ) || entry._index == var ) // If coefficient is zero then nothing to add to the sum, also skip var
			continue;

		realCoefficient = curCoefficient / -ci;
		if ( FloatUtils::isZero( realCoefficient ) )
			continue;

		tempUpper = ( isUpper && realCoefficient > 0 ) || ( !isUpper &&  realCoefficient < 0 ); // If coefficient of lhs and var are different, use same bound
		tempBound = tempUpper ? _upperBoundsExplanations[entry._index] : _lowerBoundsExplanations[entry._index];
		addVecTimesScalar( sum, tempBound, realCoefficient );
	}

	extractSparseRowCoefficients( row, rowCoefficients, ci ); // Update according to row coefficients
	addVecTimesScalar( sum, rowCoefficients, 1 );
	injectExplanation( sum, var, isUpper );

	rowCoefficients.clear();
	sum.clear();
}

void BoundsExplainer::addVecTimesScalar( std::vector<double>& sum, const std::vector<double>& input, const double scalar ) const
{
	if ( input.empty() || FloatUtils::isZero( scalar ) )
		return;

	ASSERT( sum.size() == _rowsNum && input.size() == _rowsNum );

	for ( unsigned i = 0; i < _rowsNum; ++i )
		sum[i] += scalar * input[i];
}

void BoundsExplainer::extractRowCoefficients( const TableauRow& row, std::vector<double>& coefficients, double ci ) const
{
	ASSERT( coefficients.size() == _rowsNum && ( row._size == _varsNum  || row._size == _varsNum - _rowsNum ) );
	//The coefficients of the row m highest-indices vars are the coefficients of slack variables
	for ( unsigned i = 0; i < row._size; ++i )
		if ( row._row[i]._var >= _varsNum - _rowsNum && !FloatUtils::isZero( row._row[i]._coefficient ) )
			coefficients[row._row[i]._var - _varsNum + _rowsNum] = - row._row[i]._coefficient / ci;

	if ( row._lhs >= _varsNum - _rowsNum ) //If the lhs was part of original basis, its coefficient is 1 / ci
		coefficients[row._lhs - _varsNum + _rowsNum] = 1 / ci;
}


void BoundsExplainer::extractSparseRowCoefficients( const SparseUnsortedList& row, std::vector<double>& coefficients, double ci ) const
{
	ASSERT( coefficients.size() == _rowsNum );

	//The coefficients of the row m highest-indices vars are the coefficients of slack variables
	for ( const auto& entry : row )
		if ( entry._index >= _varsNum - _rowsNum && !FloatUtils::isZero( entry._value ) )
			coefficients[entry._index - _varsNum + _rowsNum] = - entry._value / ci;
}

void BoundsExplainer::addVariable()
{
	_rowsNum += 1;
	_varsNum += 1;
	_upperBoundsExplanations.emplace_back( std::vector<double>( 0 ) );
	_lowerBoundsExplanations.emplace_back( std::vector<double>( 0 ) );

	for ( unsigned i = 0; i < _varsNum; ++i )
	{
		if ( !_upperBoundsExplanations[i].empty() )
			_upperBoundsExplanations[i].push_back( 0 );

		if ( !_lowerBoundsExplanations[i].empty() )
			_lowerBoundsExplanations[i].push_back( 0 );
	}
}

void BoundsExplainer::resetExplanation( const unsigned var, const bool isUpper )
{
	ASSERT( var < _varsNum );
	isUpper ? _upperBoundsExplanations[var].clear() : _lowerBoundsExplanations[var].clear();
}

void BoundsExplainer::injectExplanation( const std::vector<double>& expl, unsigned var, bool isUpper )
{
	ASSERT( var < _varsNum && ( expl.empty() || expl.size() == _rowsNum ) );
	std::vector<double> *temp = isUpper ? &_upperBoundsExplanations[var] : &_lowerBoundsExplanations[var];
	if ( temp->size() != expl.size() )
		temp->resize( expl.size() );

	std::copy( expl.begin(), expl.end(), temp->begin() );
}