#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <map>
#include <vector>
#include <string>
#include <utility>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include "Engine.h"
#include "InputQuery.h"
#include "ReluplexError.h"
#include "FloatUtils.h"
#include "PiecewiseLinearConstraint.h"
#include "ReluConstraint.h"

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

std::pair<std::map<int, double>, Statistics> solve(InputQuery inputQuery, std::string redirect=""){
    // Arguments: InputQuery object, filename to redirect output
    // Returns: map from variable number to value
    std::map<int, double> ret;
    Statistics retStats;
    int output=-1;
    if(redirect.length()>0)
        output=redirectOutputToFile(redirect);
    try{
        Engine engine;
        if(!engine.processInputQuery(inputQuery)) return std::make_pair(ret, *(engine.getStatistics()));
        
        if(!engine.solve()) return std::make_pair(ret, *(engine.getStatistics()));
        
        engine.extractSolution(inputQuery);
        retStats = *(engine.getStatistics());
        for(unsigned int i=0; i<inputQuery.getNumberOfVariables(); i++)
            ret[i] = inputQuery.getSolutionValue(i);
    }
    catch(const ReluplexError &e){
        printf( "Caught a ReluplexError. Code: %u. Message: %s\n", e.getCode(), e.getUserMessage() );
        return std::make_pair(ret, retStats);
    }
    if(output != -1)
        restoreOutputStream(output);
    return std::make_pair(ret, retStats);
}

// Code necessary to generate Python library
// Describes which classes and functions are exposed to API
PYBIND11_MODULE(MarabouCore, m) {
    m.doc() = "Marabou API Library";
    m.def("solve", &solve, "Takes in a description of the InputQuery and returns the solution");
    m.def("addReluConstraint", &addReluConstraint, "Add a Relu constraint to the InputQuery");
    py::class_<InputQuery>(m, "InputQuery")
        .def(py::init())
        .def("setUpperBound", &InputQuery::setUpperBound)
        .def("setLowerBound", &InputQuery::setLowerBound)
        .def("getUpperBound", &InputQuery::getUpperBound)
        .def("getLowerBound", &InputQuery::getLowerBound)
        .def("setNumberOfVariables", &InputQuery::setNumberOfVariables)
        .def("addEquation", &InputQuery::addEquation);
    py::class_<Equation>(m, "Equation")
        .def(py::init())
        .def("addAddend", &Equation::addAddend)
        .def("setScalar", &Equation::setScalar)
        .def("markAuxiliaryVariable", &Equation::markAuxiliaryVariable);
    py::class_<Statistics>(m, "Statistics")
        .def("getMaxStackDepth", &Statistics::getMaxStackDepth)
        .def("getNumPops", &Statistics::getNumPops)
        .def("getNumVisitedTreeStates", &Statistics::getNumVisitedTreeStates)
        .def("getNumSplits", &Statistics::getNumSplits)
        .def("getTotalTime", &Statistics::getTotalTime);


}