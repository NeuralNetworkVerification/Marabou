/*********************                                                        */
/*! \file RegressUtils.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifndef __RegressUtils_h__
#define __RegressUtils_h__

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

#include "MStringf.h"
#include "TimeUtils.h"
#ifdef _WIN32
#include <T/winunistd.h>
#else
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

void printTitle( const String &title )
{
    unsigned length = title.length();
    unsigned rawPadding = length + 5;

    printf( CYN "\n******************************" );
    for ( unsigned i = 0; i < rawPadding; ++i )
        printf( "*" );
    printf( "\n" );

    printf(  "*** Running tests in category: " RESET );
    printf( "%s ", title.ascii() );
    printf( CYN "***\n" );

    printf( "******************************" );
    for ( unsigned i = 0; i < rawPadding; ++i )
        printf( "*" );
    printf( RESET "\n\n" );
}

void printFailed( const String &test, struct timespec start, struct timespec end )
{
    printf( CYN "\t Test: " RESET " %s ", test.ascii() );
    for ( unsigned i = test.length(); i < 25; ++i )
        printf( " " );
    printf( " -- " RED " FAILED " RESET );

    unsigned long long micro = TimeUtils::timePassed( start, end );
    unsigned seconds = micro / 1000000;
    unsigned minutes = seconds / 60;
    unsigned hours = minutes / 60;
    printf( "[%02u:%02u:%02u] (%llu milli)\n", hours, minutes - ( hours * 60 ), seconds - ( minutes * 60 ), micro / 1000 );
}

void printPassed( const String &test, struct timespec start, struct timespec end )
{
    printf( CYN "\t Test: " RESET " %s ", test.ascii() );
    for ( unsigned i = test.length(); i < 25; ++i )
        printf( " " );
    printf( " -- " GRN " PASSED " RESET );

    unsigned long long micro = TimeUtils::timePassed( start, end );
    unsigned seconds = micro / 1000000;
    unsigned minutes = seconds / 60;
    unsigned hours = minutes / 60;
    printf( "[%02u:%02u:%02u] (%llu milli)\n", hours, minutes - ( hours * 60 ), seconds - ( minutes * 60 ), micro / 1000 );
}

int redirectOutputToFile( const String &outputFilePath )
{
    fflush( stdout );
    int outputFile = open( outputFilePath.ascii(), O_WRONLY | O_CREAT | O_TRUNC, 0644 );
    if ( outputFile < 0 )
    {
        printf( "Error redirecting output to file: %s\n", outputFilePath.ascii() );
        exit( 1 );
    }

    int outputStream = dup( STDOUT_FILENO );
    if ( outputStream < 0 )
    {
        printf( "Error duplicating standard output\n" );
        exit( 1 );
    }

    if ( dup2( outputFile, STDOUT_FILENO ) < 0 )
    {
        printf( "Error duplicating %s to standard output\n", outputFilePath.ascii() );
        exit( 1 );
    }

    close( outputFile );
    return outputStream;
}

void restoreOutputStream( int outputStream )
{
    fflush( stdout );
    if ( dup2( outputStream, STDOUT_FILENO ) < 0 )
    {
        printf( "Error restoring output stream\n" );
        exit( 1 );
    }

    close( outputStream );
}

#endif // __RegressUtils_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
