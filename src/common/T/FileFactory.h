#ifndef __T__FileFactory_h__
#define __T__FileFactory_h__

#include "cxxtest/Mock.h"

class IFile;
class String;

namespace T
{
	IFile *createFile( const String &path );
	void discardFile( IFile *file );
}

CXXTEST_SUPPLY( createFile,        /* => T::Base_AllocateIrp */
				IFile *,           /* Return type            */
				createFile,        /* Name of mock member    */
				( const String &path ),  /* Prototype              */
				T::createFile,        /* Name of real function  */
				( path )       /* Parameter list         */ );

CXXTEST_SUPPLY_VOID( discardFile,        /* => T::Base_AllocateIrp */
					 discardFile,        /* Name of mock member    */
					 ( IFile *file ),  /* Prototype              */
					 T::discardFile,        /* Name of real function  */
					 ( file )            /* Parameter list         */ );

#endif // __T__FileFactory_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
