/*********************                                                        */
/*! \file Error.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Reluplex project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __Error_h__
#define __Error_h__

class Error
{
public:
    Error( const char *errorClass, unsigned code );
    Error( const char *errorClass, unsigned code, const char *userMessage );
    int getErrno() const;
    int getCode() const;
    void setUserMessage( const char *userMessage );
    const char *getErrorClass() const;
    const char *getUserMessage() const;

private:
    enum {
        BUFFER_SIZE = 2048,
    };

    char _errorClass[BUFFER_SIZE];
    int _code;
    int _errno;
    char _userMessage[BUFFER_SIZE];
};

#endif // __Error_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
