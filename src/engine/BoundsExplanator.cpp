/*********************                                                        */
/*! \file BoundsExplanator.cpp

 **/

#include <string.h>
#include "BoundsExplanator.h"

/* Functions of SingleVarBoundsExplanator */
SingleVarBoundsExplanator::SingleVarBoundsExplanator(const unsigned length)
	:_length(length)
{
	_lower = std::vector<double>(length, 0); // Vector of zeros of size length
	_upper = std::vector<double>(length, 0);
}

SingleVarBoundsExplanator::~SingleVarBoundsExplanator()
{
	_upper.clear();
	_lower.clear();
}

void SingleVarBoundsExplanator::getVarBoundExplanation(std::vector<double>& bound, const bool isUpper) const
{
	ASSERT(bound.size() == _length)

	std::vector<double> temp;
	temp = isUpper ? _upper : _lower;
	for (unsigned i = 0; i < _length; ++i)
		bound[i] == temp[i];
}


void SingleVarBoundsExplanator::updateVarBoundExplanation(std::vector<double>& newBound, const bool isUpper)
{
	ASSERT(newBound.size() == _length)

	std::vector<double> temp;
	temp = isUpper ? _upper : _lower;
	for (unsigned i = 0; i < _length; ++i)
		temp[i] == newBound[i];
}


/* Functions of BoundsExplanator*/
BoundsExplanator::BoundsExplanator(const unsigned varsNum, const unsigned rowsNum)
	:_rowsNum(rowsNum)
	,_varsNum(varsNum)
{
	for (unsigned i = 0; i < varsNum; ++i)
		_bounds.push_back(SingleVarBoundsExplanator(rowsNum));
}

BoundsExplanator::~BoundsExplanator()
{
	_bounds.clear(); //TODO verify memory management
}

void BoundsExplanator::getOneBoundExplanation(std::vector<double>& bound, const unsigned var, const bool isUpper) const
{
	_bounds[var].getVarBoundExplanation(bound, isUpper);
}

SingleVarBoundsExplanator& BoundsExplanator::returnWholeVarExplanation(const unsigned var)
{
	ASSERT(var < _varsNum);
	return _bounds[var];
}

void BoundsExplanator::updateBoundExplanation(const TableauRow& row, const bool isUpper)
{
	bool tempUpper;
	int curCoefficient, var = row._lhs;  // The var to be updated is the lhs of the row
	ASSERT(var < _varsNum); 

	std::vector<double> rowCoefficients = std::vector<double>(_rowsNum, 0);
	std::vector<double> sum = std::vector<double>(_varsNum, 0);
	std::vector<double> tempBound = std::vector<double>(_rowsNum, 0);

	for (unsigned i = 0; i < _varsNum; ++i)
	{
		curCoefficient = row._row[i]._coefficient;
		if (!curCoefficient) //If coefficeint is zero then nothing to add to the sum
			continue;

		tempUpper = curCoefficient < 0 ? !isUpper : isUpper; // If coefficient is negative, then replace kind of bound
		getOneBoundExplanation(tempBound,row._row[i]._var, tempUpper); 
		addVecTimesScalar(sum, tempBound, curCoefficient);
	}

	extractRowCoefficients(row, rowCoefficients); // Update according to row coefficients
	addVecTimesScalar(sum, rowCoefficients, 1); 
	_bounds[var].updateVarBoundExplanation(sum, isUpper);

	tempBound.clear();
	rowCoefficients.clear();
	sum.clear();
}

void BoundsExplanator::addVecTimesScalar(std::vector<double>& sum, std::vector<double>& input, const double scalar)
{
	ASSERT(sum.size() == _rowsNum && input.size() == _rowsNum)

	for (unsigned i = 0; i < _rowsNum; ++i)
		sum[i] += scalar * input[i];
}

void BoundsExplanator::extractRowCoefficients(const TableauRow& row, std::vector<double>& coefficients)
{
	ASSERT(coefficients.size() == _rowsNum && row._size == _varsNum)

	//The coefficients of the row m highest-indices vars are the coefficents of coefficients
	for (unsigned i = 0; i < row._size; ++i)
		if (row._row[i]._var >= _varsNum - _rowsNum)
			coefficients[row._row[i]._var - _varsNum + _rowsNum] = row._row[i]._coefficient;

	if (row._lhs >= _varsNum - _rowsNum) //If the lhs was part of original basis, its coefficient is -1
		coefficients[row._lhs - _varsNum + _rowsNum] = -1;
}