/*********************                                                        */
/*! \file BoundsExplanator.h
 ** \verbatim
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]
 **/

#ifndef __BoundsExplanator_h__
#define __BoundsExplanator_h__
#include "TableauRow.h"
#include "Vector.h"

/*
  A class which encapsulates the bounds explanations of a single variable 
*/
class SingleVarBoundsExplanator {
public:
	SingleVarBoundsExplanator( const unsigned length );
	~SingleVarBoundsExplanator();

	/*
	  Puts the values of a bound explanation in the array bound.
	*/
	void getVarBoundExplanation( std::vector<double>& bound, const  bool isUpper ) const;

	/*
	  Updates the values of the bound explanation according to newBound 
	*/
	void updateVarBoundExplanation( std::vector<double>& newBound, const  bool isUpper );

private:
	unsigned _length;
	std::vector<double> _lower;
	std::vector<double> _upper;
};


/*
  A class which encapsulates the bounds explanations of all variables of a tableau
*/
class BoundsExplanator {
public:
	BoundsExplanator( const unsigned varsNum, const unsigned rowsNum );
	~BoundsExplanator();

	/*
	  Puts the values of a bound explanation in the array bound.
	*/
	void getOneBoundExplanation ( std::vector<double>& bound, const unsigned var, const bool isUpper ) const;

	/*
	  Puts the values of a bound explanation in the array bound.
	*/
	SingleVarBoundsExplanator& returnWholeVarExplanation( const unsigned var );

	/*
	  Updates the values of the bound explanation according to newBound
	*/
	void updateBoundExplanation( const TableauRow& row, const bool isUpper );

private:
	unsigned _varsNum;
	unsigned _rowsNum;
	std::vector<SingleVarBoundsExplanator> _bounds;

	/*
	  A helper functions which adds a multiplication of an array by scalar to another array
	*/
	void addVecTimesScalar( std::vector<double>& sum, std::vector<double>& input, const double scalar );

	/*
	  Upon receiving a row, extract coefficients of the original tableau's equations that creates the row
	  It is merely the coefficients of the slack variables.
	  Assumption - the slack variables indices are alwas the last m.
	*/
	void extractRowCoefficients( const TableauRow& row, std::vector<double>& coefficients );
};
#endif // __BoundsExplanator_h__