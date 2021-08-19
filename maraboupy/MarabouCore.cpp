/*********************                                                        */
/*! \file MarabouCore.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Christopher Lazarus, Andrew Wu, Shantanu Thakoor, Kyle Julian
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
#include "CommonError.h"
#include "DnCManager.h"
#include "DisjunctionConstraint.h"
#include "Engine2.h"
#include "FloatUtils.h"
#include "InputParserError.h"
#include "InputQuery.h"
#include "List.h"
#include "Map.h"
#include "MarabouError.h"
#include "MaxConstraint.h"
#include "MString.h"
#include "Options.h"
#include "PiecewiseLinearCaseSplit.h"
#include "PiecewiseLinearConstraint.h"
#include "PropertyParser.h"
#include "QueryLoader.h"
#include "ReluConstraint.h"
#include "Set.h"
#include "SnCDivideStrategy.h"
#include "SignConstraint.h"

#ifdef _WIN32
#define STDOUT_FILENO 1
#else
#include <unistd.h>
#endif

namespace py = pybind11;

typedef Map<unsigned, bool> GammaMap;
typedef List<GammaMap> GammaList;

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

void addSignConstraint(InputQuery& ipq, unsigned var1, unsigned var2){
    PiecewiseLinearConstraint* r = new SignConstraint(var1, var2);
    ipq.addPiecewiseLinearConstraint(r);
}

void addMaxConstraint(InputQuery& ipq, std::set<unsigned> elements, unsigned v){
    Set<unsigned> e;
    for(unsigned var: elements)
        e.insert(var);
    PiecewiseLinearConstraint* m = new MaxConstraint(v, e);
    ipq.addPiecewiseLinearConstraint(m);
}

void addAbsConstraint(InputQuery& ipq, unsigned b, unsigned f){
    ipq.addPiecewiseLinearConstraint(new AbsoluteValueConstraint(b, f));
}

bool createInputQuery(InputQuery &inputQuery, std::string networkFilePath, std::string propertyFilePath){
  try{
    AcasParser* acasParser = new AcasParser( String(networkFilePath) );
    acasParser->generateQuery( inputQuery );

    bool success = inputQuery.constructNetworkLevelReasoner();
    if ( success )
      printf("Successfully created a network level reasoner.\n");
    else
      printf("Warning: network level reasoner construction failed.\n");

    String propertyFilePathM = String(propertyFilePath);
    if ( propertyFilePath != "" )
      {
        printf( "Property: %s\n", propertyFilePathM.ascii() );
        PropertyParser().parse( propertyFilePathM, inputQuery );
      }
    else
      printf( "Property: None\n" );
  }
  catch(const InputParserError &e){
        printf( "Caught an InputParserError. Code: %u. Message: %s\n", e.getCode(), e.getUserMessage() );
        return false;
  }
  return true;
}

void addDisjunctionConstraint(InputQuery& ipq, const std::list<std::list<Equation>>
                              &disjuncts ){
    List<PiecewiseLinearCaseSplit> disjunctList;
    for ( const auto &disjunct : disjuncts )
    {
        PiecewiseLinearCaseSplit split;
        for ( const auto &eq : disjunct )
        {
            if ( eq._addends.size() == 1 )
            {
                // Add bounds as tightenings
                unsigned var = eq._addends.front()._variable;
                unsigned coeff = eq._addends.front()._coefficient;
                if ( coeff == 0 )
                    throw CommonError( CommonError::DIVISION_BY_ZERO,
                                       "AddDisjunctionConstraint: zero coefficient encountered" );
                double scalar = eq._scalar / coeff;
                Equation::EquationType type = eq._type;

                if ( type == Equation::EQ )
                {
                    split.storeBoundTightening( Tightening( var, scalar, Tightening::LB ) );
                    split.storeBoundTightening( Tightening( var, scalar, Tightening::UB ) );
                }
                else if ( type == Equation::GE || coeff < 0 )
                    split.storeBoundTightening( Tightening( var, scalar, Tightening::LB ) );
                else if ( type == Equation::LE || coeff < 0 )
                    split.storeBoundTightening( Tightening( var, scalar, Tightening::UB ) );
            }
            else
            {
                split.addEquation( eq );
            }
        }
        disjunctList.append( split );
    }
    ipq.addPiecewiseLinearConstraint(new DisjunctionConstraint(disjunctList));
}

struct MarabouOptions {
    MarabouOptions()
        : _snc( Options::get()->getBool( Options::DNC_MODE ) )
        , _restoreTreeStates( Options::get()->getBool( Options::RESTORE_TREE_STATES ) )
        , _solveWithMILP( Options::get()->getBool( Options::SOLVE_WITH_MILP ) )
        , _numWorkers( Options::get()->getInt( Options::NUM_WORKERS ) )
        , _initialTimeout( Options::get()->getInt( Options::INITIAL_TIMEOUT ) )
        , _initialDivides( Options::get()->getInt( Options::NUM_INITIAL_DIVIDES ) )
        , _onlineDivides( Options::get()->getInt( Options::NUM_ONLINE_DIVIDES ) )
        , _verbosity( Options::get()->getInt( Options::VERBOSITY ) )
        , _timeoutInSeconds( Options::get()->getInt( Options::TIMEOUT ) )
        , _splitThreshold( Options::get()->getInt( Options::CONSTRAINT_VIOLATION_THRESHOLD ) )
        , _timeoutFactor( Options::get()->getFloat( Options::TIMEOUT_FACTOR ) )
        , _preprocessorBoundTolerance( Options::get()->getFloat( Options::PREPROCESSOR_BOUND_TOLERANCE ) )
        , _gamma_abstract( Options::get()->getGammaAbstract( Options::GAMMA_ABSTRACT ) )
        , _gamma( Options::get()->getGamma( Options::GAMMA ) )
        , _var_index_to_pos( Options::get()->getVarIndexToPos( Options::VAR_INDEX_TO_POS ) )
        , _var_index_to_inc( Options::get()->getVarIndexToInc( Options::VAR_INDEX_TO_INC ) )
        , _post_var_indices( Options::get()->getPostVarIndices( Options::POST_VAR_INDICES ) )
        , _splittingStrategyString( Options::get()->getString( Options::SPLITTING_STRATEGY ).ascii() )
        , _sncSplittingStrategyString( Options::get()->getString( Options::SNC_SPLITTING_STRATEGY ).ascii() )
    {};

  void setOptions()
  {
    // Bool options
    Options::get()->setBool( Options::DNC_MODE, _snc );
    Options::get()->setBool( Options::RESTORE_TREE_STATES, _restoreTreeStates );
    Options::get()->setBool( Options::SOLVE_WITH_MILP, _solveWithMILP );

    // int options
    Options::get()->setInt( Options::NUM_WORKERS, _numWorkers );
    Options::get()->setInt( Options::INITIAL_TIMEOUT, _initialTimeout );
    Options::get()->setInt( Options::NUM_INITIAL_DIVIDES, _initialDivides );
    Options::get()->setInt( Options::NUM_ONLINE_DIVIDES, _onlineDivides );
    Options::get()->setInt( Options::VERBOSITY, _verbosity );
    Options::get()->setInt( Options::TIMEOUT, _timeoutInSeconds );
    Options::get()->setInt( Options::CONSTRAINT_VIOLATION_THRESHOLD, _splitThreshold );

    // float options
    Options::get()->setFloat( Options::TIMEOUT_FACTOR, _timeoutFactor );
    Options::get()->setFloat( Options::PREPROCESSOR_BOUND_TOLERANCE, _preprocessorBoundTolerance );

    // string options
    Options::get()->setString( Options::SPLITTING_STRATEGY, _splittingStrategyString );
    Options::get()->setString( Options::SNC_SPLITTING_STRATEGY, _sncSplittingStrategyString );

    // map options
    Options::get()->setGammaAbstract( Options::GAMMA_ABSTRACT, _gamma_abstract );
    Options::get()->setVarIndexToPos( Options::VAR_INDEX_TO_POS, _var_index_to_pos );
    Options::get()->setVarIndexToInc( Options::VAR_INDEX_TO_INC, _var_index_to_inc );
    Options::get()->setPostVarIndices( Options::POST_VAR_INDICES, _post_var_indices );

    // list options
    Options::get()->setGamma( Options::GAMMA, _gamma );


  }

    bool _snc;
    bool _restoreTreeStates;
    bool _solveWithMILP;
    unsigned _numWorkers;
    unsigned _initialTimeout;
    unsigned _initialDivides;
    unsigned _onlineDivides;
    unsigned _verbosity;
    unsigned _timeoutInSeconds;
    unsigned _splitThreshold;
    float _timeoutFactor;
    float _preprocessorBoundTolerance;
    std::string _splittingStrategyString;
    std::string _sncSplittingStrategyString;
    Map< unsigned, Pair<unsigned, unsigned> > _gamma_abstract;
    List<Map<unsigned, bool>> _gamma;
    Map<unsigned, bool> _var_index_to_pos;
    Map<unsigned, bool> _var_index_to_inc;
    Map<unsigned, unsigned> _post_var_indices;
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

        options.setOptions();

        printf("marabou1\n");
        bool dnc = Options::get()->getBool( Options::DNC_MODE );

        Engine2 engine( std::make_shared<SplitProvidersManager>() );

        if(!engine.processInputQuery(inputQuery)) return std::make_pair(ret, *(engine.getStatistics()));
        if ( dnc )
        {
            auto dncManager = std::unique_ptr<DnCManager>( new DnCManager( &inputQuery ) );

            dncManager->solve();
            switch ( dncManager->getExitCode() )
            {
            case DnCManager::SAT:
            {
                retStats = Statistics();
                dncManager->getSolution( ret, inputQuery );
                break;
            }
            case DnCManager::TIMEOUT:
            {
                retStats = Statistics();
                retStats.timeout();
                return std::make_pair( ret, retStats );
            }
            default:
                return std::make_pair( ret, Statistics() ); // TODO: meaningful DnCStatistics
            }
        } else
        {
            unsigned timeoutInSeconds = Options::get()->getInt( Options::TIMEOUT );
            bool is_solved = false;
            auto gamma_abstract = Options::get()->getGammaAbstract( Options::GAMMA_ABSTRACT );
            auto gamma = Options::get()->getGamma( Options::GAMMA );
            auto var_index_to_pos = Options::get()->getVarIndexToPos( Options::VAR_INDEX_TO_POS );
            auto var_index_to_inc = Options::get()->getVarIndexToInc( Options::VAR_INDEX_TO_INC );
            auto post_var_indices = Options::get()->getPostVarIndices( Options::POST_VAR_INDICES );
            if (gamma_abstract.empty()){
                is_solved = engine.solve(timeoutInSeconds);
            } else {
                is_solved = engine.solve(timeoutInSeconds);
                // is_solved = engine.solve(
                //     gamma, gamma_abstract, var_index_to_pos, var_index_to_inc,
                //     post_var_indices, timeoutInSeconds
                // );
            }
            if(!is_solved) return std::make_pair(ret, *(engine.getStatistics()));

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

//template<typename T>
//void declare_List(py::module &m, std::string &typestr) {
//    using Class = List<T>;
////    std::string pyclass_name = std::string("List") + typestr;
//    std::string pyclass_name = typestr;
//    py::class_<Class>(m, pyclass_name.c_str(), py::buffer_protocol(), py::dynamic_attr())
//    .def(py::init())
//    .def("append", static_cast<void (Class::*) (const &Class)>(&Class::append)
//    .def("append", static_cast<void (Class::*) (const &T)>(&Class::append)
//    .def("appendHead", &Class::appendHead)
//    .def("size", &Class::size)
//    .def("empty", &Class::empty)
//    .def("exists", &Class::exists);
//}

// Code necessary to generate Python library
// Describes which classes and functions are exposed to API
PYBIND11_MODULE(MarabouCore, m) {
    m.doc() = "Maraboupy bindings to the C++ Marabou via pybind11";
    m.def("createInputQuery", &createInputQuery, "Create input query from network and property file");
    m.def("solve", &solve, R"pbdoc(
        Takes in a description of the InputQuery and returns the solution

        Args:
            inputQuery (:class:`~maraboupy.MarabouCore.InputQuery`): Marabou input query to be solved
            options (class:`~maraboupy.MarabouCore.Options`): Object defining the options used for Marabou
            redirect (str, optional): Filepath to direct standard output, defaults to ""

        Returns:
            (tuple): tuple containing:
                - vals (Dict[int, float]): Empty dictionary if UNSAT, otherwise a dictionary of SATisfying values for variables
                - stats (:class:`~maraboupy.MarabouCore.Statistics`): A Statistics object to how Marabou performed
        )pbdoc",
        py::arg("inputQuery"), py::arg("options"), py::arg("redirect") = "");
    m.def("saveQuery", &saveQuery, R"pbdoc(
        Serializes the inputQuery in the given filename

        Args:
            inputQuery (:class:`~maraboupy.MarabouCore.InputQuery`): Marabou input query to be saved
            filename (str): Name of file to save query
        )pbdoc",
        py::arg("inputQuery"), py::arg("filename"));
    m.def("loadQuery", &loadQuery, R"pbdoc(
        Loads and returns a serialized InputQuery from the given filename

        Args:
            filename (str): Name of file to load into an InputQuery

        Returns:
            :class:`~maraboupy.MarabouCore.InputQuery`
        )pbdoc",
        py::arg("filename"));
    m.def("addReluConstraint", &addReluConstraint, R"pbdoc(
        Add a Relu constraint to the InputQuery

        Args:
            inputQuery (:class:`~maraboupy.MarabouCore.InputQuery`): Marabou input query to be solved
            var1 (int): Input variable to Relu constraint
            var2 (int): Output variable to Relu constraint
        )pbdoc",
        py::arg("inputQuery"), py::arg("var1"), py::arg("var2"));
    m.def("addSignConstraint", &addSignConstraint, R"pbdoc(
        Add a Sign constraint to the InputQuery

        Args:
            inputQuery (:class:`~maraboupy.MarabouCore.InputQuery`): Marabou input query to be solved
            var1 (int): Input variable to Sign constraint
            var2 (int): Output variable to Sign constraint
        )pbdoc",
          py::arg("inputQuery"), py::arg("var1"), py::arg("var2"));
    m.def("addMaxConstraint", &addMaxConstraint, R"pbdoc(
        Add a Max constraint to the InputQuery

        Args:
            inputQuery (:class:`~maraboupy.MarabouCore.InputQuery`): Marabou input query to be solved
            elements (set of int): Input variables to max constraint
            v (int): Output variable from max constraint
        )pbdoc",
        py::arg("inputQuery"), py::arg("elements"), py::arg("v"));
    m.def("addAbsConstraint", &addAbsConstraint, R"pbdoc(
        Add an Abs constraint to the InputQuery

        Args:
            inputQuery (:class:`~maraboupy.MarabouCore.InputQuery`): Marabou input query to be solved
            b (int): Input variable
            f (int): Output variable
        )pbdoc",
        py::arg("inputQuery"), py::arg("b"), py::arg("f"));
    m.def( "addDisjunctionConstraint", &addDisjunctionConstraint, R"pbdoc(
        Add a disjunction constraint to the InputQuery

        Args:
            inputQuery (:class:`~maraboupy.MarabouCore.InputQuery`): Marabou input query to be solved
            disjuncts (list of pairs): A list of disjuncts. Each disjunct is represented by a pair: a list of bounds, and a list of (in)equalities.
        )pbdoc",
        py::arg("inputQuery"), py::arg("disjuncts"));
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
        .def("outputVariableByIndex", &InputQuery::outputVariableByIndex);
    py::class_<MarabouOptions>(m, "Options")
        .def(py::init())
        .def_readwrite("_numWorkers", &MarabouOptions::_numWorkers)
        .def_readwrite("_initialTimeout", &MarabouOptions::_initialTimeout)
        .def_readwrite("_initialDivides", &MarabouOptions::_initialDivides)
        .def_readwrite("_onlineDivides", &MarabouOptions::_onlineDivides)
        .def_readwrite("_timeoutInSeconds", &MarabouOptions::_timeoutInSeconds)
        .def_readwrite("_timeoutFactor", &MarabouOptions::_timeoutFactor)
        .def_readwrite("_preprocessorBoundTolerance", &MarabouOptions::_preprocessorBoundTolerance)
        .def_readwrite("_gamma_abstract", &MarabouOptions::_gamma_abstract)
        .def_readwrite("_gamma", &MarabouOptions::_gamma)
        .def_readwrite("_verbosity", &MarabouOptions::_verbosity)
        .def_readwrite("_splitThreshold", &MarabouOptions::_splitThreshold)
        .def_readwrite("_snc", &MarabouOptions::_snc)
        .def_readwrite("_solveWithMILP", &MarabouOptions::_solveWithMILP)
        .def_readwrite("_restoreTreeStates", &MarabouOptions::_restoreTreeStates)
        .def_readwrite("_splittingStrategy", &MarabouOptions::_splittingStrategyString)
        .def_readwrite("_sncSplittingStrategy", &MarabouOptions::_sncSplittingStrategyString);
    py::enum_<PiecewiseLinearFunctionType>(m, "PiecewiseLinearFunctionType")
        .value("ReLU", PiecewiseLinearFunctionType::RELU)
        .value("AbsoluteValue", PiecewiseLinearFunctionType::ABSOLUTE_VALUE)
        .value("Max", PiecewiseLinearFunctionType::MAX)
        .value("Disjunction", PiecewiseLinearFunctionType::DISJUNCTION)
        .export_values();
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
        .def("hasTimedOut", &Statistics::hasTimedOut)
        .def("getGammaUnsatSplitSequences", &Statistics::getGammaUnsatSplitSequences);
    py::class_<PiecewiseLinearCaseSplit>(m, "PiecewiseLinearCaseSplit")
        .def(py::init())
        .def("getBoundTightenings", &PiecewiseLinearCaseSplit::getBoundTightenings)
        .def("getEquations", &PiecewiseLinearCaseSplit::getEquations);
    py::class_<Map<unsigned, Pair<unsigned, unsigned>>>(m, "GammaAbstract")
        .def(py::init())
        .def("items", [](Map<unsigned, Pair<unsigned, unsigned>> &map) { return py::make_iterator(map.begin(), map.end()); },
                py::keep_alive<0, 1>())
        .def("__iter__", [](List<Map<unsigned, PiecewiseLinearCaseSplit>> &g) { return py::make_iterator(g.begin1(), g.end1()); },
                         py::keep_alive<0, 1>() /* Essential: keep object alive while iterator exists */)
        .def("insert", &Map<unsigned, Pair<unsigned, unsigned>>::insert)
        .def("keys", &Map<unsigned, Pair<unsigned, unsigned>>::keys)
        .def("values", &Map<unsigned, Pair<unsigned, unsigned>>::values)
        .def("get", &Map<unsigned, Pair<unsigned, unsigned>>::get)
        .def("size", &Map<unsigned, Pair<unsigned, unsigned>>::size)
        .def("empty", &Map<unsigned, Pair<unsigned, unsigned>>::empty);
    py::class_<List<Map<unsigned int, bool>>::iterator>(m, "GammaListIterator");
//        .def("next", static_cast<List<Map<unsigned int, bool> >::iterator (List<Map<unsigned int, bool> >::*)()>(&List<Map<unsigned, bool>>::iterator::operator++), "next");
//        .def("next", &List<Map<unsigned int, bool>>::iterator::operator++);
    py::class_<List<Map<unsigned, bool>>>(m, "Gamma")
        .def(py::init())
// WORK       .def("append", static_cast<void (List<Map<unsigned, bool>::*) (const List<Map<unsigned, bool>)>(&List<Map<unsigned, bool>::append)
// WORK       .def("append", static_cast<void (List<Map<unsigned, bool>::*) (const Map<unsigned, bool>)>(&List<Map<unsigned, bool>>::append)
// NOT WORK       .def("back", static_cast<void (List<Map<unsigned, bool>>::*)( const Map<unsigned, bool> &)>(&List<Map<unsigned, bool>>::back), "back1")
// NOT WORK       .def("back", static_cast<void (List<Map<unsigned, bool>>::*)(const List<Map<unsigned, bool>> &)>(&List<Map<unsigned, bool>>::back), "back2")
// NOT WORK       .def("begin", static_cast<void (List<Map<unsigned, bool>>::*)(List::iterator)>(&List<Map<unsigned, bool>>::begin), "begin iterator")
// NOT WORK       .def("begin", static_cast<void (List<Map<unsigned, bool>>::*)(List::const_iterator)>(&List<Map<unsigned, bool>>::begin), "begin const_iterator")
        .def("append", static_cast<void (List<Map<unsigned, bool>>::*)( const Map<unsigned, bool> &)>(&List<Map<unsigned, bool>>::append), "append gamma list to gamma list (i.e. extend)")
        .def("append", static_cast<void (List<Map<unsigned, bool>>::*)(const List<Map<unsigned, bool>> &)>(&List<Map<unsigned, bool>>::append), "append value to gamma list")
        .def("append1", &List<Map<unsigned, bool>>::append1)
        .def("append2", &List<Map<unsigned, bool>>::append2)
        .def("appendHead", &List<Map<unsigned, bool>>::appendHead)
        .def("__iter__", [](List<Map<unsigned, bool>> &g) { return py::make_iterator(g.begin1(), g.end1()); },
                         py::keep_alive<0, 1>() /* Essential: keep object alive while iterator exists */)
        .def("begin1", static_cast<List<Map<unsigned int, bool> >::iterator (List<Map<unsigned int, bool> >::*)()>(&List<Map<unsigned, bool>>::begin1), "begin1 iterator")
//        .def("begin1", &List<Map<unsigned, bool>>::begin1)
        .def("begin2", &List<Map<unsigned, bool>>::begin2)
        .def("rbegin1", &List<Map<unsigned, bool>>::rbegin1)
        .def("rbegin2", &List<Map<unsigned, bool>>::rbegin2)
        .def("end1", &List<Map<unsigned, bool>>::end1)
        .def("end2", &List<Map<unsigned, bool>>::end2)
        .def("rend1", &List<Map<unsigned, bool>>::rend1)
        .def("rend2", &List<Map<unsigned, bool>>::rend2)
        .def("erase1", &List<Map<unsigned, bool>>::erase1)
        .def("erase2", &List<Map<unsigned, bool>>::erase2)
        .def("clear", &List<Map<unsigned, bool>>::clear)
        .def("size", &List<Map<unsigned, bool>>::size)
        .def("empty", &List<Map<unsigned, bool>>::empty)
        .def("exists", &List<Map<unsigned, bool>>::exists)
        .def("front1", &List<Map<unsigned, bool>>::front1)
        .def("front2", &List<Map<unsigned, bool>>::front2)
        .def("back1", &List<Map<unsigned, bool>>::back1)
        .def("back2", &List<Map<unsigned, bool>>::back2)
        .def("popBack", &List<Map<unsigned, bool>>::popBack)
        .def("is_equal", &List<Map<unsigned, bool>>::operator==)
        .def("is_differ", &List<Map<unsigned, bool>>::operator!=);
    py::class_<List<Tightening>>(m, "ListTightening")
        .def(py::init())
// WORK       .def("append", static_cast<void (List<Map<unsigned, bool>::*) (const List<Map<unsigned, bool>)>(&List<Map<unsigned, bool>::append)
// WORK       .def("append", static_cast<void (List<Map<unsigned, bool>::*) (const Map<unsigned, bool>)>(&List<Map<unsigned, bool>>::append)
// NOT WORK       .def("back", static_cast<void (List<Map<unsigned, bool>>::*)( const Map<unsigned, bool> &)>(&List<Map<unsigned, bool>>::back), "back1")
// NOT WORK       .def("back", static_cast<void (List<Map<unsigned, bool>>::*)(const List<Map<unsigned, bool>> &)>(&List<Map<unsigned, bool>>::back), "back2")
// NOT WORK       .def("begin", static_cast<void (List<Map<unsigned, bool>>::*)(List::iterator)>(&List<Map<unsigned, bool>>::begin), "begin iterator")
// NOT WORK       .def("begin", static_cast<void (List<Map<unsigned, bool>>::*)(List::const_iterator)>(&List<Map<unsigned, bool>>::begin), "begin const_iterator")
        .def("append1", &List<Tightening>::append1)
        .def("append2", &List<Tightening>::append2)
        .def("appendHead", &List<Tightening>::appendHead)
        .def("begin1", &List<Tightening>::begin1)
        .def("begin2", &List<Tightening>::begin2)
        .def("rbegin1", &List<Tightening>::rbegin1)
        .def("rbegin2", &List<Tightening>::rbegin2)
        .def("end1", &List<Tightening>::end1)
        .def("end2", &List<Tightening>::end2)
        .def("rend1", &List<Tightening>::rend1)
        .def("rend2", &List<Tightening>::rend2)
        .def("erase1", &List<Tightening>::erase1)
        .def("erase2", &List<Tightening>::erase2)
        .def("clear", &List<Tightening>::clear)
        .def("size", &List<Tightening>::size)
        .def("empty", &List<Tightening>::empty)
        .def("exists", &List<Tightening>::exists)
        .def("front1", &List<Tightening>::front1)
        .def("front2", &List<Tightening>::front2)
        .def("back1", &List<Tightening>::back1)
        .def("back2", &List<Tightening>::back2)
        .def("popBack", &List<Tightening>::popBack)
        .def("is_equal", &List<Tightening>::operator==)
        .def("is_differ", &List<Tightening>::operator!=);
    py::class_<Pair<unsigned, unsigned>>(m, "Pair")
        .def(py::init());
//    declare_List<GammaMap>(m, "Gamma");
}
