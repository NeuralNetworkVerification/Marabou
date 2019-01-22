/*********************                                                        */
/*! \file ConfigurationError.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifndef __ConfigurationError_h__
#define __ConfigurationError_h__

#include "Error.h"

class ConfigurationError : public Error
{
public:
	enum Code {
        OPTION_KEY_DOESNT_EXIST = 0,
    };

    ConfigurationError( ConfigurationError::Code code ) : Error( "ConfigurationError", (int)code )
	{
	}

    ConfigurationError( ConfigurationError::Code code, const char *userMessage ) : Error( "ConfigurationError", (int)code, userMessage )
    {
    }
};

#endif // __ConfigurationError_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
