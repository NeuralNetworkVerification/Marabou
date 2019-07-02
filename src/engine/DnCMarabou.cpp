/*********************                                                        */
/*! \file Marabou.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu
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

#include "DnCManager.h"
#include "DnCMarabou.h"
#include "File.h"
#include "MStringf.h"
#include "Options.h"
#include "PropertyParser.h"
#include "ReluplexError.h"


DnCMarabou::DnCMarabou()
    : _dncManager( nullptr )
{
}

void DnCMarabou::run( Options *options )
{
    // Get options
    unsigned initialDivides = options->getInt( Options::NUM_INITIAL_DIVIDES );
    unsigned initialTimeout = options->getInt( Options::INITIAL_TIMEOUT );
    unsigned numWorkers = options->getInt( Options::NUM_WORKERS );
    unsigned onlineDivides = options->getInt( Options::NUM_ONLINE_DIVIDES );
    unsigned verbosity = options->getInt( Options::VERBOSITY );
    unsigned timeoutInSeconds = options->getInt( Options::TIMEOUT );
    float timeoutFactor = options->getFloat( Options::TIMEOUT_FACTOR );
    String summaryFilePath = options->getString( Options::SUMMARY_FILE );

    String networkFilePath = options->getString( Options::INPUT_FILE_PATH );
    if ( !File::exists( networkFilePath ) )
    {
        printf( "Error: the specified network file (%s) doesn't exist!\n",
                networkFilePath.ascii() );
        throw ReluplexError( ReluplexError::FILE_DOESNT_EXIST,
                             networkFilePath.ascii() );
    }
    printf( "Network: %s\n", networkFilePath.ascii() );

    String propertyFilePath = options->getString( Options::PROPERTY_FILE_PATH );
    if ( !File::exists( propertyFilePath ) )
    {
        printf( "Error: the specified property file (%s) doesn't exist!\n",
                propertyFilePath.ascii() );
        throw ReluplexError( ReluplexError::FILE_DOESNT_EXIST,
                             propertyFilePath.ascii() );
    }
    printf( "Network: %s\n", propertyFilePath.ascii() );


    _dncManager = std::unique_ptr<DnCManager>
      ( new DnCManager ( numWorkers, initialDivides, initialTimeout,
                         onlineDivides, timeoutFactor,
                         DivideStrategy::LargestInterval, networkFilePath,
                         propertyFilePath, verbosity ) );

    struct timespec start = TimeUtils::sampleMicro();

    _dncManager->solve( timeoutInSeconds );

    struct timespec end = TimeUtils::sampleMicro();

    unsigned long long totalElapsed = TimeUtils::timePassed( start, end );
    displayResults( totalElapsed, summaryFilePath );
}


void DnCMarabou::displayResults( unsigned long long microSecondsElapsed,
                                 String summaryFilePath ) const
{
    std::cout << "Total Time: " << microSecondsElapsed / 1000000 << std::endl;
    String resultString = _dncManager->getResultString();

    if ( summaryFilePath != "" )
    {
        File summaryFile( summaryFilePath );
        summaryFile.open( File::MODE_WRITE_TRUNCATE );

        // Field #1: result
        summaryFile.write( resultString );

        // Field #2: total elapsed time
        summaryFile.write( Stringf( " %u ", microSecondsElapsed / 1000000 ) );

        // Field #3: number of visited tree states
        summaryFile.write( Stringf( "0 " ) );

        // Field #4: average pivot time in micro seconds
        summaryFile.write( Stringf( "0" ) );

        summaryFile.write( "\n" );
    }
}
