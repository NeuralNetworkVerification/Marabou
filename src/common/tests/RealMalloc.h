#ifndef __RealMalloc_h__
#define __RealMalloc_h__

#include "T/stdlib.h"

class RealMalloc :
	public T::Real_malloc,
	public T::Real_free,
    public T::Real_realloc
{
};

#endif // __RealMalloc_h__

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
