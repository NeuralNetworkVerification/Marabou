/*********************                                                        */
/*! \file ReluplexError.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __ReluplexError_h__
#define __ReluplexError_h__

#include "Error.h"

class ReluplexError : public Error
{
public:
	enum Code {
        ALLOCATION_FAILED = 0,
        VARIABLE_INDEX_OUT_OF_RANGE = 1,
        VARIABLE_DOESNT_EXIST_IN_SOLUTION = 2,
        CANNOT_RESTORE_TABLEAU = 3,
        PARTICIPATING_VARIABLES_ABSENT = 4,
        INVALID_EQUATION_ADDED_TO_TABLEAU = 5,
        INVALID_BOUND_TIGHTENING = 6,
        MISSING_PL_CONSTRAINT_STATE = 7,
        REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT = 8,
        UNBOUNDED_VARIABLES_NOT_YET_SUPPORTED = 9,
        EQUATION_INVALID = 10,
        CANT_INVERT_BASIS_BECAUSE_OF_ETAS = 11,
        PREPROCESSOR_CANT_FIND_NEW_AUXILIARY_VAR = 12,
    };

    ReluplexError( ReluplexError::Code code ) : Error( "ReluplexError", (int)code )
	{
	}

    ReluplexError( ReluplexError::Code code, const char *userMessage ) :
        Error( "ReluplexError", (int)code, userMessage )
    {
    }
};

#endif // __ReluplexError_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
