/*********************                                                        */
/*! \file toggleBounds.h
 ** \verbatim
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]
 **/

#ifndef __ToggleBounds_h__
#define __ToggleBounds_h__
#include "vector"
#include <assert.h>

class ToggleBounds {
public:
	ToggleBounds( const unsigned length );

	/*
		Gets an input upper bound, or split bound if var was split.
	*/
	double getUpperBound( const unsigned index ) const;

	/*
		Gets an input lower bound, or split bound if var was split.
	*/
	double getLowerBound( const unsigned index ) const;

	/*
		Toggles upper bound between input and split
	 */
	void toggleUpper( const unsigned index, const double value, const bool isInput );


	/*
		Toggles lower bound between input and split
	 */
	void toggleLower( const unsigned index, const double value, const bool isInput );

private:
	unsigned _length;

	std::vector<double> _splitUpperBounds;
	std::vector<double> _inputUpperBounds;
	std::vector<bool> _toggleUpperBounds;

	std::vector<double> _splitLowerBounds;
	std::vector<double> _inputLowerBounds;
	std::vector<bool> _toggleLowerBounds;
};


#endif // __ToggleBounds_h__