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
