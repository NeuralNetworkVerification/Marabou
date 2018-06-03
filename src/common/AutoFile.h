/*********************                                                        */
/*! \file AutoFile.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __AutoFile_h__
#define __AutoFile_h__

#include "IFile.h"
#include "T/FileFactory.h"

class AutoFile
{
public:
	AutoFile( const String &path )
	{
		_file = T::createFile( path );
	}

	~AutoFile()
	{
		T::discardFile( _file );
		_file = 0;
	}

	operator IFile &()
	{
		return *_file;
	}

	IFile *operator->()
	{
		return _file;
	}

private:
	IFile *_file;
};

#endif // __AutoFile_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
