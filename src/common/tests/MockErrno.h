#ifndef __MockErrno_h__
#define __MockErrno_h__

#include "T/Errno.h"

class MockErrno :
	public T::Base_errorNumber
{
public:
	MockErrno()
	{
        nextErrno = 28;
	}

    int nextErrno;

    int errorNumber()
	{
        return nextErrno;
	}
};

#endif // __MockErrno_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
