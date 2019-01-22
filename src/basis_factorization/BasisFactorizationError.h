/*********************                                                        */
/*! \file BasisFactorizationError.h
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


/*********************                                                        */
/*! \file BasisFactorizationError.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __BasisFactorizationError_h__
#define __BasisFactorizationError_h__

#include "Error.h"

class BasisFactorizationError : public Error
{
public:
	enum Code {
        ALLOCATION_FAILED = 0,
        CANT_INVERT_BASIS_BECAUSE_OF_ETAS = 1,
        UNKNOWN_BASIS_FACTORIZATION_TYPE = 2,
        CORRUPT_PERMUATION_MATRIX = 3,
        CANT_INVERT_BASIS_BECAUSE_BASIS_ISNT_AVAILABLE = 4,
        GAUSSIAN_ELIMINATION_FAILED = 5,
        FEATURE_NOT_YET_SUPPORTED = 6,
    };

    BasisFactorizationError( BasisFactorizationError::Code code ) :
      Error( "BasisFactorizationError", (int)code )
	{
	}

    BasisFactorizationError( BasisFactorizationError::Code code, const char *userMessage ) :
        Error( "BasisFactorizationError", (int)code, userMessage )
    {
    }
};

#endif // __BasisFactorizationError_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
