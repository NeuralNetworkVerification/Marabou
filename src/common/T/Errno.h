/*********************                                                        */
/*! \file Errno.h
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

#ifndef __T__Errno_h__
#define __T__Errno_h__

#include <cxxtest/Mock.h>

namespace T
{
	int errorNumber();
}

CXXTEST_SUPPLY( errorNumber,        /* => T::Base_AllocateIrp */
				int,           /* Return type            */
				errorNumber,        /* Name of mock member    */
				(),  /* Prototype              */
				T::errorNumber,        /* Name of real function  */
				()       /* Parameter list         */ );

#endif // __T__Errno_h__

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
