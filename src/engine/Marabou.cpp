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
#include "File.h"
#include "FixedReluParser.h"
#include "MStringf.h"
#include "LookAheadPreprocessor.h"
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
        _inputQuery = QueryLoader::loadQuery(inputQueryFilePath);
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
}

void Marabou::solveQuery()
{
    if ( _engine.processInputQuery( _inputQuery ) &&
         lookAheadPreprocessing() && !Options::get()->
         getBool( Options::PREPROCESS_ONLY ) )
    {
        String fixedReluFilePath = Options::get()->getString( Options::FIXED_RELU_PATH );
        if ( fixedReluFilePath != "" )
        {
            Map<unsigned, unsigned> idToPhase;
            printf( "Fixed Relus: %s\n", fixedReluFilePath.ascii() );
            FixedReluParser().parse( fixedReluFilePath, idToPhase );
            _engine.applySplits( idToPhase );
        }

        BiasStrategy biasStrategy = setBiasStrategyFromOptions
            ( Options::get()->getString( Options::BIAS_STRATEGY ) );
        _engine.setBiasedPhases( Options::get()->getInt( Options::FOCUS_LAYER ),
                                 biasStrategy );
        _engine.solve( Options::get()->getInt( Options::TIMEOUT ) );
    }
    if ( _engine.getExitCode() == Engine::SAT )
        _engine.extractSolution( _inputQuery );
}

bool Marabou::lookAheadPreprocessing()
{
    bool feasible = true;
    if ( Options::get()->getBool( Options::LOOK_AHEAD_PREPROCESSING ) )
    {
        Map<unsigned, unsigned> idToPhase;

        struct timespec start = TimeUtils::sampleMicro();
	unsigned maxDepth = Options::get()->getInt( Options::MAX_TREE_DEPTH );
        auto lookAheadPreprocessor = new LookAheadPreprocessor
            ( Options::get()->getInt( Options::NUM_WORKERS ),
              *_engine.getInputQuery(), maxDepth );

 	List<unsigned> maxTimes;
        feasible = lookAheadPreprocessor->run( idToPhase, maxTimes );
        struct timespec end = TimeUtils::sampleMicro();
        unsigned long long totalElapsed = TimeUtils::timePassed( start, end );
        String summaryFilePath = Options::get()->getString( Options::SUMMARY_FILE );
        if ( summaryFilePath != "" )
        {
            File summaryFile( summaryFilePath + ".preprocess" );
            summaryFile.open( File::MODE_WRITE_TRUNCATE );

            // Field #1: result
            summaryFile.write( ( feasible ? "UNKNOWN" : "UNSAT" ) );

            // Field #2: total elapsed time
            summaryFile.write( Stringf( " %u ", totalElapsed / 1000000 ) ); // In seconds

            // Field #3: number of fixed relus by look ahead preprocessing
            summaryFile.write( Stringf( "%u ", idToPhase.size() ) );

	    for ( const auto& maxTime : maxTimes )
		summaryFile.write( Stringf( "%u ", maxTime ) );
            summaryFile.write( "\n" );
        }
        if ( summaryFilePath != "" )
        {
            File fixedFile( summaryFilePath + ".fixed" );
            fixedFile.open( File::MODE_WRITE_TRUNCATE );

            for ( const auto entry : idToPhase )
            {
                fixedFile.write( Stringf( "%u %u\n", entry.first, entry.second ) );
            }
        }

        if ( feasible )
            _engine.applySplits( idToPhase );
        else
            // Solved by preprocessing, we are done!
            _engine._exitCode = Engine::UNSAT;
    }
    return feasible;
}

void Marabou::displayResults( unsigned long long microSecondsElapsed ) const
{
    Engine::ExitCode result = _engine.getExitCode();
    String resultString;

    if ( result == Engine::UNSAT )
    {
        resultString = "UNSAT";
        printf( "UNSAT\n" );
    }
    else if ( result == Engine::SAT )
    {
        resultString = "SAT";
        printf( "SAT\n" );

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

        summaryFile.write( "\n" );
    }
}

BiasStrategy Marabou::setBiasStrategyFromOptions( const String strategy )
{
    if ( strategy == "centroid" )
        return BiasStrategy::Centroid;
    else if ( strategy == "sampling" )
        return BiasStrategy::Sampling;
    else if ( strategy == "random" )
        return BiasStrategy::Random;
    else
        {
            printf ("Unknown bias strategy, using default (centroid).\n");
            return BiasStrategy::Centroid;
        }
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
