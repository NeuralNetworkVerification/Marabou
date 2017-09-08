/*********************                                                        */
/*! \file CommonError.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __CommonError_h__
#define __CommonError_h__

#include "Error.h"

class CommonError : public Error
{
public:
	enum Code {
        QUEUE_IS_EMPTY = 0,
        VALUE_DOESNT_EXIST_IN_VECTOR = 1,
        VECTOR_OUT_OF_BOUNDS = 2,
        POPPING_FROM_EMPTY_VECTOR = 3,
        STACK_IS_EMPTY = 4,
        KEY_DOESNT_EXIST_IN_MAP = 5,
        MAP_IS_EMPTY = 6,
        NOT_ENOUGH_MEMORY = 7,
        STAT_FAILED = 8,
        OPEN_FAILED = 9,
        WRITE_FAILED = 10,
        READ_FAILED = 11,
    };

    CommonError( CommonError::Code code ) : Error( "CommonError", (int)code )
	{
	}

    CommonError( CommonError::Code code, const char *userMessage ) : Error( "CommonError", (int)code, userMessage )
    {
    }
};

#endif // __CommonError_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
