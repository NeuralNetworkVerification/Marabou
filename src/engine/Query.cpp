/*********************                                                        */
/*! \file Query.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Christopher Lazarus, Shantanu Thakoor, Andrew Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#include "Query.h"

#include "AutoFile.h"
#include "BilinearConstraint.h"
#include "Debug.h"
#include "FloatUtils.h"
#include "LeakyReluConstraint.h"
#include "MStringf.h"
#include "MarabouError.h"
#include "MaxConstraint.h"
#include "Options.h"
#include "RoundConstraint.h"
#include "SoftmaxConstraint.h"
#include "SymbolicBoundTighteningType.h"

#define INPUT_QUERY_LOG( x, ... )                                                                  \
    LOG( GlobalConfiguration::INPUT_QUERY_LOGGING, "Input Query: %s\n", x )

Query::Query()
    : _ensureSameSourceLayerInNLR( Options::get()->getSymbolicBoundTighteningType() ==
                                   SymbolicBoundTighteningType::DEEP_POLY )
    , _networkLevelReasoner( NULL )
{
}

Query::~Query()
{
    freeConstraintsIfNeeded();
    if ( _networkLevelReasoner )
    {
        delete _networkLevelReasoner;
        _networkLevelReasoner = NULL;
    }
}

void Query::setNumberOfVariables( unsigned numberOfVariables )
{
    _numberOfVariables = numberOfVariables;
}

void Query::setLowerBound( unsigned variable, double bound )
{
    if ( variable >= _numberOfVariables )
    {
        throw MarabouError( MarabouError::VARIABLE_INDEX_OUT_OF_RANGE,
                            Stringf( "Variable = %u, number of variables = %u (setLowerBound)",
                                     variable,
                                     _numberOfVariables )
                                .ascii() );
    }

    _lowerBounds[variable] = bound;
}

void Query::setUpperBound( unsigned variable, double bound )
{
    if ( variable >= _numberOfVariables )
    {
        throw MarabouError( MarabouError::VARIABLE_INDEX_OUT_OF_RANGE,
                            Stringf( "Variable = %u, number of variables = %u (setUpperBound)",
                                     variable,
                                     _numberOfVariables )
                                .ascii() );
    }

    _upperBounds[variable] = bound;
}

bool Query::tightenLowerBound( unsigned variable, double bound )
{
    if ( variable >= _numberOfVariables )
    {
        throw MarabouError( MarabouError::VARIABLE_INDEX_OUT_OF_RANGE,
                            Stringf( "Variable = %u, number of variables = %u (tightenLowerBound)",
                                     variable,
                                     getNumberOfVariables() )
                                .ascii() );
    }

    if ( !_lowerBounds.exists( variable ) || _lowerBounds[variable] < bound )
    {
        _lowerBounds[variable] = bound;
        return true;
    }
    return false;
}

bool Query::tightenUpperBound( unsigned variable, double bound )
{
    if ( variable >= _numberOfVariables )
    {
        throw MarabouError( MarabouError::VARIABLE_INDEX_OUT_OF_RANGE,
                            Stringf( "Variable = %u, number of variables = %u (tightenUpperBound)",
                                     variable,
                                     getNumberOfVariables() )
                                .ascii() );
    }

    if ( !_upperBounds.exists( variable ) || _upperBounds[variable] > bound )
    {
        _upperBounds[variable] = bound;
        return true;
    }
    return false;
}


void Query::addEquation( const Equation &equation )
{
    _equations.append( equation );
}

unsigned Query::getNumberOfEquations() const
{
    return _equations.size();
}

unsigned Query::getNumberOfVariables() const
{
    return _numberOfVariables;
}

unsigned Query::getNewVariable()
{
    return _numberOfVariables++;
}

double Query::getLowerBound( unsigned variable ) const
{
    if ( variable >= _numberOfVariables )
    {
        throw MarabouError( MarabouError::VARIABLE_INDEX_OUT_OF_RANGE,
                            Stringf( "Variable = %u, number of variables = %u (getLowerBound)",
                                     variable,
                                     _numberOfVariables )
                                .ascii() );
    }

    if ( !_lowerBounds.exists( variable ) )
        return FloatUtils::negativeInfinity();

    return _lowerBounds.get( variable );
}

double Query::getUpperBound( unsigned variable ) const
{
    if ( variable >= _numberOfVariables )
    {
        throw MarabouError( MarabouError::VARIABLE_INDEX_OUT_OF_RANGE,
                            Stringf( "Variable = %u, number of variables = %u (getUpperBound)",
                                     variable,
                                     _numberOfVariables )
                                .ascii() );
    }

    if ( !_upperBounds.exists( variable ) )
        return FloatUtils::infinity();

    return _upperBounds.get( variable );
}

List<Equation> &Query::getEquations()
{
    return _equations;
}

void Query::removeEquationsByIndex( const Set<unsigned> indices )
{
    unsigned m = _equations.size();
    List<Equation>::iterator it = _equations.begin();

    for ( unsigned index = 0; index < m; ++index )
    {
        if ( indices.exists( index ) )
            it = _equations.erase( it );
        else
            ++it;
    }
}

const List<Equation> &Query::getEquations() const
{
    return _equations;
}

void Query::setSolutionValue( unsigned variable, double value )
{
    _solution[variable] = value;
}

double Query::getSolutionValue( unsigned variable ) const
{
    if ( !_solution.exists( variable ) )
        throw MarabouError( MarabouError::VARIABLE_DOESNT_EXIST_IN_SOLUTION,
                            Stringf( "Variable: %u", variable ).ascii() );

    return _solution.get( variable );
}

void Query::addPiecewiseLinearConstraint( PiecewiseLinearConstraint *constraint )
{
    _plConstraints.append( constraint );
}

void Query::addClipConstraint( unsigned b, unsigned f, double floor, double ceiling )
{
    /*
      f = clip(b, floor, ceiling)
      -
      aux1 = b - floor
      aux2 = relu(aux1)
      aux2.5 = aux2 + floor
      aux3 = -aux2.5 + ceiling
      aux4 = relu(aux3)
      f = -aux4 + ceiling
    */

    // aux1 = var1 - floor
    unsigned aux1 = getNewVariable();
    Equation eq1( Equation::EQ );
    eq1.addAddend( 1.0, b );
    eq1.addAddend( -1.0, aux1 );
    eq1.setScalar( floor );
    addEquation( eq1 );
    unsigned aux2 = getNewVariable();
    PiecewiseLinearConstraint *r1 = new ReluConstraint( aux1, aux2 );
    addPiecewiseLinearConstraint( r1 );
    // aux2.5 = aux2 + floor
    // aux3 = -aux2.5 + ceiling
    // So aux3 = -aux2 - floor + ceiling
    unsigned aux3 = getNewVariable();
    Equation eq2( Equation::EQ );
    eq2.addAddend( -1.0, aux2 );
    eq2.addAddend( -1.0, aux3 );
    eq2.setScalar( floor - ceiling );
    addEquation( eq2 );

    unsigned aux4 = getNewVariable();
    PiecewiseLinearConstraint *r2 = new ReluConstraint( aux3, aux4 );
    addPiecewiseLinearConstraint( r2 );

    // aux4.5 = aux4 - ceiling
    // f = -aux4.5
    // f = -aux4 + ceiling
    Equation eq3( Equation::EQ );
    eq3.addAddend( -1.0, aux4 );
    eq3.addAddend( -1.0, f );
    eq3.setScalar( -ceiling );
    addEquation( eq3 );
}

List<PiecewiseLinearConstraint *> &Query::getPiecewiseLinearConstraints()
{
    return _plConstraints;
}

const List<PiecewiseLinearConstraint *> &Query::getPiecewiseLinearConstraints() const
{
    return _plConstraints;
}

void Query::addNonlinearConstraint( NonlinearConstraint *constraint )
{
    _nlConstraints.append( constraint );
}

void Query::getNonlinearConstraints( Vector<NonlinearConstraint *> &constraints ) const
{
    for ( const auto &constraint : _nlConstraints )
        constraints.append( constraint );
}

List<NonlinearConstraint *> &Query::getNonlinearConstraints()
{
    return _nlConstraints;
}

const List<NonlinearConstraint *> &Query::getNonlinearConstraints() const
{
    return _nlConstraints;
}

unsigned Query::countInfiniteBounds()
{
    unsigned result = 0;

    for ( const auto &lowerBound : _lowerBounds )
        if ( lowerBound.second == FloatUtils::negativeInfinity() )
            ++result;

    for ( const auto &upperBound : _upperBounds )
        if ( upperBound.second == FloatUtils::infinity() )
            ++result;

    for ( unsigned i = 0; i < _numberOfVariables; ++i )
    {
        if ( !_lowerBounds.exists( i ) )
            ++result;
        if ( !_upperBounds.exists( i ) )
            ++result;
    }

    return result;
}

void Query::mergeIdenticalVariables( unsigned v1, unsigned v2 )
{
    // Handle equations
    for ( auto &equation : getEquations() )
        equation.updateVariableIndex( v1, v2 );

    // Handle PL constraints
    for ( auto &plConstraint : getPiecewiseLinearConstraints() )
    {
        if ( plConstraint->participatingVariable( v1 ) )
        {
            ASSERT( !plConstraint->participatingVariable( v2 ) );
            plConstraint->updateVariableIndex( v1, v2 );
        }
    }

    // Handle Nonlinear constraints
    for ( auto &nlConstraint : getNonlinearConstraints() )
    {
        if ( nlConstraint->participatingVariable( v1 ) )
        {
            ASSERT( !nlConstraint->participatingVariable( v2 ) );
            nlConstraint->updateVariableIndex( v1, v2 );
        }
    }
    // TODO: update lower and upper bounds
}

void Query::removeEquation( Equation e )
{
    _equations.erase( e );
}

Query &Query::operator=( const Query &other )
{
    INPUT_QUERY_LOG( "Calling deep copy constructor..." );

    _ensureSameSourceLayerInNLR = other._ensureSameSourceLayerInNLR;

    _numberOfVariables = other._numberOfVariables;
    _equations = other._equations;
    _lowerBounds = other._lowerBounds;
    _upperBounds = other._upperBounds;
    _solution = other._solution;
    _debuggingSolution = other._debuggingSolution;

    _variableToInputIndex = other._variableToInputIndex;
    _inputIndexToVariable = other._inputIndexToVariable;
    _variableToOutputIndex = other._variableToOutputIndex;
    _outputIndexToVariable = other._outputIndexToVariable;

    freeConstraintsIfNeeded();

    // Setting NLR
    if ( other._networkLevelReasoner )
    {
        if ( !_networkLevelReasoner )
            _networkLevelReasoner = new NLR::NetworkLevelReasoner;
        other._networkLevelReasoner->storeIntoOther( *_networkLevelReasoner );
    }
    else
    {
        if ( _networkLevelReasoner )
        {
            delete _networkLevelReasoner;
            _networkLevelReasoner = NULL;
        }
    }

    // Setting nlConstraints
    for ( const auto &constraint : other._nlConstraints )
        _nlConstraints.append( constraint->duplicateConstraint() );

    // Setting plConstraints and topological order
    if ( !other._networkLevelReasoner )
    {
        for ( const auto &constraint : other._plConstraints )
            _plConstraints.append( constraint->duplicateConstraint() );
    }
    else
    {
        INPUT_QUERY_LOG( Stringf( "Number of piecewise linear constraints in input query: %u",
                                  other._plConstraints.size() )
                             .ascii() );
        INPUT_QUERY_LOG( Stringf( "Number of nonlinear constraints in input query: %u",
                                  other._nlConstraints.size() )
                             .ascii() );
        INPUT_QUERY_LOG(
            Stringf( "Number of piecewise linear constraints in topological order %u",
                     other._networkLevelReasoner->getConstraintsInTopologicalOrder().size() )
                .ascii() );

        unsigned numberOfConstraintsUnhandledByNLR = 0;
        for ( const auto &constraint : other._plConstraints )
        {
            if ( !other._networkLevelReasoner->getConstraintsInTopologicalOrder().exists(
                     constraint ) )
            {
                auto *newPlc = constraint->duplicateConstraint();
                _plConstraints.append( newPlc );
                ++numberOfConstraintsUnhandledByNLR;
            }
        }

        ASSERT( other._networkLevelReasoner->getConstraintsInTopologicalOrder().size() +
                    numberOfConstraintsUnhandledByNLR ==
                other._plConstraints.size() );

        for ( const auto &constraint :
              other._networkLevelReasoner->getConstraintsInTopologicalOrder() )
        {
            auto *newPlc = constraint->duplicateConstraint();
            _plConstraints.append( newPlc );
            _networkLevelReasoner->addConstraintInTopologicalOrder( newPlc );
        }

        ASSERT( _plConstraints.size() == other._plConstraints.size() );
        ASSERT( _networkLevelReasoner->getConstraintsInTopologicalOrder().size() ==
                other._networkLevelReasoner->getConstraintsInTopologicalOrder().size() );
    }

    INPUT_QUERY_LOG( "Calling deep copy constructor - done\n" );
    return *this;
}

Query::Query( const Query &other )
    : _networkLevelReasoner( NULL )
{
    *this = other;
}

void Query::freeConstraintsIfNeeded()
{
    for ( auto &it : _plConstraints )
        delete it;

    _plConstraints.clear();

    for ( auto &it : _nlConstraints )
        delete it;

    _nlConstraints.clear();
}

const Map<unsigned, double> &Query::getLowerBounds() const
{
    return _lowerBounds;
}

const Map<unsigned, double> &Query::getUpperBounds() const
{
    return _upperBounds;
}

void Query::clearBounds()
{
    _lowerBounds.clear();
    _upperBounds.clear();
}

void Query::storeDebuggingSolution( unsigned variable, double value )
{
    _debuggingSolution[variable] = value;
}

void Query::saveQuery( const String &fileName )
{
    AutoFile queryFile( fileName );
    queryFile->open( IFile::MODE_WRITE_TRUNCATE );

    // Number of Variables
    queryFile->write( Stringf( "%u\n", _numberOfVariables ) );

    // Number of Bounds
    queryFile->write( Stringf( "%u\n", _lowerBounds.size() ) );
    queryFile->write( Stringf( "%u\n", _upperBounds.size() ) );

    // Number of Equations
    queryFile->write( Stringf( "%u\n", _equations.size() ) );

    // Number of Non-linear Constraints
    queryFile->write( Stringf( "%u", _plConstraints.size() + _nlConstraints.size() ) );

    printf( "Number of variables: %u\n", _numberOfVariables );
    printf( "Number of lower bounds: %u\n", _lowerBounds.size() );
    printf( "Number of upper bounds: %u\n", _upperBounds.size() );
    printf( "Number of equations: %u\n", _equations.size() );
    printf( "Number of non-linear constraints: %u\n",
            _plConstraints.size() + _nlConstraints.size() );

    // Number of Input Variables
    queryFile->write( Stringf( "\n%u", getNumInputVariables() ) );

    // Input Variables
    unsigned i = 0;
    for ( const auto &inVar : getInputVariables() )
    {
        queryFile->write( Stringf( "\n%u,%u", i, inVar ) );
        ++i;
    }
    ASSERT( i == getNumInputVariables() );

    // Number of Output Variables
    queryFile->write( Stringf( "\n%u", getNumOutputVariables() ) );

    // Output Variables
    i = 0;
    for ( const auto &outVar : getOutputVariables() )
    {
        queryFile->write( Stringf( "\n%u,%u", i, outVar ) );
        ++i;
    }
    ASSERT( i == getNumOutputVariables() );

    // Lower Bounds
    for ( const auto &lb : _lowerBounds )
        queryFile->write( Stringf( "\n%d,%.10f", lb.first, lb.second ) );

    // Upper Bounds
    for ( const auto &ub : _upperBounds )
        queryFile->write( Stringf( "\n%d,%.10f", ub.first, ub.second ) );

    // Equations
    i = 0;
    for ( const auto &e : _equations )
    {
        // Equation number
        queryFile->write( Stringf( "\n%u,", i ) );

        // Equation type
        queryFile->write( Stringf( "%01d,", e._type ) );

        // Equation scalar
        queryFile->write( Stringf( "%.10f", e._scalar ) );
        for ( const auto &a : e._addends )
            queryFile->write( Stringf( ",%u,%.10f", a._variable, a._coefficient ) );

        ++i;
    }

    // Piecewise-Linear Constraints
    i = 0;
    for ( const auto &constraint : _plConstraints )
    {
        // Constraint number
        queryFile->write( Stringf( "\n%u,", i ) );
        queryFile->write( constraint->serializeToString() );
        ++i;
    }

    // Nonlinear Constraints
    i = 0;
    for ( const auto &constraint : _nlConstraints )
    {
        // Constraint number
        queryFile->write( Stringf( "\n%u,", i ) );
        queryFile->write( constraint->serializeToString() );
        ++i;
    }

    queryFile->close();
}

void Query::saveQueryAsSmtLib( const String &fileName ) const
{
    if ( !_nlConstraints.empty() )
    {
        printf( "SMTLIB conversion does not support nonlinear constraints yet. Aborting "
                "Conversion.\n" );
        return;
    }

    List<Equation> equations;

    Vector<double> upperBounds( _numberOfVariables, 0 );
    Vector<double> lowerBounds( _numberOfVariables, 0 );

    for ( unsigned i = 0; i < _numberOfVariables; ++i )
    {
        upperBounds[i] = _upperBounds.exists( i ) ? _upperBounds[i] : FloatUtils::infinity();
        lowerBounds[i] =
            _lowerBounds.exists( i ) ? _lowerBounds[i] : FloatUtils::negativeInfinity();
    }

    SmtLibWriter::writeToSmtLibFile( fileName,
                                     0,
                                     _numberOfVariables,
                                     upperBounds,
                                     lowerBounds,
                                     NULL,
                                     _equations,
                                     _plConstraints );
}

void Query::markInputVariable( unsigned variable, unsigned inputIndex )
{
    _variableToInputIndex[variable] = inputIndex;
    _inputIndexToVariable[inputIndex] = variable;
}

void Query::markOutputVariable( unsigned variable, unsigned outputIndex )
{
    _variableToOutputIndex[variable] = outputIndex;
    _outputIndexToVariable[outputIndex] = variable;
}

unsigned Query::inputVariableByIndex( unsigned index ) const
{
    ASSERT( _inputIndexToVariable.exists( index ) );
    return _inputIndexToVariable.get( index );
}

unsigned Query::outputVariableByIndex( unsigned index ) const
{
    ASSERT( _outputIndexToVariable.exists( index ) );
    return _outputIndexToVariable.get( index );
}

unsigned Query::getNumInputVariables() const
{
    return _inputIndexToVariable.size();
}

unsigned Query::getNumOutputVariables() const
{
    return _outputIndexToVariable.size();
}

List<unsigned> Query::getInputVariables() const
{
    List<unsigned> result;
    for ( const auto &pair : _variableToInputIndex )
        result.append( pair.first );

    return result;
}

List<unsigned> Query::getOutputVariables() const
{
    List<unsigned> result;
    for ( const auto &pair : _variableToOutputIndex )
        result.append( pair.first );

    return result;
}

void Query::printAllBounds() const
{
    printf( "Query: Dumping all bounds\n" );

    for ( unsigned i = 0; i < _numberOfVariables; ++i )
    {
        printf( "\tx%u: [", i );
        if ( _lowerBounds.exists( i ) )
            printf( "%lf, ", _lowerBounds[i] );
        else
            printf( "-INF, " );

        if ( _upperBounds.exists( i ) )
            printf( "%lf]", _upperBounds[i] );
        else
            printf( "+INF]" );
        printf( "\n" );
    }

    printf( "\n\n" );
}

void Query::printInputOutputBounds() const
{
    printf( "Dumping bounds of the input and output variables:\n" );

    for ( const auto &pair : _variableToInputIndex )
    {
        printf( "\tInput %u (var %u): [%lf, %lf]\n",
                pair.second,
                pair.first,
                _lowerBounds[pair.first],
                _upperBounds[pair.first] );
    }

    for ( const auto &pair : _variableToOutputIndex )
    {
        printf( "\tOutput %u (var %u): [%lf, %lf]\n",
                pair.second,
                pair.first,
                _lowerBounds[pair.first],
                _upperBounds[pair.first] );
    }
}

void Query::dump() const
{
    printf( "Total number of variables: %u\n", _numberOfVariables );
    printf( "Input variables:\n" );
    for ( const auto &input : _inputIndexToVariable )
        printf( "\tx%u\n", input.second );

    printf( "Output variables:\n" );
    for ( const auto &output : _outputIndexToVariable )
        printf( "\tx%u\n", output.second );

    printf( "Variable bounds:\n" );
    for ( unsigned i = 0; i < _numberOfVariables; ++i )
    {
        printf( "\t %u: [%s, %s]\n",
                i,
                _lowerBounds.exists( i ) ? Stringf( "%lf", _lowerBounds[i] ).ascii() : "-inf",
                _upperBounds.exists( i ) ? Stringf( "%lf", _upperBounds[i] ).ascii() : "inf" );
    }

    printf( "Constraints:\n" );
    String constraintString;
    for ( const auto &pl : _plConstraints )
    {
        pl->dump( constraintString );
        printf( "\t%s\n", constraintString.ascii() );
    }

    for ( const auto &ts : _nlConstraints )
    {
        ts->dump( constraintString );
        printf( "\t%s\n", constraintString.ascii() );
    }

    printf( "Equations:\n" );
    for ( const auto &e : _equations )
    {
        printf( "\t" );
        e.dump();
    }
}

void Query::adjustInputOutputMapping( const Map<unsigned, unsigned> &oldIndexToNewIndex,
                                      const Map<unsigned, unsigned> &mergedVariables )
{
    Map<unsigned, unsigned> newInputIndexToVariable;
    unsigned currentIndex = 0;

    // Input variables
    for ( const auto &it : _inputIndexToVariable )
    {
        if ( mergedVariables.exists( it.second ) )
            throw MarabouError(
                MarabouError::MERGED_INPUT_VARIABLE,
                Stringf( "Input variable %u has been merged\n", it.second ).ascii() );

        if ( oldIndexToNewIndex.exists( it.second ) )
        {
            newInputIndexToVariable[currentIndex] = oldIndexToNewIndex[it.second];
            ++currentIndex;
        }
    }
    _inputIndexToVariable = newInputIndexToVariable;

    _variableToInputIndex.clear();
    for ( auto it : _inputIndexToVariable )
        _variableToInputIndex[it.second] = it.first;

    Map<unsigned, unsigned> newOutputIndexToVariable;
    currentIndex = 0;

    // Output variables
    for ( const auto &it : _outputIndexToVariable )
    {
        if ( mergedVariables.exists( it.second ) )
            throw MarabouError(
                MarabouError::MERGED_OUTPUT_VARIABLE,
                Stringf( "Output variable %u has been merged\n", it.second ).ascii() );

        if ( oldIndexToNewIndex.exists( it.second ) )
        {
            newOutputIndexToVariable[currentIndex] = oldIndexToNewIndex[it.second];
            ++currentIndex;
        }
    }
    _outputIndexToVariable = newOutputIndexToVariable;

    _variableToOutputIndex.clear();
    for ( auto it : _outputIndexToVariable )
        _variableToOutputIndex[it.second] = it.first;
}

Query *Query::generateQuery() const
{
    return new Query( *this );
}

void Query::setNetworkLevelReasoner( NLR::NetworkLevelReasoner *nlr )
{
    _networkLevelReasoner = nlr;
}

NLR::NetworkLevelReasoner *Query::getNetworkLevelReasoner() const
{
    return _networkLevelReasoner;
}

bool Query::constructNetworkLevelReasoner( List<Equation> &unhandledEquations,
                                           Set<unsigned> &varsInUnhandledConstraints )
{
    INPUT_QUERY_LOG( "PP: constructing an NLR... " );

    if ( _networkLevelReasoner )
        delete _networkLevelReasoner;
    NLR::NetworkLevelReasoner *nlr = new NLR::NetworkLevelReasoner;

    Map<unsigned, unsigned> handledVariableToLayer;

    // First, put all the input neurons in layer 0
    List<unsigned> inputs = getInputVariables();
    // If there is no input variable, don't construct the nlr
    if ( inputs.empty() )
    {
        INPUT_QUERY_LOG( "unsuccessful\n" );
        delete nlr;
        return false;
    }

    nlr->addLayer( 0, NLR::Layer::INPUT, inputs.size() );
    unsigned index = 0;

    NLR::Layer *inputLayer = nlr->getLayer( 0 );
    for ( const auto &inputVariable : inputs )
    {
        nlr->setNeuronVariable( NLR::NeuronIndex( 0, index ), inputVariable );
        handledVariableToLayer[inputVariable] = 0;

        inputLayer->setLb( index,
                           _lowerBounds.exists( inputVariable ) ? _lowerBounds[inputVariable]
                                                                : FloatUtils::negativeInfinity() );
        inputLayer->setUb( index,
                           _upperBounds.exists( inputVariable ) ? _upperBounds[inputVariable]
                                                                : FloatUtils::infinity() );

        ++index;
    }

    unsigned newLayerIndex = 1;
    Set<unsigned> handledEquations;
    Set<PiecewiseLinearConstraint *> handledPLConstraints;
    Set<NonlinearConstraint *> handledNLConstraints;
    // Now, repeatedly attempt to construct additional layers
    while (
        constructWeighedSumLayer( nlr, handledVariableToLayer, newLayerIndex, handledEquations ) ||
        constructReluLayer( nlr, handledVariableToLayer, newLayerIndex, handledPLConstraints ) ||
        constructRoundLayer( nlr, handledVariableToLayer, newLayerIndex, handledNLConstraints ) ||
        constructLeakyReluLayer(
            nlr, handledVariableToLayer, newLayerIndex, handledPLConstraints ) ||
        constructAbsoluteValueLayer(
            nlr, handledVariableToLayer, newLayerIndex, handledPLConstraints ) ||
        constructSignLayer( nlr, handledVariableToLayer, newLayerIndex, handledPLConstraints ) ||
        constructSigmoidLayer( nlr, handledVariableToLayer, newLayerIndex, handledNLConstraints ) ||
        constructMaxLayer( nlr, handledVariableToLayer, newLayerIndex, handledPLConstraints ) ||
        constructBilinearLayer(
            nlr, handledVariableToLayer, newLayerIndex, handledNLConstraints ) ||
        constructSoftmaxLayer( nlr, handledVariableToLayer, newLayerIndex, handledNLConstraints ) )
    {
        ++newLayerIndex;
    }

    bool success = ( newLayerIndex > 1 );

    if ( success )
    {
        unsigned count = 0;
        for ( unsigned i = 0; i < nlr->getNumberOfLayers(); ++i )
            count += nlr->getLayer( i )->getSize();

        // Collect 1) equations unaccounted for by the NLR; 2) variables that
        // participate in constraints unhandled by the NLR.
        unsigned index = 0;
        for ( const auto &e : _equations )
        {
            if ( !handledEquations.exists( index++ ) )
            {
                unhandledEquations.append( e );
                varsInUnhandledConstraints.insert( e.getParticipatingVariables() );
            }
        }

        for ( const auto &c : _plConstraints )
        {
            if ( !handledPLConstraints.exists( c ) )
                varsInUnhandledConstraints.insert( c->getParticipatingVariables() );
        }

        for ( const auto &c : _nlConstraints )
        {
            if ( !handledNLConstraints.exists( c ) )
                varsInUnhandledConstraints.insert( c->getParticipatingVariables() );
        }

        INPUT_QUERY_LOG(
            Stringf( "successful. Constructed %u layers with %u neurons (out of %u)."
                     " %u out of %u equations, %u out of %u piecewise-linear constraints,"
                     " %u out of %u nonlinear constraints accounted for.",
                     newLayerIndex,
                     count,
                     getNumberOfVariables(),
                     handledEquations.size(),
                     _equations.size(),
                     handledPLConstraints.size(),
                     _plConstraints.size(),
                     handledNLConstraints.size(),
                     _nlConstraints.size() )
                .ascii() );

        _networkLevelReasoner = nlr;
    }
    else
    {
        INPUT_QUERY_LOG( "unsuccessful\n" );
        delete nlr;
    }

    return success;
}

void Query::mergeConsecutiveWeightedSumLayers( const List<Equation> &unhandledEquations,
                                               const Set<unsigned> &varsInUnhandledConstraints,
                                               Map<unsigned, LinearExpression> &eliminatedNeurons )
{
    /*
      Merge consecutive weighted sum layers
    */
    if ( _networkLevelReasoner )
    {
        INPUT_QUERY_LOG( "Attempting to merge consecutive weighted sum layers..." );
        unsigned numberOfMergedLayers = _networkLevelReasoner->mergeConsecutiveWSLayers(
            _lowerBounds, _upperBounds, varsInUnhandledConstraints, eliminatedNeurons );

        if ( numberOfMergedLayers > 0 )
        {
            // Re-encode the affine connections if there are merged layers
            _equations = unhandledEquations;
            _networkLevelReasoner->encodeAffineLayers( *this );
        }

        INPUT_QUERY_LOG( "Attempting to merge consecutive weighted sum layers - done" );
    }
}

bool Query::constructWeighedSumLayer( NLR::NetworkLevelReasoner *nlr,
                                      Map<unsigned, unsigned> &handledVariableToLayer,
                                      unsigned newLayerIndex,
                                      Set<unsigned> &handledEquations )
{
    INPUT_QUERY_LOG( "Attempting to construct weightedSumLayer..." );
    struct NeuronInformation
    {
    public:
        NeuronInformation( unsigned variable, unsigned neuron, const Equation *eq )
            : _variable( variable )
            , _neuron( neuron )
            , _eq( eq )
        {
        }

        NeuronInformation()
            : _eq( NULL )
        {
        }

        unsigned _variable;
        unsigned _neuron;
        const Equation *_eq;
    };

    List<NeuronInformation> newNeurons;

    // Look for equations where all variables except one have already been handled
    const List<Equation> &equations = getEquations();
    unsigned index = 0;
    for ( const auto &eq : equations )
    {
        if ( handledEquations.exists( index++ ) )
            continue;

        // Only consider equalities
        if ( eq._type != Equation::EQ )
            continue;

        List<unsigned> eqVariables = eq.getListParticipatingVariables();
        auto it = eqVariables.begin();
        while ( it != eqVariables.end() )
        {
            if ( handledVariableToLayer.exists( *it ) )
                it = eqVariables.erase( it );
            else
                ++it;
        }

        if ( eqVariables.size() == 1 )
        {
            // Add the surviving variable to the new layer
            newNeurons.append( NeuronInformation( *eqVariables.begin(), newNeurons.size(), &eq ) );
            handledEquations.insert( index - 1 );
        }
    }

    // No neurons found for the new layer
    if ( newNeurons.empty() )
    {
        INPUT_QUERY_LOG( "\tFailed!" );
        return false;
    }

    nlr->addLayer( newLayerIndex, NLR::Layer::WEIGHTED_SUM, newNeurons.size() );

    NLR::Layer *layer = nlr->getLayer( newLayerIndex );
    for ( const auto &newNeuron : newNeurons )
    {
        handledVariableToLayer[newNeuron._variable] = newLayerIndex;

        layer->setLb( newNeuron._neuron,
                      _lowerBounds.exists( newNeuron._variable ) ? _lowerBounds[newNeuron._variable]
                                                                 : FloatUtils::negativeInfinity() );
        layer->setUb( newNeuron._neuron,
                      _upperBounds.exists( newNeuron._variable ) ? _upperBounds[newNeuron._variable]
                                                                 : FloatUtils::infinity() );

        // Add the new neuron
        nlr->setNeuronVariable( NLR::NeuronIndex( newLayerIndex, newNeuron._neuron ),
                                newNeuron._variable );

        /*
          We assume equations have the form

          2x1 + 3x2 - y = 5

          Where y is our weighted sum variable. If y's coefficient is
          not -1, we make it -1 by multiplying everything else
        */

        ASSERT( !FloatUtils::isZero( newNeuron._eq->getCoefficient( newNeuron._variable ) ) );
        double factor = -1.0 / newNeuron._eq->getCoefficient( newNeuron._variable );

        // Bias
        nlr->setBias( newLayerIndex, newNeuron._neuron, factor * -newNeuron._eq->_scalar );

        // Weighted sum
        for ( const auto &addend : newNeuron._eq->_addends )
        {
            if ( addend._variable == newNeuron._variable )
                continue;

            unsigned sourceLayer = handledVariableToLayer[addend._variable];
            unsigned sourceNeuron =
                nlr->getLayer( sourceLayer )->variableToNeuron( addend._variable );

            // Mark the layer dependency
            nlr->addLayerDependency( sourceLayer, newLayerIndex );

            nlr->setWeight( sourceLayer,
                            sourceNeuron,
                            newLayerIndex,
                            newNeuron._neuron,
                            factor * addend._coefficient );
        }
    }

    INPUT_QUERY_LOG( "\tSuccessful!" );
    return true;
}

bool Query::constructReluLayer( NLR::NetworkLevelReasoner *nlr,
                                Map<unsigned, unsigned> &handledVariableToLayer,
                                unsigned newLayerIndex,
                                Set<PiecewiseLinearConstraint *> &handledPLConstraints )
{
    INPUT_QUERY_LOG( "Attempting to construct ReluLayer..." );
    struct NeuronInformation
    {
    public:
        NeuronInformation( unsigned variable, unsigned neuron, unsigned sourceVariable )
            : _variable( variable )
            , _neuron( neuron )
            , _sourceVariable( sourceVariable )
        {
        }

        unsigned _variable;
        unsigned _neuron;
        unsigned _sourceVariable;
    };

    List<NeuronInformation> newNeurons;

    // Look for ReLUs where all b variables have already been handled
    const List<PiecewiseLinearConstraint *> &plConstraints = getPiecewiseLinearConstraints();

    unsigned currentSourceLayer = 0;
    for ( const auto &plc : plConstraints )
    {
        if ( handledPLConstraints.exists( plc ) )
            continue;

        // Only consider ReLUs
        if ( plc->getType() != RELU )
            continue;

        const ReluConstraint *relu = (const ReluConstraint *)plc;

        // Has the b variable been handled?
        unsigned b = relu->getB();
        if ( !handledVariableToLayer.exists( b ) ||
             ( _ensureSameSourceLayerInNLR && !newNeurons.empty() &&
               handledVariableToLayer[b] != currentSourceLayer ) )
            continue;

        // If the f variable has also been handled, ignore this constraint
        unsigned f = relu->getF();
        if ( handledVariableToLayer.exists( f ) )
            continue;

        // B has been handled, f hasn't. Add f
        if ( _ensureSameSourceLayerInNLR && newNeurons.empty() )
            currentSourceLayer = handledVariableToLayer[b];
        newNeurons.append( NeuronInformation( f, newNeurons.size(), b ) );
        nlr->addConstraintInTopologicalOrder( plc );
        handledPLConstraints.insert( plc );
    }

    // No neurons found for the new layer
    if ( newNeurons.empty() )
    {
        INPUT_QUERY_LOG( "\tFailed!" );
        return false;
    }

    nlr->addLayer( newLayerIndex, NLR::Layer::RELU, newNeurons.size() );

    NLR::Layer *layer = nlr->getLayer( newLayerIndex );
    for ( const auto &newNeuron : newNeurons )
    {
        handledVariableToLayer[newNeuron._variable] = newLayerIndex;

        layer->setLb( newNeuron._neuron,
                      _lowerBounds.exists( newNeuron._variable ) ? _lowerBounds[newNeuron._variable]
                                                                 : FloatUtils::negativeInfinity() );
        layer->setUb( newNeuron._neuron,
                      _upperBounds.exists( newNeuron._variable ) ? _upperBounds[newNeuron._variable]
                                                                 : FloatUtils::infinity() );

        unsigned sourceLayer = handledVariableToLayer[newNeuron._sourceVariable];
        unsigned sourceNeuron =
            nlr->getLayer( sourceLayer )->variableToNeuron( newNeuron._sourceVariable );

        // Mark the layer dependency
        nlr->addLayerDependency( sourceLayer, newLayerIndex );

        // Add the new neuron
        nlr->setNeuronVariable( NLR::NeuronIndex( newLayerIndex, newNeuron._neuron ),
                                newNeuron._variable );

        // Mark the activation connection
        nlr->addActivationSource( sourceLayer, sourceNeuron, newLayerIndex, newNeuron._neuron );
    }

    INPUT_QUERY_LOG( "\tSuccessful!" );
    return true;
}

bool Query::constructLeakyReluLayer( NLR::NetworkLevelReasoner *nlr,
                                     Map<unsigned, unsigned> &handledVariableToLayer,
                                     unsigned newLayerIndex,
                                     Set<PiecewiseLinearConstraint *> &handledPLConstraints )
{
    INPUT_QUERY_LOG( "Attempting to construct LeakyReLULayer..." );
    struct NeuronInformation
    {
    public:
        NeuronInformation( unsigned variable, unsigned neuron, unsigned sourceVariable )
            : _variable( variable )
            , _neuron( neuron )
            , _sourceVariable( sourceVariable )
        {
        }

        unsigned _variable;
        unsigned _neuron;
        unsigned _sourceVariable;
    };

    List<NeuronInformation> newNeurons;

    // Look for LeakyReLUs where all b variables have already been handled
    const List<PiecewiseLinearConstraint *> &plConstraints = getPiecewiseLinearConstraints();

    unsigned currentSourceLayer = 0;
    double alpha = -1;
    for ( const auto &plc : plConstraints )
    {
        if ( handledPLConstraints.exists( plc ) )
            continue;

        // Only consider Leaky ReLUs
        if ( plc->getType() != LEAKY_RELU )
            continue;

        const LeakyReluConstraint *leakyRelu = (const LeakyReluConstraint *)plc;

        // Has the b variable been handled?
        unsigned b = leakyRelu->getB();
        if ( !handledVariableToLayer.exists( b ) ||
             ( _ensureSameSourceLayerInNLR && !newNeurons.empty() &&
               handledVariableToLayer[b] != currentSourceLayer ) )
            continue;

        // Is the slope uniform?
        double alphaTemp = leakyRelu->getSlope();
        if ( alpha != -1 && alpha != alphaTemp )
        {
            continue;
        }

        // If the f variable has also been handled, ignore this constraint
        unsigned f = leakyRelu->getF();
        if ( handledVariableToLayer.exists( f ) )
            continue;
        // B has been handled, f hasn't. Add f
        if ( _ensureSameSourceLayerInNLR && newNeurons.empty() )
            currentSourceLayer = handledVariableToLayer[b];
        if ( alpha == -1 )
            alpha = alphaTemp;
        newNeurons.append( NeuronInformation( f, newNeurons.size(), b ) );
        nlr->addConstraintInTopologicalOrder( plc );
        handledPLConstraints.insert( plc );
    }

    // No neurons found for the new layer
    if ( newNeurons.empty() )
    {
        INPUT_QUERY_LOG( "\tFailed!" );
        return false;
    }

    nlr->addLayer( newLayerIndex, NLR::Layer::LEAKY_RELU, newNeurons.size() );

    NLR::Layer *layer = nlr->getLayer( newLayerIndex );
    layer->setAlpha( alpha );
    for ( const auto &newNeuron : newNeurons )
    {
        handledVariableToLayer[newNeuron._variable] = newLayerIndex;

        layer->setLb( newNeuron._neuron,
                      _lowerBounds.exists( newNeuron._variable ) ? _lowerBounds[newNeuron._variable]
                                                                 : FloatUtils::negativeInfinity() );
        layer->setUb( newNeuron._neuron,
                      _upperBounds.exists( newNeuron._variable ) ? _upperBounds[newNeuron._variable]
                                                                 : FloatUtils::infinity() );

        unsigned sourceLayer = handledVariableToLayer[newNeuron._sourceVariable];
        unsigned sourceNeuron =
            nlr->getLayer( sourceLayer )->variableToNeuron( newNeuron._sourceVariable );

        // Mark the layer dependency
        nlr->addLayerDependency( sourceLayer, newLayerIndex );

        // Add the new neuron
        nlr->setNeuronVariable( NLR::NeuronIndex( newLayerIndex, newNeuron._neuron ),
                                newNeuron._variable );

        // Mark the activation connection
        nlr->addActivationSource( sourceLayer, sourceNeuron, newLayerIndex, newNeuron._neuron );
    }

    INPUT_QUERY_LOG( "\tSuccessful!" );
    return true;
}

bool Query::constructRoundLayer( NLR::NetworkLevelReasoner *nlr,
                                 Map<unsigned, unsigned> &handledVariableToLayer,
                                 unsigned newLayerIndex,
                                 Set<NonlinearConstraint *> &handledNLConstraints )
{
    INPUT_QUERY_LOG( "Attempting to construct RoundLayer..." );
    struct NeuronInformation
    {
    public:
        NeuronInformation( unsigned variable, unsigned neuron, unsigned sourceVariable )
            : _variable( variable )
            , _neuron( neuron )
            , _sourceVariable( sourceVariable )
        {
        }

        unsigned _variable;
        unsigned _neuron;
        unsigned _sourceVariable;
    };

    List<NeuronInformation> newNeurons;

    // Look for ReLUs where all b variables have already been handled
    const List<NonlinearConstraint *> &nlConstraints = getNonlinearConstraints();

    unsigned currentSourceLayer = 0;
    for ( const auto &nlc : nlConstraints )
    {
        if ( handledNLConstraints.exists( nlc ) )
            continue;

        // Only consider Rounds
        if ( nlc->getType() != ROUND )
            continue;

        const RoundConstraint *round = (const RoundConstraint *)nlc;

        // Has the b variable been handled?
        unsigned b = round->getB();
        if ( !handledVariableToLayer.exists( b ) ||
             ( _ensureSameSourceLayerInNLR && !newNeurons.empty() &&
               handledVariableToLayer[b] != currentSourceLayer ) )
            continue;

        // If the f variable has also been handled, ignore this constraint
        unsigned f = round->getF();
        if ( handledVariableToLayer.exists( f ) )
            continue;

        // B has been handled, f hasn't. Add f
        if ( _ensureSameSourceLayerInNLR && newNeurons.empty() )
            currentSourceLayer = handledVariableToLayer[b];
        newNeurons.append( NeuronInformation( f, newNeurons.size(), b ) );
        handledNLConstraints.insert( nlc );
    }

    // No neurons found for the new layer
    if ( newNeurons.empty() )
    {
        INPUT_QUERY_LOG( "\tFailed!" );
        return false;
    }

    nlr->addLayer( newLayerIndex, NLR::Layer::ROUND, newNeurons.size() );

    NLR::Layer *layer = nlr->getLayer( newLayerIndex );
    for ( const auto &newNeuron : newNeurons )
    {
        handledVariableToLayer[newNeuron._variable] = newLayerIndex;

        layer->setLb( newNeuron._neuron,
                      _lowerBounds.exists( newNeuron._variable ) ? _lowerBounds[newNeuron._variable]
                                                                 : FloatUtils::negativeInfinity() );
        layer->setUb( newNeuron._neuron,
                      _upperBounds.exists( newNeuron._variable ) ? _upperBounds[newNeuron._variable]
                                                                 : FloatUtils::infinity() );

        unsigned sourceLayer = handledVariableToLayer[newNeuron._sourceVariable];
        unsigned sourceNeuron =
            nlr->getLayer( sourceLayer )->variableToNeuron( newNeuron._sourceVariable );

        // Mark the layer dependency
        nlr->addLayerDependency( sourceLayer, newLayerIndex );

        // Add the new neuron
        nlr->setNeuronVariable( NLR::NeuronIndex( newLayerIndex, newNeuron._neuron ),
                                newNeuron._variable );

        // Mark the activation connection
        nlr->addActivationSource( sourceLayer, sourceNeuron, newLayerIndex, newNeuron._neuron );
    }

    INPUT_QUERY_LOG( "\tSuccessful!" );
    return true;
}

bool Query::constructSigmoidLayer( NLR::NetworkLevelReasoner *nlr,
                                   Map<unsigned, unsigned> &handledVariableToLayer,
                                   unsigned newLayerIndex,
                                   Set<NonlinearConstraint *> &handledNLConstraints )
{
    INPUT_QUERY_LOG( "Attempting to construct SigmoidLayer..." );
    struct NeuronInformation
    {
    public:
        NeuronInformation( unsigned variable, unsigned neuron, unsigned sourceVariable )
            : _variable( variable )
            , _neuron( neuron )
            , _sourceVariable( sourceVariable )
        {
        }

        unsigned _variable;
        unsigned _neuron;
        unsigned _sourceVariable;
    };

    List<NeuronInformation> newNeurons;

    // Look for Sigmoids where all b variables have already been handled
    const List<NonlinearConstraint *> &nlConstraints = getNonlinearConstraints();

    unsigned currentSourceLayer = 0;
    for ( const auto &nlc : nlConstraints )
    {
        if ( handledNLConstraints.exists( nlc ) )
            continue;

        // Only consider Sigmoids
        if ( nlc->getType() != SIGMOID )
            continue;

        const SigmoidConstraint *sigmoid = (const SigmoidConstraint *)nlc;

        // Has the b variable been handled?
        unsigned b = sigmoid->getB();
        if ( !handledVariableToLayer.exists( b ) ||
             ( _ensureSameSourceLayerInNLR && !newNeurons.empty() &&
               handledVariableToLayer[b] != currentSourceLayer ) )
            continue;

        // If the f variable has also been handled, ignore this constraint
        unsigned f = sigmoid->getF();
        if ( handledVariableToLayer.exists( f ) )
            continue;

        // B has been handled, f hasn't. Add f
        if ( _ensureSameSourceLayerInNLR && newNeurons.empty() )
            currentSourceLayer = handledVariableToLayer[b];
        newNeurons.append( NeuronInformation( f, newNeurons.size(), b ) );

        handledNLConstraints.insert( nlc );
    }

    // No neurons found for the new layer
    if ( newNeurons.empty() )
    {
        INPUT_QUERY_LOG( "\tFailed!" );
        return false;
    }

    nlr->addLayer( newLayerIndex, NLR::Layer::SIGMOID, newNeurons.size() );

    NLR::Layer *layer = nlr->getLayer( newLayerIndex );
    for ( const auto &newNeuron : newNeurons )
    {
        handledVariableToLayer[newNeuron._variable] = newLayerIndex;

        layer->setLb( newNeuron._neuron,
                      _lowerBounds.exists( newNeuron._variable ) ? _lowerBounds[newNeuron._variable]
                                                                 : FloatUtils::negativeInfinity() );
        layer->setUb( newNeuron._neuron,
                      _upperBounds.exists( newNeuron._variable ) ? _upperBounds[newNeuron._variable]
                                                                 : FloatUtils::infinity() );

        unsigned sourceLayer = handledVariableToLayer[newNeuron._sourceVariable];
        unsigned sourceNeuron =
            nlr->getLayer( sourceLayer )->variableToNeuron( newNeuron._sourceVariable );

        // Mark the layer dependency
        nlr->addLayerDependency( sourceLayer, newLayerIndex );

        // Add the new neuron
        nlr->setNeuronVariable( NLR::NeuronIndex( newLayerIndex, newNeuron._neuron ),
                                newNeuron._variable );

        // Mark the activation connection
        nlr->addActivationSource( sourceLayer, sourceNeuron, newLayerIndex, newNeuron._neuron );
    }

    INPUT_QUERY_LOG( "\tSuccessful!" );
    return true;
}

bool Query::constructAbsoluteValueLayer( NLR::NetworkLevelReasoner *nlr,
                                         Map<unsigned, unsigned> &handledVariableToLayer,
                                         unsigned newLayerIndex,
                                         Set<PiecewiseLinearConstraint *> &handledPLConstraints )
{
    INPUT_QUERY_LOG( "Attempting to construct AbsoluteValueLayer..." );
    struct NeuronInformation
    {
    public:
        NeuronInformation( unsigned variable, unsigned neuron, unsigned sourceVariable )
            : _variable( variable )
            , _neuron( neuron )
            , _sourceVariable( sourceVariable )
        {
        }

        unsigned _variable;
        unsigned _neuron;
        unsigned _sourceVariable;
    };

    List<NeuronInformation> newNeurons;

    // Look for ABSOLUTE_VALUEs where all b variables have already been handled
    const List<PiecewiseLinearConstraint *> &plConstraints = getPiecewiseLinearConstraints();

    unsigned currentSourceLayer = 0;
    for ( const auto &plc : plConstraints )
    {
        if ( handledPLConstraints.exists( plc ) )
            continue;

        // Only consider ABSOLUTE_VALUE
        if ( plc->getType() != ABSOLUTE_VALUE )
            continue;

        const AbsoluteValueConstraint *abs = (const AbsoluteValueConstraint *)plc;

        // Has the b variable been handled?
        unsigned b = abs->getB();
        if ( !handledVariableToLayer.exists( b ) ||
             ( _ensureSameSourceLayerInNLR && !newNeurons.empty() &&
               handledVariableToLayer[b] != currentSourceLayer ) )
            continue;

        // If the f variable has also been handled, ignore this constraint
        unsigned f = abs->getF();
        if ( handledVariableToLayer.exists( f ) )
            continue;

        // B has been handled, f hasn't. Add f
        if ( _ensureSameSourceLayerInNLR && newNeurons.empty() )
            currentSourceLayer = handledVariableToLayer[b];
        newNeurons.append( NeuronInformation( f, newNeurons.size(), b ) );
        nlr->addConstraintInTopologicalOrder( plc );
        handledPLConstraints.insert( plc );
    }

    // No neurons found for the new layer
    if ( newNeurons.empty() )
    {
        INPUT_QUERY_LOG( "\tFailed!" );
        return false;
    }

    nlr->addLayer( newLayerIndex, NLR::Layer::ABSOLUTE_VALUE, newNeurons.size() );

    NLR::Layer *layer = nlr->getLayer( newLayerIndex );
    for ( const auto &newNeuron : newNeurons )
    {
        handledVariableToLayer[newNeuron._variable] = newLayerIndex;

        layer->setLb( newNeuron._neuron,
                      _lowerBounds.exists( newNeuron._variable ) ? _lowerBounds[newNeuron._variable]
                                                                 : FloatUtils::negativeInfinity() );
        layer->setUb( newNeuron._neuron,
                      _upperBounds.exists( newNeuron._variable ) ? _upperBounds[newNeuron._variable]
                                                                 : FloatUtils::infinity() );

        unsigned sourceLayer = handledVariableToLayer[newNeuron._sourceVariable];
        unsigned sourceNeuron =
            nlr->getLayer( sourceLayer )->variableToNeuron( newNeuron._sourceVariable );

        // Mark the layer dependency
        nlr->addLayerDependency( sourceLayer, newLayerIndex );

        // Add the new neuron
        nlr->setNeuronVariable( NLR::NeuronIndex( newLayerIndex, newNeuron._neuron ),
                                newNeuron._variable );

        // Mark the activation connection
        nlr->addActivationSource( sourceLayer, sourceNeuron, newLayerIndex, newNeuron._neuron );
    }

    INPUT_QUERY_LOG( "\tSuccessful!" );
    return true;
}

bool Query::constructSignLayer( NLR::NetworkLevelReasoner *nlr,
                                Map<unsigned, unsigned> &handledVariableToLayer,
                                unsigned newLayerIndex,
                                Set<PiecewiseLinearConstraint *> &handledPLConstraints )
{
    INPUT_QUERY_LOG( "Attempting to construct SignLayer..." );
    struct NeuronInformation
    {
    public:
        NeuronInformation( unsigned variable, unsigned neuron, unsigned sourceVariable )
            : _variable( variable )
            , _neuron( neuron )
            , _sourceVariable( sourceVariable )
        {
        }

        unsigned _variable;
        unsigned _neuron;
        unsigned _sourceVariable;
    };

    List<NeuronInformation> newNeurons;

    // Look for Signs where the b variables have already been handled
    const List<PiecewiseLinearConstraint *> &plConstraints = getPiecewiseLinearConstraints();

    unsigned currentSourceLayer = 0;
    for ( const auto &plc : plConstraints )
    {
        if ( handledPLConstraints.exists( plc ) )
            continue;

        // Only consider Signs
        if ( plc->getType() != SIGN )
            continue;

        const SignConstraint *sign = (const SignConstraint *)plc;

        // Has the b variable been handled?
        unsigned b = sign->getB();
        if ( !handledVariableToLayer.exists( b ) ||
             ( _ensureSameSourceLayerInNLR && !newNeurons.empty() &&
               handledVariableToLayer[b] != currentSourceLayer ) )
            continue;

        // If the f variable has also been handled, ignore this constraint
        unsigned f = sign->getF();
        if ( handledVariableToLayer.exists( f ) )
            continue;

        // B has been handled, f hasn't. Add f
        if ( _ensureSameSourceLayerInNLR && newNeurons.empty() )
            currentSourceLayer = handledVariableToLayer[b];
        newNeurons.append( NeuronInformation( f, newNeurons.size(), b ) );
        nlr->addConstraintInTopologicalOrder( plc );
        handledPLConstraints.insert( plc );
    }

    // No neurons found for the new layer
    if ( newNeurons.empty() )
    {
        INPUT_QUERY_LOG( "\tFailed!" );
        return false;
    }

    nlr->addLayer( newLayerIndex, NLR::Layer::SIGN, newNeurons.size() );

    NLR::Layer *layer = nlr->getLayer( newLayerIndex );
    for ( const auto &newNeuron : newNeurons )
    {
        handledVariableToLayer[newNeuron._variable] = newLayerIndex;

        layer->setLb( newNeuron._neuron,
                      _lowerBounds.exists( newNeuron._variable ) ? _lowerBounds[newNeuron._variable]
                                                                 : FloatUtils::negativeInfinity() );
        layer->setUb( newNeuron._neuron,
                      _upperBounds.exists( newNeuron._variable ) ? _upperBounds[newNeuron._variable]
                                                                 : FloatUtils::infinity() );

        unsigned sourceLayer = handledVariableToLayer[newNeuron._sourceVariable];
        unsigned sourceNeuron =
            nlr->getLayer( sourceLayer )->variableToNeuron( newNeuron._sourceVariable );

        // Mark the layer dependency
        nlr->addLayerDependency( sourceLayer, newLayerIndex );

        // Add the new neuron
        nlr->setNeuronVariable( NLR::NeuronIndex( newLayerIndex, newNeuron._neuron ),
                                newNeuron._variable );

        // Mark the activation connection
        nlr->addActivationSource( sourceLayer, sourceNeuron, newLayerIndex, newNeuron._neuron );
    }

    INPUT_QUERY_LOG( "\tSuccessful!" );
    return true;
}

bool Query::constructMaxLayer( NLR::NetworkLevelReasoner *nlr,
                               Map<unsigned, unsigned> &handledVariableToLayer,
                               unsigned newLayerIndex,
                               Set<PiecewiseLinearConstraint *> &handledPLConstraints )
{
    INPUT_QUERY_LOG( "Attempting to construct MaxLayer..." );
    struct NeuronInformation
    {
    public:
        NeuronInformation( unsigned variable,
                           unsigned neuron,
                           const List<unsigned> &sourceVariables )
            : _variable( variable )
            , _neuron( neuron )
            , _sourceVariables( sourceVariables )
        {
        }

        unsigned _variable;
        unsigned _neuron;
        List<unsigned> _sourceVariables;
    };

    List<NeuronInformation> newNeurons;

    // Look for Maxes where all the element variables have already been handled
    const List<PiecewiseLinearConstraint *> &plConstraints = getPiecewiseLinearConstraints();

    unsigned currentSourceLayer = 0;
    for ( const auto &plc : plConstraints )
    {
        if ( handledPLConstraints.exists( plc ) )
            continue;

        // Only consider Max
        if ( plc->getType() != MAX )
            continue;

        const MaxConstraint *max = (const MaxConstraint *)plc;

        // Have all elements been handled?
        // Have all input variables been handled?
        bool missingInput = false;
        bool sourceLayerDiffers = false;
        for ( const auto &input : max->getElements() )
        {
            if ( !handledVariableToLayer.exists( input ) )
            {
                missingInput = true;
                break;
            }
            else if ( _ensureSameSourceLayerInNLR && newNeurons.size() &&
                      handledVariableToLayer[input] != currentSourceLayer )
            {
                sourceLayerDiffers = true;
                break;
            }
        }

        if ( missingInput || sourceLayerDiffers )
            continue;

        // If the f variable has also been handled, ignore this constraint
        unsigned f = max->getF();
        if ( handledVariableToLayer.exists( f ) )
            continue;

        // Elements have been handled, f hasn't. Add f
        if ( _ensureSameSourceLayerInNLR && newNeurons.empty() )
            currentSourceLayer = handledVariableToLayer[*max->getElements().begin()];
        newNeurons.append( NeuronInformation( f, newNeurons.size(), max->getElements() ) );
        nlr->addConstraintInTopologicalOrder( plc );
        handledPLConstraints.insert( plc );
    }

    // No neurons found for the new layer
    if ( newNeurons.empty() )
    {
        INPUT_QUERY_LOG( "\tFailed!" );
        return false;
    }

    nlr->addLayer( newLayerIndex, NLR::Layer::MAX, newNeurons.size() );

    NLR::Layer *layer = nlr->getLayer( newLayerIndex );
    for ( const auto &newNeuron : newNeurons )
    {
        handledVariableToLayer[newNeuron._variable] = newLayerIndex;

        layer->setLb( newNeuron._neuron,
                      _lowerBounds.exists( newNeuron._variable ) ? _lowerBounds[newNeuron._variable]
                                                                 : FloatUtils::negativeInfinity() );
        layer->setUb( newNeuron._neuron,
                      _upperBounds.exists( newNeuron._variable ) ? _upperBounds[newNeuron._variable]
                                                                 : FloatUtils::infinity() );

        // Add the new neuron
        nlr->setNeuronVariable( NLR::NeuronIndex( newLayerIndex, newNeuron._neuron ),
                                newNeuron._variable );

        for ( const auto &sourceVariable : newNeuron._sourceVariables )
        {
            unsigned sourceLayer = handledVariableToLayer[sourceVariable];
            unsigned sourceNeuron =
                nlr->getLayer( sourceLayer )->variableToNeuron( sourceVariable );

            // Mark the layer dependency
            nlr->addLayerDependency( sourceLayer, newLayerIndex );

            // Mark the activation connection
            nlr->addActivationSource( sourceLayer, sourceNeuron, newLayerIndex, newNeuron._neuron );
        }
    }

    INPUT_QUERY_LOG( "\tSuccessful!" );
    return true;
}

bool Query::constructBilinearLayer( NLR::NetworkLevelReasoner *nlr,
                                    Map<unsigned, unsigned> &handledVariableToLayer,
                                    unsigned newLayerIndex,
                                    Set<NonlinearConstraint *> &handledNLConstraints )
{
    INPUT_QUERY_LOG( "Attempting to construct BilinearLayer..." );
    struct NeuronInformation
    {
    public:
        NeuronInformation( unsigned variable,
                           unsigned neuron,
                           const Vector<unsigned> &sourceVariables )
            : _variable( variable )
            , _neuron( neuron )
            , _sourceVariables( sourceVariables )
        {
        }

        unsigned _variable;
        unsigned _neuron;
        Vector<unsigned> _sourceVariables;
    };

    List<NeuronInformation> newNeurons;

    // Look for Bilinear constaints where all the element variables have already been handled
    const List<NonlinearConstraint *> &nlConstraints = getNonlinearConstraints();

    for ( const auto &nlc : nlConstraints )
    {
        if ( handledNLConstraints.exists( nlc ) )
            continue;

        // Only consider bilinear
        if ( nlc->getType() != BILINEAR )
            continue;

        const BilinearConstraint *bilinear = (const BilinearConstraint *)nlc;

        // Have all elements been handled?
        bool missingElement = false;
        for ( const auto &element : bilinear->getBs() )
        {
            if ( !handledVariableToLayer.exists( element ) )
            {
                missingElement = true;
                break;
            }
        }

        if ( missingElement )
            continue;

        // If the f variable has also been handled, ignore this constraint
        unsigned f = bilinear->getF();
        if ( handledVariableToLayer.exists( f ) )
            continue;

        // Elements have been handled, f hasn't. Add f
        newNeurons.append( NeuronInformation( f, newNeurons.size(), bilinear->getBs() ) );

        handledNLConstraints.insert( nlc );
    }

    // No neurons found for the new layer
    if ( newNeurons.empty() )
    {
        INPUT_QUERY_LOG( "\tFailed!" );
        return false;
    }

    nlr->addLayer( newLayerIndex, NLR::Layer::BILINEAR, newNeurons.size() );

    NLR::Layer *layer = nlr->getLayer( newLayerIndex );
    for ( const auto &newNeuron : newNeurons )
    {
        handledVariableToLayer[newNeuron._variable] = newLayerIndex;

        layer->setLb( newNeuron._neuron,
                      _lowerBounds.exists( newNeuron._variable ) ? _lowerBounds[newNeuron._variable]
                                                                 : FloatUtils::negativeInfinity() );
        layer->setUb( newNeuron._neuron,
                      _upperBounds.exists( newNeuron._variable ) ? _upperBounds[newNeuron._variable]
                                                                 : FloatUtils::infinity() );

        // Add the new neuron
        nlr->setNeuronVariable( NLR::NeuronIndex( newLayerIndex, newNeuron._neuron ),
                                newNeuron._variable );

        for ( const auto &sourceVariable : newNeuron._sourceVariables )
        {
            unsigned sourceLayer = handledVariableToLayer[sourceVariable];
            unsigned sourceNeuron =
                nlr->getLayer( sourceLayer )->variableToNeuron( sourceVariable );

            // Mark the layer dependency
            nlr->addLayerDependency( sourceLayer, newLayerIndex );

            // Mark the activation connection
            nlr->addActivationSource( sourceLayer, sourceNeuron, newLayerIndex, newNeuron._neuron );
        }
    }

    INPUT_QUERY_LOG( "\tSuccessful!" );
    return true;
}

bool Query::constructSoftmaxLayer( NLR::NetworkLevelReasoner *nlr,
                                   Map<unsigned, unsigned> &handledVariableToLayer,
                                   unsigned newLayerIndex,
                                   Set<NonlinearConstraint *> &handledNLConstraints )
{
    INPUT_QUERY_LOG( "Attempting to construct SoftmaxLayer..." );
    struct NeuronInformation
    {
    public:
        NeuronInformation( unsigned variable,
                           unsigned neuron,
                           const Vector<unsigned> &sourceVariables )
            : _variable( variable )
            , _neuron( neuron )
            , _sourceVariables( sourceVariables )
        {
        }

        unsigned _variable;
        unsigned _neuron;
        Vector<unsigned> _sourceVariables;
    };

    List<NeuronInformation> newNeurons;

    // Look for Softmaxes where all the element variables have already been handled
    const List<NonlinearConstraint *> &nlConstraints = getNonlinearConstraints();

    unsigned currentSourceLayer = 0;
    for ( const auto &nlc : nlConstraints )
    {
        if ( handledNLConstraints.exists( nlc ) )
            continue;

        // Only consider Softmax
        if ( nlc->getType() != SOFTMAX )
            continue;

        const SoftmaxConstraint *softmax = (const SoftmaxConstraint *)nlc;

        // Have all input variables been handled?
        bool missingInput = false;
        bool sourceLayerDiffers = false;
        for ( const auto &input : softmax->getInputs() )
        {
            if ( !handledVariableToLayer.exists( input ) )
            {
                missingInput = true;
                break;
            }
            else if ( _ensureSameSourceLayerInNLR && newNeurons.size() &&
                      handledVariableToLayer[input] != currentSourceLayer )
            {
                sourceLayerDiffers = true;
                break;
            }
        }

        if ( missingInput || sourceLayerDiffers )
            continue;

        // If any output has also been handled, ignore this constraint
        bool outputHandled = false;
        for ( const auto &output : softmax->getOutputs() )
        {
            if ( handledVariableToLayer.exists( output ) )
            {
                outputHandled = true;
                break;
            }
        }

        if ( outputHandled )
            continue;

        // inputs have been handled, outputs haven't. Add the outputs
        // We want the following criteron to hold:
        // if we sort the inputs by their neuron index and sort the outputs
        // by their neuron index, each output would correspond to the correct input

        Map<unsigned, unsigned> neuronToVariable;
        Vector<unsigned> neurons;
        currentSourceLayer = handledVariableToLayer[*softmax->getInputs().begin()];
        NLR::Layer *layer = nlr->getLayer( currentSourceLayer );
        for ( const auto &input : softmax->getInputs() )
        {
            unsigned neuron = layer->variableToNeuron( input );
            neuronToVariable[neuron] = input;
            neurons.append( neuron );
        }
        neurons.sort();

        for ( unsigned neuron : neurons )
        {
            unsigned output = softmax->getOutput( neuronToVariable[neuron] );
            newNeurons.append(
                NeuronInformation( output, newNeurons.size(), softmax->getInputs() ) );
        }
        handledNLConstraints.insert( nlc );
    }

    // No neurons found for the new layer
    if ( newNeurons.empty() )
    {
        INPUT_QUERY_LOG( "\tFailed!" );
        return false;
    }

    nlr->addLayer( newLayerIndex, NLR::Layer::SOFTMAX, newNeurons.size() );

    NLR::Layer *layer = nlr->getLayer( newLayerIndex );
    for ( const auto &newNeuron : newNeurons )
    {
        handledVariableToLayer[newNeuron._variable] = newLayerIndex;

        layer->setLb( newNeuron._neuron,
                      _lowerBounds.exists( newNeuron._variable ) ? _lowerBounds[newNeuron._variable]
                                                                 : FloatUtils::negativeInfinity() );
        layer->setUb( newNeuron._neuron,
                      _upperBounds.exists( newNeuron._variable ) ? _upperBounds[newNeuron._variable]
                                                                 : FloatUtils::infinity() );

        // Add the new neuron
        nlr->setNeuronVariable( NLR::NeuronIndex( newLayerIndex, newNeuron._neuron ),
                                newNeuron._variable );

        for ( const auto &sourceVariable : newNeuron._sourceVariables )
        {
            unsigned sourceLayer = handledVariableToLayer[sourceVariable];
            unsigned sourceNeuron =
                nlr->getLayer( sourceLayer )->variableToNeuron( sourceVariable );

            // Mark the layer dependency
            nlr->addLayerDependency( sourceLayer, newLayerIndex );

            // Mark the activation connection
            nlr->addActivationSource( sourceLayer, sourceNeuron, newLayerIndex, newNeuron._neuron );
        }
    }

    INPUT_QUERY_LOG( "\tSuccessful!" );
    return true;
}
