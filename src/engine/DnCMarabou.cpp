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

void DnCMarabou::run()
{
    /*
      Step 1: extract the network
    */
    String networkFilePath = Options::get()->getString( Options::INPUT_FILE_PATH );
    if ( !File::exists( networkFilePath ) )
    {
        printf( "Error: the specified network file (%s) doesn't exist!\n",
                networkFilePath.ascii() );
        throw ReluplexError( ReluplexError::FILE_DOESNT_EXIST,
                             networkFilePath.ascii() );
    }
    printf( "Network: %s\n", networkFilePath.ascii() );

    /*
      Step 2: extract the property in question
    */
    String propertyFilePath = Options::get()->getString( Options::PROPERTY_FILE_PATH );
    if ( propertyFilePath != "" )
    {
        if ( !File::exists( propertyFilePath ) )
        {
            printf( "Error: the specified property file (%s) doesn't exist!\n",
                    propertyFilePath.ascii() );
            throw ReluplexError( ReluplexError::FILE_DOESNT_EXIST,
                                 propertyFilePath.ascii() );
        }
        printf( "Property: %s\n", propertyFilePath.ascii() );
    }
    else
        printf( "Property: None\n" );

    printf( "\n" );

    /*
      Step 3: initialzie the DNC core
    */
    unsigned initialDivides = Options::get()->getInt( Options::NUM_INITIAL_DIVIDES );
    unsigned initialTimeout = Options::get()->getInt( Options::INITIAL_TIMEOUT );
    unsigned numWorkers = Options::get()->getInt( Options::NUM_WORKERS );
    unsigned onlineDivides = Options::get()->getInt( Options::NUM_ONLINE_DIVIDES );
    unsigned verbosity = Options::get()->getInt( Options::VERBOSITY );
    unsigned timeoutInSeconds = Options::get()->getInt( Options::TIMEOUT );
    float timeoutFactor = Options::get()->getFloat( Options::TIMEOUT_FACTOR );

    _dncManager = std::unique_ptr<DnCManager>
      ( new DnCManager( numWorkers, initialDivides, initialTimeout,
                        onlineDivides, timeoutFactor,
                        DivideStrategy::LargestInterval, networkFilePath,
                        propertyFilePath, verbosity ) );

    struct timespec start = TimeUtils::sampleMicro();

    _dncManager->solve( timeoutInSeconds );

    struct timespec end = TimeUtils::sampleMicro();

    unsigned long long totalElapsed = TimeUtils::timePassed( start, end );
    displayResults( totalElapsed );
}

void DnCMarabou::displayResults( unsigned long long microSecondsElapsed ) const
{
    std::cout << "Total Time: " << microSecondsElapsed / 1000000 << std::endl;
    String resultString = _dncManager->getResultString();

    // Create a summary file, if requested
    String summaryFilePath = Options::get()->getString( Options::SUMMARY_FILE );
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

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
