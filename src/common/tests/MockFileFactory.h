/*********************                                                        */
/*! \file MockFileFactory.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Christopher Lazarus
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

#ifndef __MockFileFactory_h__
#define __MockFileFactory_h__

#include "MockFile.h"
#include "T/FileFactory.h"

class MockFileFactory :
	public T::Base_createFile,
	public T::Base_discardFile
{
public:
	MockFile mockFile;

	~MockFileFactory()
	{
		if ( mockFile.wasCreated )
		{
			TS_ASSERT( mockFile.wasDiscarded );
		}
	}

	IFile *createFile( const String &path )
	{
		mockFile.mockConstructor( path );
		return &mockFile;
	}

	void discardFile( IFile *file )
	{
		TS_ASSERT_EQUALS( file, &mockFile );
        mockFile.mockDestructor();
    }
};

#endif // __MockFileFactory_h__

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
