/*********************                                                        */
/*! \file InputQuery.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Christopher Lazarus, Shantanu Thakoor
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#include "AutoFile.h"
#include "Debug.h"
#include "FloatUtils.h"
#include "InputQuery.h"
#include "MStringf.h"
#include "MarabouError.h"

#define INPUT_QUERY_LOG( x, ... ) LOG( GlobalConfiguration::INPUT_QUERY_LOGGING, "Preprocessor: %s\n", x )

InputQuery::InputQuery()
    : _networkLevelReasoner( NULL )
{
}

InputQuery::~InputQuery()
{
    freeConstraintsIfNeeded();
    if ( _networkLevelReasoner )
    {
        delete _networkLevelReasoner;
        _networkLevelReasoner = NULL;
    }
}

void InputQuery::setNumberOfVariables( unsigned numberOfVariables )
{
    _numberOfVariables = numberOfVariables;
}

void InputQuery::setLowerBound( unsigned variable, double bound )
{
    if ( variable >= _numberOfVariables )
    {
        throw MarabouError( MarabouError::VARIABLE_INDEX_OUT_OF_RANGE,
                             Stringf( "Variable = %u, number of variables = %u (setLowerBound)",
                                      variable, _numberOfVariables ).ascii() );
    }

    _lowerBounds[variable] = bound;
}

void InputQuery::setUpperBound( unsigned variable, double bound )
{
    if ( variable >= _numberOfVariables )
    {
        throw MarabouError( MarabouError::VARIABLE_INDEX_OUT_OF_RANGE,
                             Stringf( "Variable = %u, number of variables = %u (setUpperBound)",
                                      variable, _numberOfVariables ).ascii() );
    }

    _upperBounds[variable] = bound;
}

void InputQuery::addEquation( const Equation &equation )
{
    _equations.append( equation );
}

unsigned InputQuery::getNumberOfVariables() const
{
    return _numberOfVariables;
}

double InputQuery::getLowerBound( unsigned variable ) const
{
    if ( variable >= _numberOfVariables )
    {
        throw MarabouError( MarabouError::VARIABLE_INDEX_OUT_OF_RANGE,
                             Stringf( "Variable = %u, number of variables = %u (getLowerBound)",
                                      variable, _numberOfVariables ).ascii() );
    }

    if ( !_lowerBounds.exists( variable ) )
        return FloatUtils::negativeInfinity();

    return _lowerBounds.get( variable );
}

double InputQuery::getUpperBound( unsigned variable ) const
{
    if ( variable >= _numberOfVariables )
    {
        throw MarabouError( MarabouError::VARIABLE_INDEX_OUT_OF_RANGE,
                             Stringf( "Variable = %u, number of variables = %u (getUpperBound)",
                                      variable, _numberOfVariables ).ascii() );
    }

    if ( !_upperBounds.exists( variable ) )
        return FloatUtils::infinity();

    return _upperBounds.get( variable );
}

List<Equation> &InputQuery::getEquations()
{
    return _equations;
}

void InputQuery::removeEquationsByIndex( const Set<unsigned> indices )
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

const List<Equation> &InputQuery::getEquations() const
{
    return _equations;
}

void InputQuery::setSolutionValue( unsigned variable, double value )
{
    _solution[variable] = value;
}

double InputQuery::getSolutionValue( unsigned variable ) const
{
    if ( !_solution.exists( variable ) )
        throw MarabouError( MarabouError::VARIABLE_DOESNT_EXIST_IN_SOLUTION,
                             Stringf( "Variable: %u", variable ).ascii() );

    return _solution.get( variable );
}

void InputQuery::addPiecewiseLinearConstraint( PiecewiseLinearConstraint *constraint )
{
    _plConstraints.append( constraint );
}

List<PiecewiseLinearConstraint *> &InputQuery::getPiecewiseLinearConstraints()
{
    return _plConstraints;
}

const List<PiecewiseLinearConstraint *> &InputQuery::getPiecewiseLinearConstraints() const
{
    return _plConstraints;
}

unsigned InputQuery::countInfiniteBounds()
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

void InputQuery::mergeIdenticalVariables( unsigned v1, unsigned v2 )
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

    // TODO: update lower and upper bounds
}

void InputQuery::removeEquation( Equation e )
{
    _equations.erase( e );
}

InputQuery &InputQuery::operator=( const InputQuery &other )
{
    INPUT_QUERY_LOG( "Making a deep copy of the InputQuery..." );
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
    for ( const auto &constraint : other._plConstraints )
        _plConstraints.append( constraint->duplicateConstraint() );

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
    return *this;
}

InputQuery::InputQuery( const InputQuery &other )
    : _networkLevelReasoner( NULL )
{
    *this = other;
}

void InputQuery::freeConstraintsIfNeeded()
{
    for ( auto &it : _plConstraints )
        delete it;

    _plConstraints.clear();
}

const Map<unsigned, double> &InputQuery::getLowerBounds() const
{
    return _lowerBounds;
}

const Map<unsigned, double> &InputQuery::getUpperBounds() const
{
    return _upperBounds;
}

void InputQuery::storeDebuggingSolution( unsigned variable, double value )
{
    _debuggingSolution[variable] = value;
}

void InputQuery::saveQuery( const String &fileName )
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

    // Number of Constraints
    queryFile->write( Stringf( "%u", _plConstraints.size() ) );

    printf( "Number of variables: %u\n", _numberOfVariables );
    printf( "Number of lower bounds: %u\n", _lowerBounds.size() );
    printf( "Number of upper bounds: %u\n", _upperBounds.size() );
    printf( "Number of equations: %u\n", _equations.size() );
    printf( "Number of constraints: %u\n", _plConstraints.size() );

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

    // Constraints
    i = 0;
    for ( const auto &constraint : _plConstraints )
    {
        // Constraint number
        queryFile->write( Stringf( "\n%u,", i ) );
        queryFile->write( constraint->serializeToString() );
        ++i;
    }

    queryFile->close();
}

void InputQuery::markInputVariable( unsigned variable, unsigned inputIndex )
{
    _variableToInputIndex[variable] = inputIndex;
    _inputIndexToVariable[inputIndex] = variable;
}

void InputQuery::markOutputVariable( unsigned variable, unsigned outputIndex )
{
    _variableToOutputIndex[variable] = outputIndex;
    _outputIndexToVariable[outputIndex] = variable;
}

unsigned InputQuery::inputVariableByIndex( unsigned index ) const
{
    ASSERT( _inputIndexToVariable.exists( index ) );
    return _inputIndexToVariable.get( index );
}

unsigned InputQuery::outputVariableByIndex( unsigned index ) const
{
    ASSERT( _outputIndexToVariable.exists( index ) );
    return _outputIndexToVariable.get( index );
}

unsigned InputQuery::getNumInputVariables() const
{
    return _inputIndexToVariable.size();
}

unsigned InputQuery::getNumOutputVariables() const
{
    return _outputIndexToVariable.size();
}

List<unsigned> InputQuery::getInputVariables() const
{
    List<unsigned> result;
    for ( const auto &pair : _variableToInputIndex )
        result.append( pair.first );

    return result;
}

List<unsigned> InputQuery::getOutputVariables() const
{
    List<unsigned> result;
    for ( const auto &pair : _variableToOutputIndex )
        result.append( pair.first );

    return result;
}

void InputQuery::printAllBounds() const
{
    printf( "InputQuery: Dumping all bounds\n" );

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

void InputQuery::printInputOutputBounds() const
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

void InputQuery::dump() const
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
        printf( "\t %u: [%s, %s]\n", i,
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

    printf( "Equations:\n" );
    for ( const auto &e : _equations )
    {
        printf( "\t" );
        e.dump();
    }
}

void InputQuery::adjustInputOutputMapping( const Map<unsigned, unsigned> &oldIndexToNewIndex,
                                           const Map<unsigned, unsigned> &mergedVariables )
{
    Map<unsigned, unsigned> newInputIndexToVariable;
    unsigned currentIndex = 0;

    // Input variables
    for ( const auto &it : _inputIndexToVariable )
    {
        if ( mergedVariables.exists( it.second ) )
            throw MarabouError( MarabouError::MERGED_INPUT_VARIABLE,
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
            throw MarabouError( MarabouError::MERGED_OUTPUT_VARIABLE,
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

void InputQuery::setNetworkLevelReasoner( NLR::NetworkLevelReasoner *nlr )
{
    _networkLevelReasoner = nlr;
}

NLR::NetworkLevelReasoner *InputQuery::getNetworkLevelReasoner() const
{
    return _networkLevelReasoner;
}

bool InputQuery::constructNetworkLevelReasoner()
{
    INPUT_QUERY_LOG( "PP: constructing an NLR... " );

    if ( _networkLevelReasoner )
        delete _networkLevelReasoner;
    NLR::NetworkLevelReasoner *nlr = new NLR::NetworkLevelReasoner;

    Map<unsigned, unsigned> handledVariableToLayer;

    // First, put all the input neurons in layer 0
    List<unsigned> inputs = getInputVariables();
    nlr->addLayer( 0, NLR::Layer::INPUT, inputs.size() );
    unsigned index = 0;
    for ( const auto &inputVariable : inputs )
    {
        nlr->setNeuronVariable( NLR::NeuronIndex( 0, index ), inputVariable );
        handledVariableToLayer[inputVariable] = 0;
        ++index;
    }

    unsigned newLayerIndex = 1;
    // Now, repeatedly attempt to construct addditional layers
    while ( constructWeighedSumLayer( nlr, handledVariableToLayer, newLayerIndex ) ||
            constructReluLayer( nlr, handledVariableToLayer, newLayerIndex ) ||
            constructAbsoluteValueLayer( nlr, handledVariableToLayer, newLayerIndex ) )
    {
        ++newLayerIndex;
    }

    bool success = ( newLayerIndex > 1 );

    if ( success )
    {
        unsigned count = 0;
        for ( unsigned i = 0; i < nlr->getNumberOfLayers(); ++i )
            count += nlr->getLayer( i )->getSize();

        INPUT_QUERY_LOG( Stringf( "successful. Constructed %u layers with %u neurons (out of %u)\n",
                      newLayerIndex,
                      count,
                      getNumberOfVariables() ).ascii() );

        _networkLevelReasoner = nlr;
    }
    else
    {
        INPUT_QUERY_LOG( "unsuccessful\n" );
        delete nlr;
    }

    return success;
}

bool InputQuery::constructWeighedSumLayer( NLR::NetworkLevelReasoner *nlr,
                                           Map<unsigned, unsigned> &handledVariableToLayer,
                                           unsigned newLayerIndex )
{
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
    for ( const auto &eq : equations )
    {
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
        }
    }

    // No neurons found for the new layer
    if ( newNeurons.empty() )
        return false;

    nlr->addLayer( newLayerIndex, NLR::Layer::WEIGHTED_SUM, newNeurons.size() );
    for ( const auto &newNeuron : newNeurons )
    {
        handledVariableToLayer[newNeuron._variable] = newLayerIndex;

        // Add the new neuron
        nlr->setNeuronVariable( NLR::NeuronIndex( newLayerIndex, newNeuron._neuron ), newNeuron._variable );

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
            unsigned sourceNeuron = nlr->getLayer( sourceLayer )->variableToNeuron( addend._variable );

            // Mark the layer dependency
            nlr->addLayerDependency( sourceLayer, newLayerIndex );

            nlr->setWeight( sourceLayer,
                            sourceNeuron,
                            newLayerIndex,
                            newNeuron._neuron,
                            factor * addend._coefficient );
        }
    }

    return true;
}

bool InputQuery::constructReluLayer( NLR::NetworkLevelReasoner *nlr,
                                     Map<unsigned, unsigned> &handledVariableToLayer,
                                     unsigned newLayerIndex )
{
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
    const List<PiecewiseLinearConstraint *> &plConstraints =
        getPiecewiseLinearConstraints();

    for ( const auto &plc : plConstraints )
    {
        // Only consider ReLUs
        if ( plc->getType() != RELU )
            continue;

        const ReluConstraint *relu = (const ReluConstraint *)plc;

        // Has the b variable been handled?
        unsigned b = relu->getB();
        if ( !handledVariableToLayer.exists( b ) )
            continue;

        // If the f variable has also been handled, ignore this constraint
        unsigned f = relu->getF();
        if ( handledVariableToLayer.exists( f ) )
            continue;

        // B has been handled, f hasn't. Add f
        newNeurons.append( NeuronInformation( f, newNeurons.size(), b ) );
    }

    // No neurons found for the new layer
    if ( newNeurons.empty() )
        return false;

    nlr->addLayer( newLayerIndex, NLR::Layer::RELU, newNeurons.size() );
    for ( const auto &newNeuron : newNeurons )
    {
        handledVariableToLayer[newNeuron._variable] = newLayerIndex;

        unsigned sourceLayer = handledVariableToLayer[newNeuron._sourceVariable];
        unsigned sourceNeuron = nlr->getLayer( sourceLayer )->variableToNeuron( newNeuron._sourceVariable );

        // Mark the layer dependency
        nlr->addLayerDependency( sourceLayer, newLayerIndex );

        // Add the new neuron
        nlr->setNeuronVariable( NLR::NeuronIndex( newLayerIndex, newNeuron._neuron ), newNeuron._variable );

        // Mark the activation connection
        nlr->addActivationSource( sourceLayer,
                                  sourceNeuron,
                                  newLayerIndex,
                                  newNeuron._neuron );
    }

    return true;
}

bool InputQuery::constructAbsoluteValueLayer( NLR::NetworkLevelReasoner *nlr,
                                              Map<unsigned, unsigned> &handledVariableToLayer,
                                              unsigned newLayerIndex )
{
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
    const List<PiecewiseLinearConstraint *> &plConstraints =
        getPiecewiseLinearConstraints();

    for ( const auto &plc : plConstraints )
    {
        // Only consider ABSOLUTE_VALUE
        if ( plc->getType() != ABSOLUTE_VALUE )
            continue;

        const AbsoluteValueConstraint *abs = ( const AbsoluteValueConstraint *)plc;

        // Has the b variable been handled?
        unsigned b = abs->getB();
        if ( !handledVariableToLayer.exists( b ) )
            continue;

        // If the f variable has also been handled, ignore this constraint
        unsigned f = abs->getF();
        if ( handledVariableToLayer.exists( f ) )
            continue;

        // B has been handled, f hasn't. Add f
        newNeurons.append( NeuronInformation( f, newNeurons.size(), b ) );
        nlr->addConstraintInTopologicalOrder( plc );
    }

    // No neurons found for the new layer
    if ( newNeurons.empty() )
        return false;

    nlr->addLayer( newLayerIndex, NLR::Layer::ABSOLUTE_VALUE, newNeurons.size() );
    for ( const auto &newNeuron : newNeurons )
    {
        handledVariableToLayer[newNeuron._variable] = newLayerIndex;

        unsigned sourceLayer = handledVariableToLayer[newNeuron._sourceVariable];
        unsigned sourceNeuron = nlr->getLayer( sourceLayer )->variableToNeuron( newNeuron._sourceVariable );

        // Mark the layer dependency
        nlr->addLayerDependency( sourceLayer, newLayerIndex );

        // Add the new neuron
        nlr->setNeuronVariable( NLR::NeuronIndex( newLayerIndex, newNeuron._neuron ), newNeuron._variable );

        // Mark the activation connection
        nlr->addActivationSource( sourceLayer,
                                  sourceNeuron,
                                  newLayerIndex,
                                  newNeuron._neuron );
    }

    return true;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
