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

#include "BoundsExplanator.h"

/* Functions of SingleVarBoundsExplanator */
SingleVarBoundsExplanator::SingleVarBoundsExplanator( const unsigned length )
	:_upperRecLevel( 0 )
	,_lowerRecLevel ( 0 )
	,_length( length )
	,_lower( _length, 0 )
	,_upper( _length, 0)
{
}

SingleVarBoundsExplanator& SingleVarBoundsExplanator::operator=(const SingleVarBoundsExplanator& other)
{
	if (this == &other)
		return *this;

	_upper.clear();
	_lower.clear();

	_upper.resize( other._upper.size() );
	_lower.resize( other._lower.size() );
	_length = other._length;

	std::copy( other._lower.begin(), other._lower.end(), _lower.begin() );
	std::copy( other._upper.begin(), other._upper.end(), _upper.begin() );
	_lowerRecLevel = other._lowerRecLevel;
	_upperRecLevel = other._upperRecLevel;

	assert( _upper.size() == _length );
	assert( _lower.size() == _length );

	return *this;
}

void SingleVarBoundsExplanator::getVarBoundExplanation( std::vector<double>& bound, const bool isUpper ) const
{
	assert( bound.size() == _length );
	const std::vector<double>& temp = isUpper? _upper : _lower;
	std::copy( temp.begin(), temp.end(), bound.begin() );
}


unsigned SingleVarBoundsExplanator::getLength() const
{
	return _length;
}

void SingleVarBoundsExplanator::updateVarBoundExplanation(const std::vector<double>& newBound, const bool isUpper )
{
	assert( newBound.size() == _length );
	std::vector<double>& temp = isUpper ? _upper : _lower;
	std::copy( newBound.begin(), newBound.end(), temp.begin() );
}


void SingleVarBoundsExplanator::multiplyAllCoefficients( const double alpha, const bool isUpper)
{
	std::vector<double>& temp = isUpper ? _upper : _lower;
	for ( unsigned i = 0; i < _length; ++i )
		temp[i] *= alpha;
}

void SingleVarBoundsExplanator::addEntry( double coefficient )
{
	_length += 1;
	_upper.push_back( coefficient );
	_lower.push_back( coefficient );
}

void SingleVarBoundsExplanator::injectEntry( unsigned position, double coefficient, bool isUpper )
{
	std::vector<double> &temp = isUpper ? _upper : _lower;
	temp[position] = coefficient;
}

void SingleVarBoundsExplanator::assertLengthConsistency()
{
	ASSERT( _length == _upper.size() );
	ASSERT (_length == _lower.size() );
}


/* Functions of BoundsExplanator */
BoundsExplanator::BoundsExplanator( const unsigned varsNum, const unsigned rowsNum )
	:_varsNum( varsNum )
	,_rowsNum( rowsNum )
	,_bounds( varsNum, SingleVarBoundsExplanator( rowsNum ) )
{

}

unsigned BoundsExplanator::getRowsNum() const
{
	return _rowsNum;
}

unsigned BoundsExplanator::getVarsNum() const
{
	return _varsNum;
}

BoundsExplanator& BoundsExplanator::operator=(const BoundsExplanator& other)
{
	if (this == &other)
		return *this;

	_rowsNum = other._rowsNum;
	_varsNum = other._varsNum;

	for ( unsigned i = 0; i < _varsNum; ++i )
		_bounds[i] = other._bounds[i];

	return *this;
}


void BoundsExplanator::getOneBoundExplanation( std::vector<double>& bound, const unsigned var, const bool isUpper ) const
{
	_bounds[var].getVarBoundExplanation( bound, isUpper );
}

SingleVarBoundsExplanator& BoundsExplanator::returnWholeVarExplanation( const unsigned var )
{
	assert ( var < _varsNum );
	return _bounds[var];
}

void BoundsExplanator::updateBoundExplanation( const TableauRow& row, const bool isUpper )
{
	if ( !row._size )
		return;

	bool tempUpper;
	unsigned var = row._lhs, maxLevel = 0, tempLevel,  tempVar;  // The var to be updated is the lhs of the row
	double curCoefficient;
	assert ( var < _varsNum );
	assert( row._size == _varsNum || row._size == _varsNum - _rowsNum );
	std::vector<double> rowCoefficients = std::vector<double>( _rowsNum, 0 );
	std::vector<double> sum = std::vector<double>( _rowsNum, 0 );
	std::vector<double> tempBound = std::vector<double>( _rowsNum, 0 );

	for ( unsigned i = 0; i < row._size; ++i )
	{
		curCoefficient = row._row[i]._coefficient;
		if ( FloatUtils::isZero( curCoefficient ) ) // If coefficient is zero then nothing to add to the sum
			continue;

		tempUpper = curCoefficient < 0 ? !isUpper : isUpper; // If coefficient is negative, then replace kind of bound
		tempVar = row._row[i]._var;
		getOneBoundExplanation( tempBound, tempVar, tempUpper );
		addVecTimesScalar( sum, tempBound, curCoefficient );


		// TODO delete when completing
		tempLevel = tempUpper? _bounds[tempVar]._upperRecLevel : _bounds[tempVar]._lowerRecLevel;
		if (tempLevel > maxLevel)
			maxLevel = tempLevel;
	}

	extractRowCoefficients( row, rowCoefficients ); // Update according to row coefficients
	addVecTimesScalar( sum, rowCoefficients, 1 );
	_bounds[var].updateVarBoundExplanation( sum, isUpper );
	++maxLevel;
	if ( isUpper )
		_bounds[var]._upperRecLevel = maxLevel;
	else
		_bounds[var]._lowerRecLevel = maxLevel;
	tempBound.clear();
	rowCoefficients.clear();
	sum.clear();
}

void BoundsExplanator::updateBoundExplanation( const TableauRow& row, const bool isUpper, const unsigned var)
{
	if ( !row._size )
		return;

	assert ( var < _varsNum );
	if ( var == row._lhs )
	{
		updateBoundExplanation( row, isUpper );
		return;
	}

	// Find index of variable
	int varIndex = -1;
	for ( unsigned i = 0; i < row._size; ++i )
		if ( row._row[i]._var == var )
		{
			varIndex = (int) i;
			break;
		}

	assert ( varIndex >= 0 );
	double ci = row[varIndex];
	assert ( ci );  // Coefficient of var cannot be zero.
	double coeff = - 1 / ci;
	// Create a new row with var as its lhs
	TableauRow equiv = TableauRow( row._size );
	equiv._lhs = var;
	equiv._scalar = FloatUtils::isZero( row._scalar ) ? 0 :  row._scalar * coeff;

	for ( unsigned i = 0; i < row._size; ++i )
	{
		if ( !FloatUtils::isZero( row[i] ) ) // Updates of zero coefficients are unnecessary
		{
			equiv._row[i]._coefficient = row[i] * coeff;
			equiv._row[i]._var = row._row[i]._var;
		}
		else
		{
			equiv._row[i]._coefficient = 0;
			equiv._row[i]._var = row._row[i]._var;
		}
	}

	// Since the original var is the new lhs, the new var should be replaced with original lhs 
	equiv._row[varIndex]._coefficient = -coeff;
	equiv._row[varIndex]._var = row._lhs;

	updateBoundExplanation( equiv, isUpper );
}

void BoundsExplanator::updateBoundExplanationSparse( const SparseUnsortedList& row, const bool isUpper, const unsigned var )
{
	if ( row.empty() )
		return;

	assert( var < _varsNum );
	bool tempUpper;
	double curCoefficient, ci = 0, realCoefficient;
	for ( const auto& entry : row )
		if ( entry._index == var )
		{
			ci = entry._value;
			break;
		}

	assert( ci );
	unsigned tempLevel, maxLevel = 0;
	std::vector<double> rowCoefficients = std::vector<double>( _rowsNum, 0 );
	std::vector<double> sum = std::vector<double>( _rowsNum, 0 );
	std::vector<double> tempBound = std::vector<double>( _rowsNum, 0 );

	for ( const auto& entry : row )
	{
		curCoefficient = entry._value;
		if ( FloatUtils::isZero( curCoefficient ) || entry._index == var ) // If coefficient is zero then nothing to add to the sum, also skip var
			continue;
		realCoefficient = curCoefficient / -ci;
		if ( FloatUtils::isZero( realCoefficient ) )
			continue;

		tempUpper =  ( isUpper && FloatUtils::isPositive( realCoefficient ) ) ||  ( !isUpper && FloatUtils::isNegative( realCoefficient ) ); // If coefficient of lhs and var are different, use same bound
		getOneBoundExplanation( tempBound, entry._index, tempUpper );
		addVecTimesScalar( sum, tempBound, realCoefficient);
		tempLevel = tempUpper? _bounds[entry._index]._upperRecLevel : _bounds[entry._index]._lowerRecLevel;
		if ( tempLevel > maxLevel )
			maxLevel = tempLevel;
	}

	extractSparseRowCoefficients( row, rowCoefficients, ci ); // Update according to row coefficients
	addVecTimesScalar( sum, rowCoefficients, 1 );
	_bounds[var].updateVarBoundExplanation( sum, isUpper );

	++maxLevel;
	if ( isUpper )
		_bounds[var]._upperRecLevel = maxLevel;
	else
		_bounds[var]._lowerRecLevel = maxLevel;

	tempBound.clear();
	rowCoefficients.clear();
	sum.clear();
}


void BoundsExplanator::multiplyExplanationCoefficients( const unsigned var, const double alpha, const bool isUpper )
{
	_bounds[var].multiplyAllCoefficients( alpha, isUpper );
}


void BoundsExplanator::addVecTimesScalar( std::vector<double>& sum, const std::vector<double>& input, const double scalar ) const
{
	assert( sum.size() == _rowsNum && input.size() == _rowsNum );

	if (FloatUtils::isZero( scalar) )
		return;

	for ( unsigned i = 0; i < _rowsNum; ++i )
		sum[i] += scalar * input[i];
}

void BoundsExplanator::extractRowCoefficients( const TableauRow& row, std::vector<double>& coefficients ) const
{
	assert( coefficients.size() == _rowsNum && ( row._size == _varsNum  || row._size == _varsNum - _rowsNum ) );
	//The coefficients of the row m highest-indices vars are the coefficients of slack variables
	for ( unsigned i = 0; i < row._size; ++i )
		if ( row._row[i]._var >= _varsNum - _rowsNum && !FloatUtils::isZero( row._row[i]._coefficient ) )
			coefficients[row._row[i]._var - _varsNum + _rowsNum] = row._row[i]._coefficient;

	if ( row._lhs >= _varsNum - _rowsNum ) //If the lhs was part of original basis, its coefficient is -1
		coefficients[row._lhs - _varsNum + _rowsNum] = -1;
}


void BoundsExplanator::extractSparseRowCoefficients( const SparseUnsortedList& row, std::vector<double>& coefficients, double ci ) const
{
	assert( coefficients.size() == _rowsNum );

	//The coefficients of the row m highest-indices vars are the coefficients of slack variables
	for ( const auto& entry : row )
		if ( entry._index >= _varsNum - _rowsNum && !FloatUtils::isZero( entry._value ) )
			coefficients[entry._index - _varsNum + _rowsNum] = - entry._value / ci;
}

std::vector<SingleVarBoundsExplanator>& BoundsExplanator::getExplanations()
{
	return _bounds;
}

void BoundsExplanator::addZeroExplanation()
{
	_rowsNum += 1;
	_varsNum += 1;
	_bounds.emplace_back( _rowsNum );
}

void BoundsExplanator::resetExplanation ( const unsigned var, const bool isUpper )
{
	_bounds[var].updateVarBoundExplanation( std::vector<double>( _rowsNum, 0.0 ), isUpper);
	_bounds[var]._lowerRecLevel = 0;
	_bounds[var]._upperRecLevel = 0;
}

void BoundsExplanator::injectExplanation( unsigned var, SingleVarBoundsExplanator& expl )
{
	assert( expl.getLength() == _bounds[var].getLength() );
	_bounds[var] = expl;
}