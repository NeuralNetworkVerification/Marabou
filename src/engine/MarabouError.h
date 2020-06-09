/*********************                                                        */
/*! \file MarabouError.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Christopher Lazarus
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifndef __MarabouError_h__
#define __MarabouError_h__

#include "Error.h"

class MarabouError : public Error
{
public:
	enum Code {
        ALLOCATION_FAILED = 0,
        VARIABLE_INDEX_OUT_OF_RANGE = 1,
        VARIABLE_DOESNT_EXIST_IN_SOLUTION = 2,
        PARTICIPATING_VARIABLES_ABSENT = 3,
        NON_EQUALITY_INPUT_EQUATION_DISCOVERED = 4,
        SIMULATOR_ERROR = 5,
        MISSING_PL_CONSTRAINT_STATE = 6,
        REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT = 7,
        UNBOUNDED_VARIABLES_NOT_YET_SUPPORTED = 8,
        EQUATION_INVALID = 9,
        RESTORING_ENGINE_FROM_INVALID_STATE = 10,
        RESTORATION_FAILED_TO_RESTORE_PRECISION = 11,
        CANNOT_RESTORE_TABLEAU = 12,
        FAILURE_TO_ADD_NEW_EQUATION = 13,
        RESTORATION_FAILED_TO_REFACTORIZE_BASIS = 14,
        FILE_DOESNT_EXIST = 15,
        SYMBOLIC_BOUND_TIGHTENER_FAULTY_INPUT = 16,
        SYMBOLIC_BOUND_TIGHTENER_UNSUPPORTED_CONSTRAINT_TYPE = 17,
        SYMBOLIC_BOUND_TIGHTENER_OPTION_NOT_SUPPORTED = 18,
        SYMBOLIC_BOUND_TIGHTENER_UNKNOWN_VARIABLE_INDEX = 19,
        MERGED_INPUT_VARIABLE = 20,
        MERGED_OUTPUT_VARIABLE = 21,
        INVALID_WEIGHTED_SUM_INDEX = 22,
        UNSUCCESSFUL_QUEUE_PUSH = 23,
        NETWORK_LEVEL_REASONER_ACTIVATION_NOT_SUPPORTED = 24,

        // Error codes for Query Loader
        FILE_DOES_NOT_EXIST = 100,
        INVALID_EQUATION_TYPE = 101,
        UNSUPPORTED_PIECEWISE_CONSTRAINT = 102,

        FEATURE_NOT_YET_SUPPORTED = 900,

        DEBUGGING_ERROR = 999,
    };

    MarabouError( MarabouError::Code code ) : Error( "MarabouError", (int)code )
	{
	}

    MarabouError( MarabouError::Code code, const char *userMessage ) :
        Error( "MarabouError", (int)code, userMessage )
    {
    }
};

#endif // __MarabouError_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
