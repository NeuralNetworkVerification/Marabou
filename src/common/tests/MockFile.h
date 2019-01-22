/*********************                                                        */
/*! \file MockFile.h
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

#ifndef __MockFile_h__
#define __MockFile_h__

#include "HeapData.h"
#include "IFile.h"

class MockFile : public IFile
{
public:
	MockFile()
	{
		wasCreated = false;
		wasDiscarded = false;
    }

	bool wasCreated;
	bool wasDiscarded;
    String lastPath;

	void mockConstructor( const String &path )
	{
		TS_ASSERT( !wasCreated );
        wasCreated = true;

        lastPath = path;
	}

    void mockDestructor()
    {
        TS_ASSERT( wasCreated );
        TS_ASSERT( !wasDiscarded );

        wasDiscarded = true;
    }

    bool openWasCalled;
    IFile::Mode lastOpenMode;

    void open( Mode mode )
    {
        openWasCalled = true;
        lastOpenMode = mode;
    }

    void write( const String &/* line */ )
    {
    }

    String readLine( char /* lineSeparatingChar */ )
    {
        return "";
    }

    void read( HeapData &/* buffer */, unsigned /* maxReadSize */ )
    {
    }

    void close()
    {
    }
};

#endif // __MockFile_h__

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
