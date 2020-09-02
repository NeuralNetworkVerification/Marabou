/*********************                                                        */
/*! \file Marabou.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
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

#include "AcasParser.h"
#include "GlobalConfiguration.h"
#include "File.h"
#include "MStringf.h"
#include "Marabou.h"
#include "Options.h"
#include "PropertyParser.h"
#include "MarabouError.h"
#include "QueryLoader.h"

#ifdef _WIN32
#undef ERROR
#endif

Marabou::Marabou( unsigned verbosity )
    : _acasParser( NULL )
    , _engine( verbosity )
{
}

Marabou::~Marabou()
{
    if ( _acasParser )
    {
        delete _acasParser;
        _acasParser = NULL;
    }
}

void Marabou::run()
{
    struct timespec start = TimeUtils::sampleMicro();

    prepareInputQuery();
    solveQuery();

    struct timespec end = TimeUtils::sampleMicro();

    unsigned long long totalElapsed = TimeUtils::timePassed( start, end );
    displayResults( totalElapsed );
}

void Marabou::prepareInputQuery()
{
    String inputQueryFilePath = Options::get()->getString( Options::INPUT_QUERY_FILE_PATH );
    if ( inputQueryFilePath.length() > 0 )
    {
        /*
          Step 1: extract the query
        */
        if ( !File::exists( inputQueryFilePath ) )
        {
            printf( "Error: the specified inputQuery file (%s) doesn't exist!\n", inputQueryFilePath.ascii() );
            throw MarabouError( MarabouError::FILE_DOESNT_EXIST, inputQueryFilePath.ascii() );
        }

        printf( "InputQuery: %s\n", inputQueryFilePath.ascii() );
        _inputQuery = QueryLoader::loadQuery( inputQueryFilePath );
        _inputQuery.constructNetworkLevelReasoner();
    }
    else
    {
        /*
          Step 1: extract the network
        */
        String networkFilePath = Options::get()->getString( Options::INPUT_FILE_PATH );
        if ( !File::exists( networkFilePath ) )
        {
            printf( "Error: the specified network file (%s) doesn't exist!\n", networkFilePath.ascii() );
            throw MarabouError( MarabouError::FILE_DOESNT_EXIST, networkFilePath.ascii() );
        }
        printf( "Network: %s\n", networkFilePath.ascii() );

        // For now, assume the network is given in ACAS format
        _acasParser = new AcasParser( networkFilePath );
        _acasParser->generateQuery( _inputQuery );
        _inputQuery.constructNetworkLevelReasoner();

        /*
          Step 2: extract the property in question
        */
        String propertyFilePath = Options::get()->getString( Options::PROPERTY_FILE_PATH );
        if ( propertyFilePath != "" )
        {
            printf( "Property: %s\n", propertyFilePath.ascii() );
            PropertyParser().parse( propertyFilePath, _inputQuery );
        }
        else
            printf( "Property: None\n" );

        printf( "\n" );
    }

    extractSplittingThreshold();
    unsigned numberOfWorkers = Options::get()->getInt( Options::NUM_WORKERS );
    _engine.setNumberOfWorkers( numberOfWorkers );

    String queryDumpFilePath = Options::get()->getString( Options::QUERY_DUMP_FILE );
    if ( queryDumpFilePath.length() > 0 )
    {
        _inputQuery.saveQuery( queryDumpFilePath );
        printf( "\nInput query successfully dumped to file\n" );
        exit( 0 );
    }
}

void Marabou::solveQuery()
{
    if ( _engine.processInputQuery( _inputQuery ) )
        _engine.solve( Options::get()->getInt( Options::TIMEOUT ) );

    if ( _engine.getExitCode() == Engine::SAT )
        _engine.extractSolution( _inputQuery );
}

void Marabou::displayResults( unsigned long long microSecondsElapsed ) const
{
    Engine::ExitCode result = _engine.getExitCode();
    String resultString;

    if ( result == Engine::UNSAT )
    {
        resultString = "unsat";
        printf( "unsat\n" );
    }
    else if ( result == Engine::SAT )
    {
        resultString = "sat";
        printf( "sat\n" );

        printf( "Input assignment:\n" );
        for ( unsigned i = 0; i < _inputQuery.getNumInputVariables(); ++i )
            printf( "\tx%u = %lf\n", i, _inputQuery.getSolutionValue( _inputQuery.inputVariableByIndex( i ) ) );

        printf( "\n" );
        printf( "Output:\n" );
        for ( unsigned i = 0; i < _inputQuery.getNumOutputVariables(); ++i )
            printf( "\ty%u = %lf\n", i, _inputQuery.getSolutionValue( _inputQuery.outputVariableByIndex( i ) ) );
        printf( "\n" );
    }
    else if ( result == Engine::TIMEOUT )
    {
        resultString = "TIMEOUT";
        printf( "Timeout\n" );
    }
    else if ( result == Engine::ERROR )
    {
        resultString = "ERROR";
        printf( "Error\n" );
    }
    else
    {
        resultString = "UNKNOWN";
        printf( "UNKNOWN EXIT CODE! (this should not happen)" );
    }

    // Create a summary file, if requested
    String summaryFilePath = Options::get()->getString( Options::SUMMARY_FILE );
    if ( summaryFilePath != "" )
    {
        File summaryFile( summaryFilePath );
        summaryFile.open( File::MODE_WRITE_TRUNCATE );

        // Field #1: result
        summaryFile.write( resultString );

        // Field #2: total elapsed time
        summaryFile.write( Stringf( " %u ", microSecondsElapsed / 1000000 ) ); // In seconds

        // Field #3: number of visited tree states
        summaryFile.write( Stringf( "%u ",
                                    _engine.getStatistics()->getNumVisitedTreeStates() ) );

        // Field #4: average pivot time in micro seconds
        summaryFile.write( Stringf( "%u",
                                    _engine.getStatistics()->getAveragePivotTimeInMicro() ) );

        summaryFile.write( "\n" );
    }
}

void Marabou::extractSplittingThreshold()
{
    int splitThreshold = Options::get()->getInt( Options::SPLIT_THRESHOLD );
    if ( splitThreshold < 0 )
    {
        printf( "Invalid constraint violation threshold value %d,"
                " using default value %u.\n\n", splitThreshold,
                GlobalConfiguration::CONSTRAINT_VIOLATION_THRESHOLD );
        splitThreshold = GlobalConfiguration::CONSTRAINT_VIOLATION_THRESHOLD;
    }
    _engine.setConstraintViolationThreshold( splitThreshold );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
