#ifndef __T__Errno_h__
#define __T__Errno_h__

#include <cxxtest/Mock.h>

namespace T
{
	int errorNumber();
}

CXXTEST_SUPPLY( errorNumber,        /* => T::Base_AllocateIrp */
				int,           /* Return type            */
				errorNumber,        /* Name of mock member    */
				(),  /* Prototype              */
				T::errorNumber,        /* Name of real function  */
				()       /* Parameter list         */ );

#endif // __T__Errno_h__

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
