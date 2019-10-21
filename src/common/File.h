/*********************                                                        */
/*! \file File.h
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

#ifndef __File_h__
#define __File_h__

class ConstSimpleData;
class HeapData;

#include "IFile.h"
#include "MString.h"

#ifdef _WIN32
#include <io.h>
#define S_ISDIR(mode)   (((mode) & S_IFMT) == S_IFDIR)

typedef int mode_t;

/// @Note If STRICT_UGO_PERMISSIONS is not defined, then setting Read for any
///       of User, Group, or Other will set Read for User and setting Write
///       will set Write for User.  Otherwise, Read and Write for Group and
///       Other are ignored.
///
/// @Note For the POSIX modes that do not have a Windows equivalent, the modes
///       defined here use the POSIX values left shifted 16 bits.

static const mode_t S_ISUID = 0x08000000;           ///< does nothing
static const mode_t S_ISGID = 0x04000000;           ///< does nothing
static const mode_t S_ISVTX = 0x02000000;           ///< does nothing
static const mode_t S_IRUSR = mode_t(_S_IREAD);     ///< read by user
static const mode_t S_IWUSR = mode_t(_S_IWRITE);    ///< write by user
static const mode_t S_IXUSR = 0x00400000;           ///< does nothing
#   ifndef STRICT_UGO_PERMISSIONS
static const mode_t S_IRGRP = mode_t(_S_IREAD);     ///< read by *USER*
static const mode_t S_IWGRP = mode_t(_S_IWRITE);    ///< write by *USER*
static const mode_t S_IXGRP = 0x00080000;           ///< does nothing
static const mode_t S_IROTH = mode_t(_S_IREAD);     ///< read by *USER*
static const mode_t S_IWOTH = mode_t(_S_IWRITE);    ///< write by *USER*
static const mode_t S_IXOTH = 0x00010000;           ///< does nothing
#   else
static const mode_t S_IRGRP = 0x00200000;           ///< does nothing
static const mode_t S_IWGRP = 0x00100000;           ///< does nothing
static const mode_t S_IXGRP = 0x00080000;           ///< does nothing
static const mode_t S_IROTH = 0x00040000;           ///< does nothing
static const mode_t S_IWOTH = 0x00020000;           ///< does nothing
static const mode_t S_IXOTH = 0x00010000;           ///< does nothing
#   endif
#endif

class File : public IFile
{
public:
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
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
