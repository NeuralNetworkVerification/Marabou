/*********************                                                        */
/*! \file LPElement.h
** \verbatim
** Top contributors (to current version):
**   Derek Huang
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __LPElement_h__
#define __LPElement_h__

#include "Debug.h"
#include <utility>

class EtaMatrix;

class LPElement
{
public:
	LPElement( EtaMatrix *_eta, std::pair<unsigned, unsigned> *_pair );
	~LPElement();
    void dump() const;

	EtaMatrix *_eta;
	std::pair<unsigned, unsigned> *_pair;
};

#endif //__LPElement_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
