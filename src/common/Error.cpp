/*********************                                                        */
/*! \file Error.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#include "Error.h"
#include "T/Errno.h"
#include <stdlib.h>
#include <string.h>

Error::Error( const char *errorClass, int code ) : _code( code )
{
    memset( _errorClass, 0, sizeof(_userMessage) );
    memset( _userMessage, 0, sizeof(_userMessage) );

    strncpy( _errorClass, errorClass, BUFFER_SIZE - 1 );
    _errno = T::errorNumber();
}

Error::Error( const char *errorClass, int code, const char *userMessage ) : _code( code )
{
    memset( _errorClass, 0, sizeof(_userMessage) );
    memset( _userMessage, 0, sizeof(_userMessage) );

    strncpy( _errorClass, errorClass, BUFFER_SIZE - 1 );
    setUserMessage( userMessage );

    _errno = T::errorNumber();
}

int Error::getErrno() const
{
    return _errno;
}

int Error::getCode() const
{
    return _code;
}

void Error::setUserMessage( const char *userMessage )
{
    strncpy( _userMessage, userMessage, BUFFER_SIZE - 1 );
}

const char *Error::getErrorClass() const
{
    return _errorClass;
}

const char *Error::getUserMessage() const
{
    return _userMessage;
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
