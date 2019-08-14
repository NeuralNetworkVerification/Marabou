/*********************                                                        */
/*! \file RealMalloc.h
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

#ifndef __RealMalloc_h__
#define __RealMalloc_h__

#include "T/stdlib.h"

class RealMalloc :
	public T::Real_malloc,
	public T::Real_free,
    public T::Real_realloc
{
};

#endif // __RealMalloc_h__

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
