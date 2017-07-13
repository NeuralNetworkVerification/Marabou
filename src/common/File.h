/*********************                                                        */
/*! \file File.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __File_h__
#define __File_h__

class ConstSimpleData;
class HeapData;

#include "MString.h"

class File
{
public:
    enum Mode {
        MODE_READ,
        MODE_WRITE_APPEND,
        MODE_WRITE_TRUNCATE,
    };

    File( const String &path );
    virtual ~File();
    void close();
    static bool exists( const String &path );
    static bool directory( const String &path );
    static unsigned getSize( const String &path );
    void open( Mode openMode );
    void write( const String &line );
    void write( const ConstSimpleData &data );
    void read( HeapData &buffer, unsigned maxReadSize );
    String readLine( char lineSeparatingChar = '\n' );

private:
    enum {
        NO_DESCRIPTOR = -1,
    };

    String _path;
    int _descriptor;
    String _readLineBuffer;

    void closeIfNeeded();
};

#endif // __File_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
