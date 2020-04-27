/*********************                                                        */
/*! \file MarabouCore.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Christopher Lazarus, Andrew Wu, Shantanu Thakoor
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

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <map>
#include <vector>
#include <set>
#include <string>
#include <utility>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include "AcasParser.h"
#include "DnCManager.h"
#include "Engine.h"
#include "FloatUtils.h"
#include "InputQuery.h"
#include "MarabouError.h"
#include "MString.h"
#include "MaxConstraint.h"
#include "PiecewiseLinearConstraint.h"
#include "PropertyParser.h"
#include "QueryLoader.h"
#include "ReluConstraint.h"
#include "Set.h"

#ifdef _WIN32
#define STDOUT_FILENO 1
#else
#include <unistd.h>
#endif

namespace py = pybind11;

int redirectOutputToFile(std::string outputFilePath){
    // Redirect standard output to a file
    int outputFile = open(outputFilePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if ( outputFile < 0 )
    {
        printf( "Error redirecting output to file\n");
        exit( 1 );
    }

    int outputStream = dup( STDOUT_FILENO );
    if (outputStream < 0)
    {
        printf( "Error duplicating standard output\n" );
        exit(1);
    }

    if ( dup2( outputFile, STDOUT_FILENO ) < 0 )
    {
        printf("Error duplicating to standard output\n");
        exit(1);
    }

    close( outputFile );
    return outputStream;
}

void restoreOutputStream(int outputStream)
{
    // Restore standard output
    fflush( stdout );
    if (dup2( outputStream, STDOUT_FILENO ) < 0){
        printf( "Error restoring output stream\n" );
        exit( 1 );
    }
    close(outputStream);
}

void addReluConstraint(InputQuery& ipq, unsigned var1, unsigned var2){
    PiecewiseLinearConstraint* r = new ReluConstraint(var1, var2);
    ipq.addPiecewiseLinearConstraint(r);
}

void addMaxConstraint(InputQuery& ipq, std::set<unsigned> elements, unsigned v){
    Set<unsigned> e;
    for(unsigned var: elements)
        e.insert(var);
    PiecewiseLinearConstraint* m = new MaxConstraint(v, e);
    ipq.addPiecewiseLinearConstraint(m);
}

void createInputQuery(InputQuery &inputQuery, std::string networkFilePath, std::string propertyFilePath){
  AcasParser* acasParser = new AcasParser( String(networkFilePath) );
  acasParser->generateQuery( inputQuery );
  String propertyFilePathM = String(propertyFilePath);
  if ( propertyFilePath != "" )
    {
      printf( "Property: %s\n", propertyFilePathM.ascii() );
      PropertyParser().parse( propertyFilePathM, inputQuery );
    }
  else
    printf( "Property: None\n" );
}

struct MarabouOptions {
    MarabouOptions()
        : _numWorkers( 4 )
        , _initialTimeout( 5 )
        , _initialDivides( 0 )
        , _onlineDivides( 2 )
        , _timeoutInSeconds( 0 )
        , _timeoutFactor( 1.5 )
        , _verbosity( 2 )
        , _dnc( false )
    {};

    unsigned _numWorkers;
    unsigned _initialTimeout;
    unsigned _initialDivides;
    unsigned _onlineDivides;
    unsigned _timeoutInSeconds;
    float _timeoutFactor;
    unsigned _verbosity;
    bool _dnc;
};

/* The default parameters here are just for readability, you should specify
 * them in the to make them work*/
std::pair<std::map<int, double>, Statistics> solve(InputQuery &inputQuery, MarabouOptions &options,
                                                   std::string redirect=""){
    // Arguments: InputQuery object, filename to redirect output
    // Returns: map from variable number to value
    std::map<int, double> ret;
    Statistics retStats;
    int output=-1;
    if(redirect.length()>0)
        output=redirectOutputToFile(redirect);
    try{
        bool verbosity = options._verbosity;
        unsigned timeoutInSeconds = options._timeoutInSeconds;
        bool dnc = options._dnc;

        Engine engine;
        engine.setVerbosity(verbosity);

        if(!engine.processInputQuery(inputQuery)) return std::make_pair(ret, *(engine.getStatistics()));
        if ( dnc )
        {
            unsigned initialDivides = options._initialDivides;
            unsigned initialTimeout = options._initialTimeout;
            unsigned numWorkers = options._numWorkers;
            unsigned onlineDivides = options._onlineDivides;
            float timeoutFactor = options._timeoutFactor;

            auto dncManager = std::unique_ptr<DnCManager>
                ( new DnCManager( numWorkers, initialDivides, initialTimeout, onlineDivides,
                                  timeoutFactor, DivideStrategy::LargestInterval,
                                  &inputQuery, verbosity ) );

            dncManager->solve( timeoutInSeconds );
            switch ( dncManager->getExitCode() )
            {
            case DnCManager::SAT:
            {
                retStats = Statistics();
                dncManager->getSolution( ret );
                break;
            }
            case DnCManager::TIMEOUT:
            {
                retStats = Statistics();
                retStats.timeout();
                return std::make_pair( ret, Statistics() );
            }
            default:
                return std::make_pair( ret, Statistics() ); // TODO: meaningful DnCStatistics
            }
        } else
        {
            if(!engine.solve(timeoutInSeconds)) return std::make_pair(ret, *(engine.getStatistics()));

            if (engine.getExitCode() == Engine::SAT)
                engine.extractSolution(inputQuery);
            retStats = *(engine.getStatistics());
            for(unsigned int i=0; i<inputQuery.getNumberOfVariables(); ++i)
                ret[i] = inputQuery.getSolutionValue(i);
        }
    }
    catch(const MarabouError &e){
        printf( "Caught a MarabouError. Code: %u. Message: %s\n", e.getCode(), e.getUserMessage() );
        return std::make_pair(ret, retStats);
    }
    if(output != -1)
        restoreOutputStream(output);
    return std::make_pair(ret, retStats);
}

void saveQuery(InputQuery& inputQuery, std::string filename){
    inputQuery.saveQuery(String(filename));
}

InputQuery loadQuery(std::string filename){
    return QueryLoader::loadQuery(String(filename));
}

// Code necessary to generate Python library
// Describes which classes and functions are exposed to API
PYBIND11_MODULE(MarabouCore, m) {
    m.doc() = "Marabou API Library";
    m.def("createInputQuery", &createInputQuery, "Create input query from network and property file");
    m.def("solve", &solve, "Takes in a description of the InputQuery and returns the solution", py::arg("inputQuery"), py::arg("options"), py::arg("redirect") = "");
    m.def("saveQuery", &saveQuery, "Serializes the inputQuery in the given filename");
    m.def("loadQuery", &loadQuery, "Loads and returns a serialized inputQuery from the given filename");
    m.def("addReluConstraint", &addReluConstraint, "Add a Relu constraint to the InputQuery");
    m.def("addMaxConstraint", &addMaxConstraint, "Add a Max constraint to the InputQuery");
    py::class_<InputQuery>(m, "InputQuery")
        .def(py::init())
        .def("setUpperBound", &InputQuery::setUpperBound)
        .def("setLowerBound", &InputQuery::setLowerBound)
        .def("getUpperBound", &InputQuery::getUpperBound)
        .def("getLowerBound", &InputQuery::getLowerBound)
        .def("dump", &InputQuery::dump)
        .def("setNumberOfVariables", &InputQuery::setNumberOfVariables)
        .def("addEquation", &InputQuery::addEquation)
        .def("getSolutionValue", &InputQuery::getSolutionValue)
        .def("getNumberOfVariables", &InputQuery::getNumberOfVariables)
        .def("getNumInputVariables", &InputQuery::getNumInputVariables)
        .def("getNumOutputVariables", &InputQuery::getNumOutputVariables)
        .def("inputVariableByIndex", &InputQuery::inputVariableByIndex)
        .def("markInputVariable", &InputQuery::markInputVariable)
        .def("markOutputVariable", &InputQuery::markOutputVariable)
        .def("outputVariableByIndex", &InputQuery::outputVariableByIndex)
        .def("setNetworkLevelReasoner", &InputQuery::setNetworkLevelReasoner);
    py::class_<MarabouOptions>(m, "Options")
        .def(py::init())
        .def_readwrite("_numWorkers", &MarabouOptions::_numWorkers)
        .def_readwrite("_initialTimeout", &MarabouOptions::_initialTimeout)
        .def_readwrite("_initialDivides", &MarabouOptions::_initialDivides)
        .def_readwrite("_onlineDivides", &MarabouOptions::_onlineDivides)
        .def_readwrite("_timeoutInSeconds", &MarabouOptions::_timeoutInSeconds)
        .def_readwrite("_timeoutFactor", &MarabouOptions::_timeoutFactor)
        .def_readwrite("_verbosity", &MarabouOptions::_verbosity)
        .def_readwrite("_dnc", &MarabouOptions::_dnc);
    py::class_<NetworkLevelReasoner, std::unique_ptr<NetworkLevelReasoner,py::nodelete>>(m, "NetworkLevelReasoner")
        .def(py::init())
        .def("setNumberOfLayers", &NetworkLevelReasoner::setNumberOfLayers)
        .def("setLayerSize", &NetworkLevelReasoner::setLayerSize)
        .def("setNeuronActivationFunction", &NetworkLevelReasoner::setNeuronActivationFunction)
        .def("setBias", &NetworkLevelReasoner::setBias)
        .def("setWeight", &NetworkLevelReasoner::setWeight)
        .def("allocateMemoryByTopology", &NetworkLevelReasoner::allocateMemoryByTopology)
        .def("setWeightedSumVariable", &NetworkLevelReasoner::setWeightedSumVariable)
        .def("setActivationResultVariable", &NetworkLevelReasoner::setActivationResultVariable);
    py::class_<Equation> eq(m, "Equation");
    eq.def(py::init());
    eq.def(py::init<Equation::EquationType>());
    eq.def("addAddend", &Equation::addAddend);
    eq.def("setScalar", &Equation::setScalar);
    py::enum_<Equation::EquationType>(eq, "EquationType")
        .value("EQ", Equation::EquationType::EQ)
        .value("GE", Equation::EquationType::GE)
        .value("LE", Equation::EquationType::LE)
        .export_values();
    py::class_<Statistics>(m, "Statistics")
        .def("getMaxStackDepth", &Statistics::getMaxStackDepth)
        .def("getNumPops", &Statistics::getNumPops)
        .def("getNumVisitedTreeStates", &Statistics::getNumVisitedTreeStates)
        .def("getNumSplits", &Statistics::getNumSplits)
        .def("getTotalTime", &Statistics::getTotalTime)
        .def("getNumTableauPivots", &Statistics::getNumTableauPivots)
        .def("getMaxDegradation", &Statistics::getMaxDegradation)
        .def("getNumPrecisionRestorations", &Statistics::getNumPrecisionRestorations)
        .def("getNumSimplexPivotSelectionsIgnoredForStability", &Statistics::getNumSimplexPivotSelectionsIgnoredForStability)
        .def("getNumSimplexUnstablePivots", &Statistics::getNumSimplexUnstablePivots)
        .def("getNumMainLoopIterations", &Statistics::getNumMainLoopIterations)
        .def("getTimeSimplexStepsMicro", &Statistics::getTimeSimplexStepsMicro)
        .def("getNumConstraintFixingSteps", &Statistics::getNumConstraintFixingSteps)
        .def("hasTimedOut", &Statistics::hasTimedOut);
}
