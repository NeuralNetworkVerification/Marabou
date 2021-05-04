/*********************                                                        */
/*! \file toggleBounds.cpp
 ** \verbatim
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/


#include "ToggleBounds.h"
ToggleBounds::ToggleBounds( const unsigned length )
		:_length( length )
		,_splitUpperBounds( _length, 0 )
		,_inputUpperBounds( _length, 0 )
		,_toggleUpperBounds( _length, false )
		,_splitLowerBounds( _length, 0 )
		,_inputLowerBounds( _length, 0 )
		,_toggleLowerBounds( _length, false )
{

}

double ToggleBounds::getUpperBound( const unsigned index ) const
{
	assert( index < _length);
	return _toggleUpperBounds[index] ? _splitUpperBounds[index] : _inputUpperBounds[index];
}


double ToggleBounds::getLowerBound( const unsigned index ) const
{
	assert( index < _length);
	return _toggleLowerBounds[index] ? _splitLowerBounds[index] : _inputLowerBounds[index];
}


void ToggleBounds::toggleUpper( const unsigned index, const double value, const bool isInput )
{
	assert( index < _length);
	std::vector<double>& temp = isInput? _inputUpperBounds : _splitUpperBounds;
	temp[index] = value;
	_toggleUpperBounds[index] = !isInput;
}


void ToggleBounds::toggleLower( const unsigned index, const double value, const bool isInput )
{
	assert( index < _length);
	std::vector<double>& temp = isInput? _inputLowerBounds : _splitLowerBounds;
	temp[index] = value;
	_toggleLowerBounds[index] = !isInput;
}


void ToggleBounds::revertBoundToInput( const unsigned index, const bool isUpper )
{
	std::vector<bool>& temp = isUpper? _toggleUpperBounds : _toggleLowerBounds;
	temp[index] = false;
}