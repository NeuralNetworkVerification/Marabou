/*********************                                                        */
/*! \file File.cpp
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

#include "CommonError.h"
#include "ConstSimpleData.h"
#include "File.h"
#include "HeapData.h"
#include "MStringf.h"
#include "T/sys/stat.h"
#include "T/unistd.h"
#include "Vector.h"

File::File( const String &path ) : _path( path ), _descriptor( NO_DESCRIPTOR )
{
}

File::~File()
{
    closeIfNeeded();
}

void File::close()
{
    closeIfNeeded();
}

bool File::exists( const String &path )
{
    struct stat DONT_CARE;
    return T::stat( path.ascii(), &DONT_CARE ) == 0;
}

bool File::directory( const String &path )
{
    struct stat fileData;
    memset( &fileData, 0, sizeof(struct stat) );

    if ( T::stat( path.ascii(), &fileData ) == 0 )
        return S_ISDIR( fileData.st_mode );

    return false;
}

unsigned File::getSize( const String &path )
{
    struct stat dataBuffer;
    memset( &dataBuffer, 0, sizeof(dataBuffer) );

    if ( T::stat( path.ascii(), &dataBuffer ) != 0 )
        throw CommonError( CommonError::STAT_FAILED );

    return dataBuffer.st_size;
}

void File::open( Mode openMode )
{
    int flags;
    mode_t mode;

    if ( openMode == File::MODE_READ )
        flags = O_RDONLY;
    else if ( openMode == File::MODE_WRITE_TRUNCATE )
        flags = O_RDWR | O_CREAT | O_TRUNC;
    else
        flags = O_CREAT | O_RDWR | O_APPEND;

    if ( ( openMode == File::MODE_WRITE_APPEND ) || ( openMode == File::MODE_WRITE_TRUNCATE ) )
        mode = S_IRUSR | S_IWUSR;
    else
        mode = (mode_t)NULL;

    if ( ( _descriptor = T::open( _path.ascii(), flags, mode ) ) == NO_DESCRIPTOR )
        throw CommonError( CommonError::OPEN_FAILED, _path.ascii() );
}

void File::write( const String &line )
{
    if ( T::write( _descriptor, line.ascii(), line.length() ) != (int)line.length() )
        throw CommonError( CommonError::WRITE_FAILED );
}

void File::write( const ConstSimpleData &data )
{
    if ( T::write( _descriptor, data.data(), data.size() ) != (int)data.size() )
        throw CommonError( CommonError::WRITE_FAILED );
}

void File::read( HeapData &buffer, unsigned maxReadSize )
{
    Vector<char> readVector( maxReadSize );
    char *readBuffer( readVector.data() );
    int bytesRead;

    if ( ( bytesRead = T::read( _descriptor, readBuffer, sizeof(readBuffer) ) ) == -1 )
        throw CommonError( CommonError::READ_FAILED );

    buffer = ConstSimpleData( readBuffer, bytesRead );
}

String File::readLine( char lineSeparatingChar )
{
    enum {
        SIZE_OF_BUFFER = 10240,
    };

    Stringf separatorAsString( "%c", lineSeparatingChar );

    while ( !_readLineBuffer.contains( separatorAsString ) )
    {
        char *buffer = new char[SIZE_OF_BUFFER + 1];
        int n = T::read( _descriptor, buffer, sizeof(char) * SIZE_OF_BUFFER );
        if ( ( n == -1 ) || ( n == 0 ) )
        {
            delete[] buffer;
            if ( _readLineBuffer != "" )
            {
                String result = _readLineBuffer;
                _readLineBuffer = "";
                return result;
            }

            throw CommonError( CommonError::READ_FAILED );
        }

        buffer[n] = 0;
        _readLineBuffer += buffer;
        delete[] buffer;
    }

    String result = _readLineBuffer.substring( 0, _readLineBuffer.find( separatorAsString ) );
    _readLineBuffer = _readLineBuffer.substring( _readLineBuffer.find( separatorAsString ) + 1,
                                                 _readLineBuffer.length() );
    return result;
}

void File::closeIfNeeded()
{
    if ( _descriptor != NO_DESCRIPTOR )
    {
        T::close( _descriptor );
        _descriptor = NO_DESCRIPTOR;
    }
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
