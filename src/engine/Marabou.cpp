/*********************                                                        */
/*! \file Marabou.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Andrew Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief [[ Add one-line brief description here ]]
 **
 ** [[ Add lengthier description here ]]
 **/

#include "Marabou.h"

#include "AcasParser.h"
#include "AutoFile.h"
#include "File.h"
#include "GlobalConfiguration.h"
#include "MStringf.h"
#include "MarabouError.h"
#include "OnnxParser.h"
#include "Options.h"
#include "PropertyParser.h"
#include "QueryLoader.h"
#include "VnnLibParser.h"

#ifdef _WIN32
#undef ERROR
#endif

Marabou::Marabou()
    : _acasParser( NULL )
    , _onnxParser( NULL )
    , _cegarSolver( NULL )
    , _engine( std::unique_ptr<Engine>( new Engine() ) )
{
}

Marabou::~Marabou()
{
    if ( _acasParser )
    {
        delete _acasParser;
        _acasParser = NULL;
    }

    if ( _onnxParser )
    {
        delete _onnxParser;
        _onnxParser = NULL;
    }
}

void Marabou::run()
{
    struct timespec start = TimeUtils::sampleMicro();

    prepareQuery();
    solveQuery();

    struct timespec end = TimeUtils::sampleMicro();

    unsigned long long totalElapsed = TimeUtils::timePassed( start, end );
    displayResults( totalElapsed );

    if ( Options::get()->getBool( Options::EXPORT_ASSIGNMENT ) )
        exportAssignment();
}

void Marabou::prepareQuery()
{
    String inputQueryFilePath = Options::get()->getString( Options::INPUT_QUERY_FILE_PATH );
    if ( inputQueryFilePath.length() > 0 )
    {
        /*
          Step 1: extract the query
        */
        if ( !File::exists( inputQueryFilePath ) )
        {
            printf( "Error: the specified inputQuery file (%s) doesn't exist!\n",
                    inputQueryFilePath.ascii() );
            throw MarabouError( MarabouError::FILE_DOESNT_EXIST, inputQueryFilePath.ascii() );
        }

        printf( "Query: %s\n", inputQueryFilePath.ascii() );
        QueryLoader::loadQuery( inputQueryFilePath, _inputQuery );
    }
    else
    {
        /*
          Step 1: extract the network
        */
        String networkFilePath = Options::get()->getString( Options::INPUT_FILE_PATH );

        if ( networkFilePath.length() == 0 )
        {
            printf( "Error: no network file provided!\n" );
            throw MarabouError( MarabouError::FILE_DOESNT_EXIST, networkFilePath.ascii() );
        }

        if ( !File::exists( networkFilePath ) )
        {
            printf( "Error: the specified network file (%s) doesn't exist!\n",
                    networkFilePath.ascii() );
            throw MarabouError( MarabouError::FILE_DOESNT_EXIST, networkFilePath.ascii() );
        }
        printf( "Network: %s\n", networkFilePath.ascii() );

        if ( ( (String)networkFilePath ).endsWith( ".onnx" ) )
        {
            InputQueryBuilder queryBuilder;
            OnnxParser::parse( queryBuilder, networkFilePath, {}, {} );
            queryBuilder.generateQuery( _inputQuery );
        }
        else
        {
            _acasParser = new AcasParser( networkFilePath );
            _acasParser->generateQuery( _inputQuery );
        }

        /*
          Step 2: extract the property in question
        */
        String propertyFilePath = Options::get()->getString( Options::PROPERTY_FILE_PATH );
        if ( propertyFilePath != "" )
        {
            printf( "Property: %s\n", propertyFilePath.ascii() );
            if ( propertyFilePath.endsWith( ".vnnlib" ) )
            {
                VnnLibParser().parse( propertyFilePath, _inputQuery );
            }
            else
            {
                PropertyParser().parse( propertyFilePath, _inputQuery );
            }
        }
        else
            printf( "Property: None\n" );

        printf( "\n" );
    }

    if ( Options::get()->getBool( Options::DEBUG_ASSIGNMENT ) )
        importDebuggingSolution();

    String queryDumpFilePath = Options::get()->getString( Options::QUERY_DUMP_FILE );
    if ( queryDumpFilePath.length() > 0 )
    {
        _inputQuery.saveQuery( queryDumpFilePath );
        printf( "\nInput query successfully dumped to file\n" );
        exit( 0 );
    }
}

void Marabou::importDebuggingSolution()
{
    String fileName = Options::get()->getString( Options::IMPORT_ASSIGNMENT_FILE_PATH );
    AutoFile input( fileName );

    if ( !IFile::exists( fileName ) )
    {
        throw MarabouError( MarabouError::FILE_DOES_NOT_EXIST,
                            Stringf( "File %s not found.\n", fileName.ascii() ).ascii() );
    }

    input->open( IFile::MODE_READ );

    unsigned numVars = atoi( input->readLine().trim().ascii() );
    ASSERT( numVars == _inputQuery.getNumberOfVariables() );

    unsigned var;
    double value;
    String line;

    // Import each assignment
    for ( unsigned i = 0; i < numVars; ++i )
    {
        line = input->readLine();
        List<String> tokens = line.tokenize( "," );
        auto it = tokens.begin();
        var = atoi( it->ascii() );
        ASSERT( var == i );
        it++;
        value = atof( it->ascii() );
        it++;
        ASSERT( it == tokens.end() );
        _inputQuery.storeDebuggingSolution( var, value );
    }

    input->close();
}

void Marabou::exportAssignment() const
{
    String assignmentFileName = "assignment.txt";
    AutoFile exportFile( assignmentFileName );
    exportFile->open( IFile::MODE_WRITE_TRUNCATE );

    unsigned numberOfVariables = _inputQuery.getNumberOfVariables();
    // Number of Variables
    exportFile->write( Stringf( "%u\n", numberOfVariables ) );

    // Export each assignment
    for ( unsigned var = 0; var < numberOfVariables; ++var )
        exportFile->write( Stringf( "%u, %f\n", var, _inputQuery.getSolutionValue( var ) ) );

    exportFile->close();
}

void Marabou::solveQuery()
{
    enum {
        MICROSECONDS_IN_SECOND = 1000000
    };

    struct timespec start = TimeUtils::sampleMicro();
    unsigned timeoutInSeconds = Options::get()->getInt( Options::TIMEOUT );
    if ( _engine->processInputQuery( _inputQuery ) )
    {
        _engine->solve( timeoutInSeconds );
        if ( _engine->shouldProduceProofs() && _engine->getExitCode() == Engine::UNSAT )
            _engine->certifyUNSATCertificate();
    }

    if ( _engine->getExitCode() == Engine::UNKNOWN )
    {
        struct timespec end = TimeUtils::sampleMicro();
        unsigned long long totalElapsed = TimeUtils::timePassed( start, end );
        if ( timeoutInSeconds == 0 || totalElapsed < timeoutInSeconds * MICROSECONDS_IN_SECOND )
        {
            _cegarSolver = new CEGAR::IncrementalLinearization( _inputQuery, _engine.release() );
            unsigned long long timeoutInMicroSeconds =
                ( timeoutInSeconds == 0
                      ? 0
                      : timeoutInSeconds * MICROSECONDS_IN_SECOND - totalElapsed );
            _cegarSolver->setInitialTimeoutInMicroSeconds( timeoutInMicroSeconds );
            _cegarSolver->solve();
            _engine = std::unique_ptr<Engine>( _cegarSolver->releaseEngine() );
        }
    }

    // TODO: update the variable assignment using NLR if possible and double-check that all the
    // constraints are indeed satisfied.
    if ( _engine->getExitCode() == Engine::SAT )
        _engine->extractSolution( _inputQuery );
}

void Marabou::displayResults( unsigned long long microSecondsElapsed ) const
{
    Engine::ExitCode result = _engine->getExitCode();
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
            printf( "\tx%u = %lf\n",
                    i,
                    _inputQuery.getSolutionValue( _inputQuery.inputVariableByIndex( i ) ) );

        printf( "\n" );
        printf( "Output:\n" );
        for ( unsigned i = 0; i < _inputQuery.getNumOutputVariables(); ++i )
            printf( "\ty%u = %lf\n",
                    i,
                    _inputQuery.getSolutionValue( _inputQuery.outputVariableByIndex( i ) ) );
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
    else if ( result == Engine::UNKNOWN )
    {
        resultString = "UNKNOWN";
        printf( "UNKNOWN\n" );
    }
    else
    {
        resultString = "NOT_DONE";
        printf( "Unexpected exit code! (this should not happen)" );
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
                                    _engine->getStatistics()->getUnsignedAttribute(
                                        Statistics::NUM_VISITED_TREE_STATES ) ) );

        // Field #4: average pivot time in micro seconds
        summaryFile.write(
            Stringf( "%u", _engine->getStatistics()->getAveragePivotTimeInMicro() ) );

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
