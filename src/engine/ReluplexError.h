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
        PARTICIPATING_VARIABLES_ABSENT = 3,
        INVALID_EQUATION_ADDED_TO_TABLEAU = 4,
        INVALID_BOUND_TIGHTENING = 5,
        MISSING_PL_CONSTRAINT_STATE = 6,
        REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT = 7,
        UNBOUNDED_VARIABLES_NOT_YET_SUPPORTED = 8,
        EQUATION_INVALID = 9,
        RESTORING_ENGINE_FROM_INVALID_STATE = 10,
        PREPROCESSOR_CANT_FIND_NEW_AUXILIARY_VAR = 11,
        RESTORATION_FAILED_TO_RESTORE_PRECISION = 12,
        CANNOT_RESTORE_TABLEAU = 13,

        DEBUGGING_ERROR = 999,
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
