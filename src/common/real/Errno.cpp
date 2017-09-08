#include <errno.h>

namespace T
{
    int errorNumber()
	{
		return errno;
    }
}

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
