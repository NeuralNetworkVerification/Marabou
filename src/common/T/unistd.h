#ifndef __T__Unistd_h__
#define __T__Unistd_h__

#include <cxxtest/Mock.h>
#include <unistd.h>

typedef struct stat StructStat;

CXXTEST_MOCK_GLOBAL( int,
					 close,
					 ( int fd ),
					 ( fd ) );

CXXTEST_MOCK_GLOBAL( pid_t,
					 fork,
					 (),
					 () );

CXXTEST_MOCK_GLOBAL( ssize_t,
                     write,
                     ( int fd, const void *buf, size_t count ),
                     ( fd, buf, count ) );

CXXTEST_MOCK_GLOBAL( ssize_t,
                     read,
                     ( int fd, void *buf, size_t count ),
                     ( fd, buf, count ) );

CXXTEST_MOCK_VOID_GLOBAL( _exit,
                          ( int status ),
                          ( status ) );

CXXTEST_MOCK_GLOBAL( int,
                     stat,
                     ( const char *path, StructStat *buf ),
                     ( path, buf ) );

CXXTEST_MOCK_GLOBAL( unsigned,
                     sleep,
                     ( unsigned seconds ),
                     ( seconds ) );

CXXTEST_MOCK_GLOBAL( int,
                     pipe,
                     ( int fildes[2] ),
                     ( fildes ) );

CXXTEST_MOCK_GLOBAL( int,
                     execvp,
                     ( const char *file, char *const argv[] ),
                     ( file, argv ) );

CXXTEST_MOCK_GLOBAL( int,
                     dup2,
                     ( int oldfd, int newfd ),
                     ( oldfd, newfd ) );

#endif // __T__Unistd_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
