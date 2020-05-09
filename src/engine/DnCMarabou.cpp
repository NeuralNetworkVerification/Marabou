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

#include "AcasParser.h"
#include "DnCManager.h"
#include "DnCMarabou.h"
#include "File.h"
#include "FixedReluParser.h"
#include "LookAheadPreprocessor.h"
#include "MStringf.h"
#include "Options.h"
#include "PropertyParser.h"
#include "MarabouError.h"
#include "QueryLoader.h"
#include "AcasParser.h"

DnCMarabou::DnCMarabou()
    : _dncManager( nullptr )
    , _inputQuery( InputQuery() )
{
    unsigned verbosity = Options::get()->getInt( Options::VERBOSITY );
    _baseEngine = std::make_shared<Engine>( verbosity );
}

void DnCMarabou::run()
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
    }
    else
    {
        /*
          Step 1: extract the network
        */
        String networkFilePath = Options::get()->getString( Options::INPUT_FILE_PATH );
        if ( !File::exists( networkFilePath ) )
        {
            printf( "Error: the specified network file (%s) doesn't exist!\n",
                    networkFilePath.ascii() );
            throw MarabouError( MarabouError::FILE_DOESNT_EXIST,
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
                throw MarabouError( MarabouError::FILE_DOESNT_EXIST,
                                    propertyFilePath.ascii() );
            }
            printf( "Property: %s\n", propertyFilePath.ascii() );
        }
        else
            printf( "Property: None\n" );

        AcasParser acasParser( networkFilePath );
        acasParser.generateQuery( _inputQuery );
        if ( propertyFilePath != "" )
            PropertyParser().parse( propertyFilePath, _inputQuery );
    }
    printf( "\n" );

    /*
      Step 3: initialize the DNC core
    */
    unsigned initialDivides = Options::get()->getInt( Options::NUM_INITIAL_DIVIDES );

    int initialTimeoutInt = Options::get()->getInt( Options::INITIAL_TIMEOUT );
    unsigned initialTimeout = 0;
    if ( initialTimeoutInt < 0 )
        initialTimeout = _inputQuery.getPiecewiseLinearConstraints().size() / 10;
    else
        initialTimeout = static_cast<unsigned>(initialTimeoutInt);

    unsigned numWorkers = Options::get()->getInt( Options::NUM_WORKERS );
    unsigned onlineDivides = Options::get()->getInt( Options::NUM_ONLINE_DIVIDES );
    unsigned verbosity = Options::get()->getInt( Options::VERBOSITY );
    unsigned timeoutInSeconds = Options::get()->getInt( Options::TIMEOUT );
    float timeoutFactor = Options::get()->getFloat( Options::TIMEOUT_FACTOR );

    DivideStrategy divideStrategy = DivideStrategy::SplitRelu;
    if ( Options::get()->getString( Options::DIVIDE_STRATEGY ) == "auto" )
    {
        if ( _inputQuery.getInputVariables().size() < 10  )
            divideStrategy = DivideStrategy::LargestInterval;
        else
            divideStrategy = DivideStrategy::SplitRelu;
    }
    else
        divideStrategy = setDivideStrategyFromOptions
            ( Options::get()->getString( Options::DIVIDE_STRATEGY ) );


    bool restoreTreeStates = Options::get()->getBool( Options::RESTORE_TREE_STATES );
    unsigned biasedLayer = Options::get()->getInt( Options::FOCUS_LAYER );
    unsigned maxDepth = Options::get()->getInt( Options::MAX_DEPTH );
    BiasStrategy biasStrategy = setBiasStrategyFromOptions
        ( Options::get()->getString( Options::BIAS_STRATEGY ) );

    int splitThreshold = Options::get()->getInt( Options::SPLIT_THRESHOLD );
    if ( splitThreshold < 0 )
    {
	printf( "Invalid constraint violation threshold value %d,"
		" using default value %u.\n\n", splitThreshold,
		GlobalConfiguration::CONSTRAINT_VIOLATION_THRESHOLD );
	splitThreshold = GlobalConfiguration::CONSTRAINT_VIOLATION_THRESHOLD;
    }

    std::cout << "Initial Divides: " << initialDivides << std::endl;
    std::cout << "Initial Timeout: " << initialTimeout << std::endl;
    std::cout << "Number of Workers: " << numWorkers << std::endl;
    std::cout << "Online Divides: " << onlineDivides  << std::endl;
    std::cout << "Verbosity: " << verbosity << std::endl;
    std::cout << "Timeout: " << timeoutInSeconds  << std::endl;
    std::cout << "Timeout Factor: "  << timeoutFactor << std::endl;
    std::cout << "Divide Strategy: " << ( divideStrategy ==
                                          DivideStrategy::LargestInterval ?
                                          "Largest Interval" : "Split Relu" )
              << std::endl;
    std::cout << "Max Depth: " << maxDepth << std::endl;
    std::cout << "Perform tree state restoration: " << ( restoreTreeStates ? "Yes" : "No" )
              << std::endl;
    std::cout << "Focus layer: " << biasedLayer << std::endl;
    std::cout << "Bias Strategy: " << ( biasStrategy == BiasStrategy::Estimate ?
                                          "estimate" : "no estimate" )
              << std::endl;
    std::cout << "Split threshold: " << splitThreshold << std::endl;

    struct timespec start = TimeUtils::sampleMicro();

    Map<unsigned, unsigned> idToPhase;
    if ( _baseEngine->processInputQuery( _inputQuery ) &&
         lookAheadPreprocessing( idToPhase, splitThreshold ) &&
         !Options::get()->getBool( Options::PREPROCESS_ONLY ) )
    {
        if ( !Options::get()->getBool( Options::PREPROCESS_ONLY ) )
        {
            String fixedReluFilePath = Options::get()->getString( Options::FIXED_RELU_PATH );
            if ( fixedReluFilePath != "" )
            {
                printf( "Fixed Relus: %s\n", fixedReluFilePath.ascii() );
                FixedReluParser().parse( fixedReluFilePath, idToPhase );
            }

            _dncManager = std::unique_ptr<DnCManager>
                ( new DnCManager( numWorkers, initialDivides, initialTimeout,
                                  onlineDivides, timeoutFactor, divideStrategy,
                                  _baseEngine->getInputQuery(), verbosity,
                                  idToPhase, biasedLayer, biasStrategy,
                                  maxDepth ) );
	    _dncManager->setConstraintViolationThreshold( splitThreshold );
            _dncManager->solve( timeoutInSeconds, restoreTreeStates );
        }
    }
    else
    {
        _dncManager->_exitCode = DnCManager::UNSAT;
    }
    struct timespec end = TimeUtils::sampleMicro();

    unsigned long long totalElapsed = TimeUtils::timePassed( start, end );
    displayResults( totalElapsed );
}

bool DnCMarabou::lookAheadPreprocessing( Map<unsigned, unsigned> &idToPhase, unsigned splitThreshold )
{
    bool feasible = true;
    if ( Options::get()->getBool( Options::LOOK_AHEAD_PREPROCESSING ) )
    {
        struct timespec start = TimeUtils::sampleMicro();

        auto lookAheadPreprocessor = new LookAheadPreprocessor
            ( Options::get()->getInt( Options::NUM_WORKERS ),
              *(_baseEngine->getInputQuery()), splitThreshold );
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
                fixedFile.write( Stringf( "%u %u\n", entry.first, entry.second ) );
        }
    }
    return feasible;
}

void DnCMarabou::displayResults( unsigned long long microSecondsElapsed ) const
{
    std::cout << "Total Time: " << microSecondsElapsed / 1000000 << std::endl;
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

BiasStrategy DnCMarabou::setBiasStrategyFromOptions( const String strategy )
{
    if ( strategy == "centroid" )
        return BiasStrategy::Centroid;
    else if ( strategy == "sampling" )
        return BiasStrategy::Sampling;
    else if ( strategy == "random" )
        return BiasStrategy::Random;
    else if ( strategy == "estimate" )
        return BiasStrategy::Estimate;
    else
    {
        printf ("Unknown bias strategy, using default (centroid).\n");
        return BiasStrategy::Estimate;
    }
}

DivideStrategy DnCMarabou::setDivideStrategyFromOptions( const String strategy )
{
    if ( strategy == "split-relu" )
        return DivideStrategy::SplitRelu;
    else if ( strategy == "largest-interval" )
        return DivideStrategy::LargestInterval;
    else
    {
        printf ("Unknown divide strategy, using default (SplitRelu).\n");
        return DivideStrategy::SplitRelu;
    }
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
