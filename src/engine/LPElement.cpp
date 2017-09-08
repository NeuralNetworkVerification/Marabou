/*********************                                                        */
/*! \file LPElement.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Derek Huang
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "EtaMatrix.h"
#include "LPElement.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <utility>

LPElement::LPElement( const EtaMatrix *eta, const std::pair<unsigned, unsigned> *pair )
	: _eta( NULL )
	, _pair( NULL )
{
    if ( pair )
    {
		_pair = new std::pair<unsigned, unsigned>;
		memcpy( _pair, pair, sizeof(std::pair<unsigned, unsigned>) );
	}
    else
		_eta = new EtaMatrix( eta->_m, eta->_columnIndex, eta->_column );
}

LPElement::~LPElement()
{
	if ( _eta )
	{
		delete _eta;
		_eta = NULL;
	}

	if ( _pair )
	{
		delete _pair;
		_pair = NULL;
	}
}

void LPElement::dump() const
{
    printf( "Dumping LP Element:\n" );
    if ( _pair )
        printf( "P element: swap rows %u, %u\n", _pair->first, _pair->second );
    else
    {
        printf( "Eta element\n" );
        _eta->dump();
    }
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
