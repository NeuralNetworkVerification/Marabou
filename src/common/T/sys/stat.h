#ifndef __T__sys__Stat_h__
#define __T__sys__Stat_h__

#include <cxxtest/Mock.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

CXXTEST_MOCK_GLOBAL( int,
					 open,
					 ( const char *pathname, int flags, mode_t mode ),
					 ( pathname, flags, mode  ) );

#endif // __T__sys__Stat_h__

//
// Local Variables:
// compile-command: "make -C ../../../.. "
// tags-file-name: "../../../../TAGS"
// c-basic-offset: 4
// End:
//
