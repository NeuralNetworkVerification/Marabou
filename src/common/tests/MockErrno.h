/*********************                                                        */
/*! \file MockErrno.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief [[ Add one-line brief description here ]]
 **
 ** [[ Add lengthier description here ]]
 **/

#ifndef __MockErrno_h__
#define __MockErrno_h__

#include "T/Errno.h"

class MockErrno :
	public T::Base_errorNumber
{
public:
	MockErrno()
	{
        nextErrno = 28;
	}

    int nextErrno;

    int errorNumber()
	{
        return nextErrno;
	}
};

#endif // __MockErrno_h__

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
