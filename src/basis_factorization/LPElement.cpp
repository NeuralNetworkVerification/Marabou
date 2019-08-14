/*********************                                                        */
/*! \file LPElement.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Derek Huang
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#include "EtaMatrix.h"
#include "LPElement.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <utility>

LPElement::LPElement( EtaMatrix *eta, std::pair<unsigned, unsigned> *pair )
	: _eta( NULL )
	, _pair( NULL )
{
    ASSERT( eta || pair );

    if ( pair )
        _pair = pair;
    else
        _eta = eta;
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

LPElement *LPElement::duplicate() const
{
    EtaMatrix *newEta = NULL;
    std::pair<unsigned, unsigned> *newPair = NULL;

    if ( _eta )
        newEta = new EtaMatrix( *_eta );

    if ( _pair )
        newPair = new std::pair<unsigned, unsigned>( *_pair );

    return new LPElement( newEta, newPair );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
