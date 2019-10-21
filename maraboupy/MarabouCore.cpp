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
#include "Engine.h"
#include "InputQuery.h"
#include "MarabouError.h"
#include "MString.h"
#include "FloatUtils.h"
#include "MaxConstraint.h"
#include "PiecewiseLinearConstraint.h"
#include "PropertyParser.h"
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

/* The default parameters here are just for readability, you should specify
 * them in the to make them work*/
std::pair<std::map<int, double>, Statistics> solve(InputQuery &inputQuery, std::string redirect="", unsigned timeout=0, unsigned verbosity = 2){
    // Arguments: InputQuery object, filename to redirect output
    // Returns: map from variable number to value
    std::map<int, double> ret;
    Statistics retStats;
    int output=-1;
    if(redirect.length()>0)
        output=redirectOutputToFile(redirect);
    try{
        Engine engine;
        engine.setVerbosity(verbosity);

        if(!engine.processInputQuery(inputQuery)) return std::make_pair(ret, *(engine.getStatistics()));

        if(!engine.solve(timeout)) return std::make_pair(ret, *(engine.getStatistics()));

        if (engine.getExitCode() == Engine::SAT)
            engine.extractSolution(inputQuery);
        retStats = *(engine.getStatistics());
        for(unsigned int i=0; i<inputQuery.getNumberOfVariables(); i++)
            ret[i] = inputQuery.getSolutionValue(i);
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

// Code necessary to generate Python library
// Describes which classes and functions are exposed to API
PYBIND11_MODULE(MarabouCore, m) {
    m.doc() = "Marabou API Library";
    m.def("createInputQuery", &createInputQuery, "Create input query from network and property file");
    m.def("solve", &solve, "Takes in a description of the InputQuery and returns the solution", py::arg("inputQuery"), py::arg("redirect") = "", py::arg("timeout") = 0, py::arg("verbosity") = 2);
    m.def("saveQuery", &saveQuery, "Serializes the inputQuery in the given filename");
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
        .def("getNumInputVariables", &InputQuery::getNumInputVariables)
        .def("getNumOutputVariables", &InputQuery::getNumOutputVariables)
        .def("inputVariableByIndex", &InputQuery::inputVariableByIndex)
        .def("outputVariableByIndex", &InputQuery::outputVariableByIndex)
        .def("setSymbolicBoundTightener", &InputQuery::setSymbolicBoundTightener);
    py::class_<SymbolicBoundTightener, std::unique_ptr<SymbolicBoundTightener,py::nodelete>>(m, "SymbolicBoundTightener")
        .def(py::init())
        .def("setNumberOfLayers", &SymbolicBoundTightener::setNumberOfLayers)
        .def("setLayerSize", &SymbolicBoundTightener::setLayerSize)
        .def("allocateWeightAndBiasSpace", &SymbolicBoundTightener::allocateWeightAndBiasSpace)
        .def("setBias", &SymbolicBoundTightener::setBias)
        .def("setWeight", &SymbolicBoundTightener::setWeight)
        .def("setInputLowerBound", &SymbolicBoundTightener::setInputLowerBound)
        .def("setInputUpperBound", &SymbolicBoundTightener::setInputUpperBound)
        .def("setReluBVariable", &SymbolicBoundTightener::setReluBVariable)
        .def("setReluFVariable", &SymbolicBoundTightener::setReluFVariable);
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
