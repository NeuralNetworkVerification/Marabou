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
#include "MarabouError.h"
#include "OnnxParser.h"
#include "QueryLoader.h"
#include "AcasParser.h"

DnCMarabou::DnCMarabou( InputQuery &inputQuery )
    : _inputQuery( inputQuery )
    , _dncManager( nullptr )
{

}

void DnCMarabou::run()
{
    // Initialize the DNC core
    _dncManager = std::unique_ptr<DnCManager>
        ( new DnCManager( &_inputQuery ) );

    struct timespec start = TimeUtils::sampleMicro();

    _dncManager->solve();

    struct timespec end = TimeUtils::sampleMicro();

    unsigned long long totalElapsed = TimeUtils::timePassed( start, end );
    displayResults( totalElapsed );
}

void DnCMarabou::displayResults( unsigned long long microSecondsElapsed ) const
{
    _dncManager->printResult();
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
