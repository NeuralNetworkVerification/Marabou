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
	,_lower( )
	,_upper( )
{
	_upper.push(  { std::vector<double>( _length, 0 ), 0 } );
	_lower.push( { std::vector<double>( _length, 0 ), 0 } );
}

void SingleVarBoundsExplanator::getVarBoundExplanation( std::vector<double>& bound, const bool isUpper ) const
{
	assert( bound.size() == _length );

	const std::vector<double>& temp = isUpper? _upper.top()._bound : _lower.top()._bound;
	for ( unsigned i = 0; i < _length; ++i )
		bound[i] = temp[i];
}


void SingleVarBoundsExplanator::updateVarBoundExplanation( const DynamicBound& newBound, const bool isUpper )
{
	assert( newBound._bound.size() == _length );

	DynamicBound& temp = isUpper? _upper.top() : _lower.top();
	if ( newBound._depth > temp._depth )
	{
		isUpper? _upper.push(newBound) : _lower.push(newBound);
		return;
	}
	temp._bound = newBound._bound;
	// Doesn't update depth, so the depth cannot decrease
	// TODO change to enable deducing bounds independent of the current split
}

unsigned SingleVarBoundsExplanator::getExplanationDepth(const bool isUpper) const
{
	return isUpper? _upper.top()._depth : _lower.top()._depth;
}

void SingleVarBoundsExplanator::popUntilDepth( const unsigned depth)
{
	while(_upper.top()._depth > depth)
		_upper.pop();

	while(_lower.top()._depth > depth)
		_lower.pop();
}


void SingleVarBoundsExplanator::imposeBound( const unsigned depth, const bool isUpper)
{
	std::stack<DynamicBound>& temp = isUpper? _upper : _lower;
	temp.push( {std::vector<double>(_length, 0), depth });
}

/* Functions of BoundsExplanator*/
BoundsExplanator::BoundsExplanator( const unsigned varsNum, const unsigned rowsNum )
	:_varsNum( varsNum )
	,_rowsNum( rowsNum )
	,_bounds( varsNum, SingleVarBoundsExplanator( rowsNum ) )
{

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
	bool tempUpper;
	unsigned var = row._lhs, maxLevel = 0, maxDepth = 0, tempLevel,  tempVar;  // The var to be updated is the lhs of the row
	double curCoefficient;
	assert ( var < _varsNum );
	std::vector<double> rowCoefficients = std::vector<double>( _rowsNum, 0 );
	std::vector<double> sum = std::vector<double>( _rowsNum, 0 );
	std::vector<double> tempBound = std::vector<double>( _rowsNum, 0 );

	for ( unsigned i = 0; i < row._size; ++i )
	{
		curCoefficient = row._row[i]._coefficient;
		if ( FloatUtils::isZero(curCoefficient) ) // If coefficient is zero then nothing to add to the sum
			continue;

		tempUpper = curCoefficient < 0 ? !isUpper : isUpper; // If coefficient is negative, then replace kind of bound
		tempVar = row._row[i]._var;
		getOneBoundExplanation( tempBound, tempVar, tempUpper );
		addVecTimesScalar( sum, tempBound, curCoefficient );

		if ( maxDepth > _bounds[tempVar].getExplanationDepth( tempUpper ) )
			maxDepth = _bounds[tempVar].getExplanationDepth( tempUpper );

		// TODO delete when completing
		tempLevel = tempUpper? _bounds[tempVar]._upperRecLevel : _bounds[tempVar]._lowerRecLevel;
		if (tempLevel > maxLevel)
			maxLevel = tempLevel;
	}

	extractRowCoefficients( row, rowCoefficients ); // Update according to row coefficients
	addVecTimesScalar( sum, rowCoefficients, 1 ); 
	_bounds[var].updateVarBoundExplanation( { sum, maxDepth }, isUpper );
	++maxLevel;
	if ( isUpper )
		_bounds[var]._upperRecLevel = maxLevel;
	else
		_bounds[var]._lowerRecLevel = maxLevel;
	printf("Recursion level update: %d  of var %d\n", maxLevel, var);


	tempBound.clear();
	rowCoefficients.clear();
	sum.clear();
}

void BoundsExplanator::updateBoundExplanation( const TableauRow& row, const bool isUpper, const unsigned varIndex )
{
	unsigned var = row._row[varIndex]._var;
	assert ( var < _varsNum );
	if ( var == row._lhs )
	{
		updateBoundExplanation( row, isUpper );
		return;
	}
			
	double ci = row[varIndex]; 
	assert ( ci );  // Coefficient of var cannot be zero.
	double coeff = 1 / ci;
	// Create a new row with var as its lhs
	TableauRow equiv = TableauRow( row._size );
	equiv._lhs = var;
	equiv._scalar = - row._scalar * coeff;

	for ( unsigned i = 0; i < row._size; ++i )
	{
	
		if( row[i] ) // Updates of zero coefficients are unnecessary 
			{
				equiv._row[i]._coefficient = - row[i] * coeff; 
				equiv._row[i]._var = row._row[i]._var;
			}
	}
	// Since the original var is the new lhs, the new var should be replaced with original lhs 
	equiv._row[varIndex]._coefficient = coeff;
	equiv._row[varIndex]._var = row._lhs;

	updateBoundExplanation( equiv, isUpper );
}

void BoundsExplanator::updateBoundExplanationSparse( const SparseUnsortedList& row, const bool isUpper, const unsigned var )
{
	ASSERT( var < _varsNum );
	bool tempUpper;
	double curCoefficient, ci = 0;
	unsigned maxDepth = 0;
	for ( const auto& entry : row )
		if ( entry._index == var )
			ci = entry._value;

	ASSERT( ci );

	std::vector<double> rowCoefficients = std::vector<double>( _rowsNum, 0 );
	std::vector<double> sum = std::vector<double>( _rowsNum, 0 );
	std::vector<double> tempBound = std::vector<double>( _rowsNum, 0 );

	for ( const auto& entry : row )
	{
		curCoefficient = entry._value;
		if ( !FloatUtils::isZero( curCoefficient ) || entry._index == var ) // If coefficient is zero then nothing to add to the sum, also skip var
			continue;

		tempUpper = curCoefficient < 0 ? !isUpper : isUpper; // If coefficient is negative, then replace kind of bound
		if ( maxDepth > _bounds[entry._index].getExplanationDepth( tempUpper ) )
			maxDepth = _bounds[entry._index].getExplanationDepth( tempUpper );

		getOneBoundExplanation( tempBound, entry._index, tempUpper );
		addVecTimesScalar( sum, tempBound, curCoefficient / -ci );
	}

	extractSparseRowCoefficients( row, rowCoefficients, ci ); // Update according to row coefficients
	addVecTimesScalar( sum, rowCoefficients, 1 );
	_bounds[var].updateVarBoundExplanation( { sum, maxDepth }, isUpper );

	tempBound.clear();
	rowCoefficients.clear();
	sum.clear();
}


void BoundsExplanator::addVecTimesScalar( std::vector<double>& sum, const std::vector<double>& input, const double scalar ) const
{
	assert( sum.size() == _rowsNum && input.size() == _rowsNum );

	for ( unsigned i = 0; i < _rowsNum; ++i )
		sum[i] += scalar * input[i];
}

void BoundsExplanator::extractRowCoefficients( const TableauRow& row, std::vector<double>& coefficients ) const
{
	assert( coefficients.size() == _rowsNum && ( row._size == _varsNum  || row._size == _varsNum - _rowsNum ) );
	//The coefficients of the row m highest-indices vars are the coefficients of slack variables
	for ( unsigned i = 0; i < row._size; ++i )
		if ( row._row[i]._var >= _varsNum - _rowsNum )
			coefficients[row._row[i]._var - _varsNum + _rowsNum] = row._row[i]._coefficient;

	if ( row._lhs >= _varsNum - _rowsNum ) //If the lhs was part of original basis, its coefficient is -1
		coefficients[row._lhs - _varsNum + _rowsNum] = -1;
}


void BoundsExplanator::extractSparseRowCoefficients( const SparseUnsortedList& row, std::vector<double>& coefficients, double ci ) const
{
	assert( coefficients.size() == _rowsNum );

	//The coefficients of the row m highest-indices vars are the coefficients of slack variables
	for ( const auto& entry : row )
		if ( entry._index >= _varsNum - _rowsNum )
				coefficients[entry._index - _varsNum + _rowsNum] = entry._value / -ci;
}


void BoundsExplanator::popAllStacksUntilDepth( const unsigned depth)
{
	for (SingleVarBoundsExplanator expl : _bounds)
		expl.popUntilDepth(depth);
}

void BoundsExplanator::imposeNewExplanation( const unsigned index, const unsigned depth, const bool isUpper)
{
	_bounds[index].imposeBound(depth, isUpper);
}