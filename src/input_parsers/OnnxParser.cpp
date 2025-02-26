/*********************                                                        */
/*! \file OnnxParser.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Matthew Daggitt
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** A parser for ONNX neural network files.
 **
 ** Some hard won knowledge about the design in this file:
 **   - The data is annoyingly heterogenous. Inputs and outputs of the onnx graph
 **   have different types from the nodes in the onnx graph, which have different
 **   types from the initializers (i.e. the constants (biases, filter weights etc.).
 **   This explains why all the methods take the names of things as inputs
 **   rather than passing references to onnx::NodeProto as one might expect.
 **   Likewise, this is why we store the shape data in a separate data structure
 **   rather than simply retrieving it when needed.
 **   - Node names are not unique, and do not necessarily even exist.
 **/

#include "OnnxParser.h"

#include "FloatUtils.h"
#include "InputParserError.h"
#include "MString.h"
#include "Query.h"
#include "ReluConstraint.h"
#include "TensorUtils.h"
#include "onnx.proto3.pb.h"

#include <fstream>
#include <iostream>
#include <math.h>

/******************
 * Public methods *
 ******************/

/**
 * @brief Generates the variables for a query over only a subset of the network.
 *
 * @param query The object to which to add the generated constraints to.
 * @param path The path to the ONNX neural network file.
 * @param initialNodeNames The set of initial nodes. Note these don't have to be an actual input to
 * the network, they can be intermediate nodes. If empty, then defaults to the network's input
 * nodes.
 * @param terminalNodeNames The set of terminal nodes. Note that these doesn't have to be outputs of
 * the network, they can be intermediate nodes. If empty, then defaults to the network's output
 * nodes.
 */
void OnnxParser::parse( InputQueryBuilder &query,
                        const String &path,
                        const Set<String> initialNodeNames,
                        const Set<String> terminalNodeNames )
{
    OnnxParser parser = OnnxParser( query, path, initialNodeNames, terminalNodeNames );
    parser.processGraph();
}


/*************
 * Utilities *
 *************/

void illTypedAttributeError( onnx::NodeProto &node,
                             const onnx::AttributeProto &attr,
                             onnx::AttributeProto_AttributeType expectedType )
{
    String errorMessage = Stringf(
        "Expected attribute %s on Onnx node of type %s to be of type %d but is actually of type %d",
        attr.name().c_str(),
        node.op_type().c_str(),
        expectedType,
        attr.type() );
    throw MarabouError( MarabouError::ONNX_PARSER_ERROR, errorMessage.ascii() );
}

void missingAttributeError( onnx::NodeProto &node, String attributeName )
{
    String errorMessage = Stringf( "Onnx node of type %s is missing the expected attribute %s",
                                   node.op_type().c_str(),
                                   attributeName.ascii() );
    throw MarabouError( MarabouError::ONNX_PARSER_ERROR, errorMessage.ascii() );
}

void unimplementedOperationError( onnx::NodeProto &node )
{
    String errorMessage = Stringf( "Onnx '%s' operation not yet implemented for command line "
                                   "support. Should be relatively easy to add.",
                                   node.op_type().c_str() );
    throw MarabouError( MarabouError::ONNX_PARSER_ERROR, errorMessage.ascii() );
}

void unimplementedAttributeError( onnx::NodeProto &node, String attributeName )
{
    String errorMessage =
        Stringf( "Onnx '%s' operation with non-default value for attribute '%s' not yet supported.",
                 node.op_type().c_str(),
                 attributeName.ascii() );
    throw MarabouError( MarabouError::ONNX_PARSER_ERROR, errorMessage.ascii() );
}

void unimplementedConstantTypeError( onnx::TensorProto_DataType type )
{
    String errorMessage = Stringf( "Support for Onnx constants of type '%s' not yet implemented.",
                                   TensorProto_DataType_Name( type ).c_str() );
    throw MarabouError( MarabouError::ONNX_PARSER_ERROR, errorMessage.ascii() );
}

void unsupportedError( onnx::NodeProto &node )
{
    String errorMessage =
        Stringf( "Onnx operation %s not currently supported by Marabou", node.op_type().c_str() );
    throw MarabouError( MarabouError::ONNX_PARSER_ERROR, errorMessage.ascii() );
}

void unsupportedCastError( onnx::TensorProto_DataType from, onnx::TensorProto_DataType to )
{
    String errorMessage =
        Stringf( "The ONNX parser does not currently support casting from '%s' to '%s'",
                 TensorProto_DataType_Name( from ).c_str(),
                 TensorProto_DataType_Name( to ).c_str() );
    throw MarabouError( MarabouError::ONNX_PARSER_ERROR, errorMessage.ascii() );
}

void unexpectedNegativeValue( int value, String location )
{
    String errorMessage =
        Stringf( "Found unexpected negative value '%d' for '%s'", value, location.ascii() );
    throw MarabouError( MarabouError::ONNX_PARSER_ERROR, errorMessage.ascii() );
}

void missingNodeError( String &missingNodeName )
{
    String errorMessage = Stringf( "Internal invariant violated: missing node '%s' not found",
                                   missingNodeName.ascii() );
    throw MarabouError( MarabouError::ONNX_PARSER_ERROR, errorMessage.ascii() );
}

void unexpectedNumberOfInputs( onnx::NodeProto &node,
                               unsigned int actualNumberOfInputs,
                               unsigned int lowerBound,
                               unsigned int upperBound )
{
    String errorMessage;
    if ( lowerBound == upperBound )
    {
        errorMessage = Stringf( "%s node expected to have exactly %d inputs, but found %d",
                                node.op_type().c_str(),
                                lowerBound,
                                actualNumberOfInputs );
    }
    else
    {
        errorMessage = Stringf( "%s node expected to have between %d and %d inputs, but found %d",
                                node.op_type().c_str(),
                                lowerBound,
                                upperBound,
                                actualNumberOfInputs );
    }
    throw MarabouError( MarabouError::ONNX_PARSER_ERROR, errorMessage.ascii() );
}

void checkTensorDataSourceIsInternal( const onnx::TensorProto &tensor )
{
    if ( tensor.data_location() == onnx::TensorProto_DataLocation_EXTERNAL )
    {
        String errorMessage =
            Stringf( "External data locations not yet implemented for command line Onnx support" );
        throw MarabouError( MarabouError::ONNX_PARSER_ERROR, errorMessage.ascii() );
    }
}

void checkTensorDataType( const onnx::TensorProto &tensor, int32_t expectedDataType )
{
    int32_t actualDataType = tensor.data_type();
    if ( actualDataType != expectedDataType )
    {
        std::string actualName = onnx::TensorProto_DataType_Name( actualDataType );
        std::string expectedName = onnx::TensorProto_DataType_Name( actualDataType );
        String errorMessage =
            Stringf( "Expected tensor '%s' to be of type %s but actually of type %s",
                     expectedName.c_str(),
                     actualName.c_str() );
        throw MarabouError( MarabouError::ONNX_PARSER_ERROR, errorMessage.ascii() );
    }
}

TensorShape shapeOfInput( onnx::ValueInfoProto &input )
{
    TensorShape result;
    for ( auto dim : input.type().tensor_type().shape().dim() )
    {
        int size = dim.dim_value();
        if ( size < 0 )
        {
            String errorMessage =
                Stringf( "Found input tensor in ONNX file with invalid size '%d'", size );
            throw MarabouError( MarabouError::ONNX_PARSER_ERROR, errorMessage.ascii() );
        }
        else if ( size == 0 )
        {
            // A size of `0` represents an unknown size. The most likely case is
            // that this is the batch size which is unspecified in most models.
            // Under this assumption the correct thing to do is to pretend the
            // batch size is 1 as verification properties refer to single values
            // rather than batches.
            result.append( 1 );
        }
        else
        {
            result.append( size );
        }
    }
    return result;
}

TensorShape shapeOfConstant( const onnx::TensorProto &constant )
{
    TensorShape result;
    for ( auto dim : constant.dims() )
    {
        result.append( dim );
    }
    return result;
}

/**
 * @brief Fills in the variables (values 0 and -1) in the new shape template.
 * See https://github.com/onnx/onnx/blob/main/docs/Operators.md#Reshape for a
 * description of the variables.
 *
 * @param oldShape the previous shape of the tensor
 * @param newShapeTemplate the template for the new shape of the tensor, may
 * contain -1 values representing dimensions to be inferred.
 * @param allowZero whether or not zero-valued dimensions are allowed. If they
 * are not then all zeroes are replaced with the corresponding value in the
 * old shape.
 * @return
 */
TensorShape
instantiateReshapeTemplate( TensorShape oldShape, Vector<int64_t> newShapeTemplate, bool allowZero )
{
    TensorShape newShape;
    int inferredIndex = -1;

    for ( unsigned int i = 0; i < newShapeTemplate.size(); i++ )
    {
        int dimSize = newShapeTemplate[i];
        if ( dimSize == -1 )
        {
            // Then this dimension should be inferred.
            // Temporarily set the dimSize to be 1 so that we
            // can use the tensorSize function to compute the
            // size so far.
            inferredIndex = i;
            dimSize = 1;
        }
        else if ( dimSize == 0 )
        {
            if ( !allowZero )
            {
                if ( i < oldShape.size() )
                {
                    dimSize = oldShape[i];
                }
            }
        }
        newShape.append( dimSize );
    }

    if ( inferredIndex != -1 )
    {
        newShape[inferredIndex] = tensorSize( oldShape ) / tensorSize( newShape );
    }
    return newShape;
}

void checkEndianness()
{
    int num = 1;
    bool systemIsLittleEndian = *(char *)&num == 1;
    if ( !systemIsLittleEndian )
    {
        String errorMessage = "Support for Onnx files on non-little endian systems is not "
                              "currently implemented on the command line";
        throw MarabouError( MarabouError::ONNX_PARSER_ERROR, errorMessage.ascii() );
    }
}

const onnx::AttributeProto *
findAttribute( onnx::NodeProto &node, String name, onnx::AttributeProto_AttributeType expectedType )
{
    for ( const onnx::AttributeProto &attr : node.attribute() )
    {
        if ( attr.name() == name.ascii() )
        {
            if ( attr.type() != expectedType )
            {
                illTypedAttributeError( node, attr, expectedType );
            }
            return &attr;
        }
    }
    return nullptr;
}

float getFloatAttribute( onnx::NodeProto &node, String name, float defaultValue )
{
    const onnx::AttributeProto *attr =
        findAttribute( node, name, onnx::AttributeProto_AttributeType_FLOAT );
    if ( attr == nullptr )
    {
        return defaultValue;
    }
    return attr->f();
}

String getStringAttribute( onnx::NodeProto &node, String name, String defaultValue )
{
    const onnx::AttributeProto *attr =
        findAttribute( node, name, onnx::AttributeProto_AttributeType_STRING );
    if ( attr == nullptr )
    {
        return defaultValue;
    }
    return attr->s();
}

int getIntAttribute( onnx::NodeProto &node, String name, int defaultValue )
{
    const onnx::AttributeProto *attr =
        findAttribute( node, name, onnx::AttributeProto_AttributeType_INT );
    if ( attr == nullptr )
    {
        return defaultValue;
    }
    return attr->i();
}

int getRequiredIntAttribute( onnx::NodeProto &node, String name )
{
    const onnx::AttributeProto *attr =
        findAttribute( node, name, onnx::AttributeProto_AttributeType_INT );
    if ( attr == nullptr )
    {
        missingAttributeError( node, name );
    }
    return attr->i();
}

const onnx::AttributeProto *getTensorAttribute( onnx::NodeProto &node, String name )
{
    const onnx::AttributeProto *attr =
        findAttribute( node, name, onnx::AttributeProto_AttributeType_TENSOR );
    if ( attr == nullptr )
    {
        missingAttributeError( node, name );
    }
    return attr;
}

Vector<int> getIntsAttribute( onnx::NodeProto &node, String name, Vector<int> &defaultValue )
{
    const onnx::AttributeProto *attr =
        findAttribute( node, name, onnx::AttributeProto_AttributeType_INTS );
    if ( attr == nullptr )
    {
        return defaultValue;
    }

    Vector<int> result;
    for ( int i = 0; i < attr->ints_size(); i++ )
    {
        result.append( attr->ints( i ) );
    }
    return result;
}

Vector<unsigned int>
getNonNegativeIntsAttribute( onnx::NodeProto &node, String name, Vector<uint> &defaultValue )
{
    const onnx::AttributeProto *attr =
        findAttribute( node, name, onnx::AttributeProto_AttributeType_INTS );
    if ( attr == nullptr )
    {
        return defaultValue;
    }

    Vector<unsigned int> result;
    for ( int i = 0; i < attr->ints_size(); i++ )
    {
        int value = attr->ints( i );
        if ( value >= 0 )
        {
            result.append( (uint)value );
        }
        else
        {
            String location =
                Stringf( "attribute '%s' on node '%s'", name.ascii(), node.name().c_str() );
            unexpectedNegativeValue( value, location );
        }
    }
    return result;
}


Vector<double> getTensorFloatValues( const onnx::TensorProto &tensor, const TensorShape shape )
{
    int size = tensorSize( shape );
    std::string raw_data = tensor.raw_data();
    Vector<double> result;
    if ( raw_data.size() != 0 )
    {
        checkEndianness();
        const char *bytes = raw_data.c_str();
        const float *floats = reinterpret_cast<const float *>( bytes );
        for ( int i = 0; i < size; i++ )
        {
            result.append( *( floats + i ) );
        }
    }
    else
    {
        for ( int i = 0; i < size; i++ )
        {
            result.append( tensor.float_data( i ) );
        }
    }
    return result;
}

Vector<int64_t> getTensorIntValues( const onnx::TensorProto &tensor, const TensorShape shape )
{
    int size = tensorSize( shape );
    std::string raw_data = tensor.raw_data();
    Vector<int64_t> result;
    if ( raw_data.size() != 0 )
    {
        checkEndianness();
        const char *bytes = raw_data.c_str();
        const int64_t *ints = reinterpret_cast<const int64_t *>( bytes );
        for ( int i = 0; i < size; i++ )
        {
            int64_t value = *( ints + i );
            result.append( value );
        }
    }
    else
    {
        for ( int i = 0; i < size; i++ )
        {
            int value = tensor.int64_data( i );
            result.append( value );
        }
    }
    return result;
}

Vector<int32_t> getTensorInt32Values( const onnx::TensorProto &tensor, const TensorShape shape )
{
    int size = tensorSize( shape );
    std::string raw_data = tensor.raw_data();
    Vector<int32_t> result;
    if ( raw_data.size() != 0 )
    {
        checkEndianness();
        const char *bytes = raw_data.c_str();
        const int32_t *int32 = reinterpret_cast<const int32_t *>( bytes );
        for ( int i = 0; i < size; i++ )
        {
            int32_t value = *( int32 + i );
            result.append( value );
        }
    }
    else
    {
        for ( int i = 0; i < size; i++ )
        {
            int32_t value = tensor.int32_data( i );
            result.append( value );
        }
    }
    return result;
}

bool OnnxParser::isConstantNode( String name )
{
    return _constantIntTensors.exists( name ) || _constantFloatTensors.exists( name ) ||
           _constantInt32Tensors.exists( name );
}

void OnnxParser::insertConstant( String name,
                                 const onnx::TensorProto &tensor,
                                 const TensorShape shape )
{
    checkTensorDataSourceIsInternal( tensor );

    onnx::TensorProto_DataType dataType =
        static_cast<onnx::TensorProto_DataType>( tensor.data_type() );
    if ( dataType == onnx::TensorProto_DataType_INT64 )
    {
        _constantIntTensors.insert( name, getTensorIntValues( tensor, shape ) );
    }
    else if ( dataType == onnx::TensorProto_DataType_FLOAT )
    {
        _constantFloatTensors.insert( name, getTensorFloatValues( tensor, shape ) );
    }
    else if ( dataType == onnx::TensorProto_DataType_BOOL )
    {
        // NOTE: INT32, INT16, INT8, INT4, UINT16, UINT8, UINT4, BOOL, FLOAT16,
        // BFLOAT16, FLOAT8E4M3FN, FLOAT8E4M3FNUZ, FLOAT8E5M2, FLOAT8E5M2FNUZ
        // are all represented as an int32. Support for those should be added later
        // when there is a practical need.
        _constantInt32Tensors.insert( name, getTensorInt32Values( tensor, shape ) );
    }
    else
    {
        unimplementedConstantTypeError( dataType );
    }
}

void OnnxParser::transferValues( String oldName, String newName )
{
    if ( _varMap.exists( oldName ) )
    {
        _varMap[newName] = _varMap[oldName];
    }
    else if ( _constantIntTensors.exists( oldName ) )
    {
        _constantIntTensors.insert( newName, _constantIntTensors[oldName] );
    }
    else if ( _constantFloatTensors.exists( oldName ) )
    {
        _constantFloatTensors.insert( newName, _constantFloatTensors[oldName] );
    }
    else if ( _constantInt32Tensors.exists( oldName ) )
    {
        _constantInt32Tensors.insert( newName, _constantInt32Tensors[oldName] );
    }
    else
    {
        missingNodeError( oldName );
    }
}

/*******************
 * Private methods *
 *******************/

OnnxParser::OnnxParser( InputQueryBuilder &query,
                        const String &path,
                        const Set<String> inputNames,
                        const Set<String> terminalNames )
    : _query( query )
{
    // open file and move current position in file to the end
    std::ifstream input( path.ascii(), std::ios::ate | std::ios::binary );
    // get current position in file
    std::streamsize size = input.tellg();
    // move to start of file
    input.seekg( 0, std::ios::beg );

    // read raw data
    Vector<char> buffer( size );
    input.read( buffer.data(), size );

    // parse protobuf
    onnx::ModelProto model;
    model.ParseFromArray( buffer.data(), size );
    _network = model.graph();

    _numberOfFoundInputs = 0;


    if ( inputNames.empty() )
    {
        _inputNames = readInputNames();
    }
    else
    {
        validateUserInputNames( inputNames );
        _inputNames = inputNames;
    }

    if ( terminalNames.empty() )
    {
        _terminalNames = readOutputNames();
    }
    else
    {
        validateUserTerminalNames( terminalNames );
        _terminalNames = terminalNames;
    }
}

void OnnxParser::validateUserInputNames( const Set<String> &inputNames )
{
    // Collate all input nodes
    Set<String> allInputNames;
    for ( const onnx::ValueInfoProto &node : _network.input() )
    {
        const std::string name = node.name();
        if ( isConstantNode( name ) )
            continue;

        if ( allInputNames.exists( name ) )
        {
            String errorMessage = Stringf(
                "Input nodes in Onnx network must have a unique name but found duplicate name '%s'",
                name.c_str() );
            throw MarabouError( MarabouError::ONNX_PARSER_ERROR, errorMessage.ascii() );
        }
        else
        {
            allInputNames.insert( name );
        }
    }

    // Validate the provided inputs
    for ( String inputName : inputNames )
    {
        if ( !allInputNames.exists( inputName ) )
        {
            String errorMessage = Stringf( "Input %s not found in graph!", inputName.ascii() );
            throw MarabouError( MarabouError::ONNX_PARSER_ERROR, errorMessage.ascii() );
        }
    }
}

void OnnxParser::validateUserTerminalNames( const Set<String> &terminalNames )
{
    for ( String terminalName : terminalNames )
    {
        for ( auto node : _network.node() )
        {
            for ( String outputNodeName : node.output() )
            {
                if ( terminalName == outputNodeName )
                    return;
            }
        }

        String errorMessage = Stringf( "Output %s not found in graph!", terminalName.ascii() );
        throw MarabouError( MarabouError::ONNX_PARSER_ERROR, errorMessage.ascii() );
    }
}

const Set<String> OnnxParser::readInputNames()
{
    ASSERT( _network.input().size() >= 1 );

    Set<String> initializerNames;
    for ( auto &initNode : _network.initializer() )
    {
        ONNX_LOG( Stringf( "Found initialiser '%s'", initNode.name().c_str() ).ascii() );
        initializerNames.insert( initNode.name() );
    }

    Set<String> inputNames;
    for ( auto &inputNode : _network.input() )
    {
        ONNX_LOG( Stringf( "Found input '%s'", inputNode.name().c_str() ).ascii() );
        inputNames.insert( inputNode.name() );
    }

    return Set<String>::difference( inputNames, initializerNames );
}

const Set<String> OnnxParser::readOutputNames()
{
    Set<String> outputNames;
    for ( auto &outputNode : _network.output() )
    {
        ONNX_LOG( Stringf( "Found output '%s'", outputNode.name().c_str() ).ascii() );
        outputNames.insert( outputNode.name() );
    }
    return outputNames;
}

void OnnxParser::initializeShapeAndConstantMaps()
{
    // Add shapes for inputs
    for ( auto input : _network.input() )
    {
        String inputName = input.name();
        _shapeMap.insert( inputName, shapeOfInput( input ) );

        // If this is one of the specified inputs, create new variables
        if ( _inputNames.exists( inputName ) )
        {
            _processedNodes.insert( inputName );
            _numberOfFoundInputs += 1;
            Vector<Variable> nodeVars = makeNodeVariables( inputName, true );
        }
    }

    // Initialise constants
    for ( const onnx::TensorProto &constant : _network.initializer() )
    {
        String constantName = constant.name();
        if ( isConstantNode( constantName ) )
        {
            String errorMessage = Stringf( "Initializers in Onnx network must have a unique name "
                                           "but found duplicate name '%s'",
                                           constantName.ascii() );
            throw MarabouError( MarabouError::ONNX_PARSER_ERROR, errorMessage.ascii() );
        }
        else
        {
            TensorShape constantShape = shapeOfConstant( constant );
            _shapeMap.insert( constantName, constantShape );
            _processedNodes.insert( constantName );
            insertConstant( constantName, constant, constantShape );
        }
    }
}

void OnnxParser::validateAllInputsAndOutputsFound()
{
    // By this point, all input variables need to have been found
    if ( _numberOfFoundInputs != _inputNames.size() )
    {
        String errorMessage = "These input variables could not be found:";
        for ( String inputName : _inputNames )
        {
            if ( !_varMap.exists( inputName ) )
            {
                String space = " ";
                errorMessage += space + inputName;
            }
        }
        throw MarabouError( MarabouError::ONNX_PARSER_ERROR, errorMessage.ascii() );
    }

    // Mark the output variables
    for ( String terminalName : _terminalNames )
    {
        for ( Variable var : _varMap[terminalName] )
        {
            _query.markOutputVariable( var );
        }
    }
}

/**
 * @brief Processes the graph, adding all the generated constraints to the query.
 * Unlike the Python implementation, at the moment assumes there is only a single output.
 *
 * @param inputNames The names of the input nodes to start at.
 * @param terminalNames The names of the output node to end at.
 * @param query The query in which to store the generated constraints.
 */
void OnnxParser::processGraph()
{
    initializeShapeAndConstantMaps();
    for ( String terminalName : _terminalNames )
    {
        processNode( terminalName, true );
    }
    validateAllInputsAndOutputsFound();
}

/**
 * @brief Recursively processes the structure of the graph, adding the relevant
 * constraints for each node as it goes. Takes in a string rather than a NodeProto
 * as we initially pass in the output which is *not* a NodeProto.
 *
 * @param node
 * @param makeEquations
 */
void OnnxParser::processNode( String &nodeName, bool makeEquations )
{
    if ( _processedNodes.exists( nodeName ) )
        return;

    if ( _inputNames.exists( nodeName ) )
    {
        _numberOfFoundInputs += 1;
        // If an inputName is an intermediate layer of the network, we don't need to create Marabou
        // equations for its inputs. However, we still need to call makeMarabouEquations in order to
        // compute shapes. We just need to set the makeEquations flag to false
        makeEquations = false;
    }

    _processedNodes.insert( nodeName );

    List<onnx::NodeProto> nodes = getNodesWithOutput( nodeName );
    ASSERT( nodes.size() == 1 );
    onnx::NodeProto &node = nodes.front();

    // First recursively process the input nodes.
    // This ensures that shapes and values of a node's inputs have been computed first.
    for ( auto inputNode : getInputsToNode( node ) )
    {
        processNode( inputNode, makeEquations );
    }

    // Compute node's shape and create Marabou equations as needed
    makeMarabouEquations( node, makeEquations );

    // Create new variables when we find one of the inputs
    if ( _inputNames.exists( nodeName ) )
    {
        Vector<Variable> vars = makeNodeVariables( nodeName, true );
    }
}

/**
 * @brief Assuming the node's shape is known, return a set of new variables in the same shape
 */
Vector<Variable> OnnxParser::makeNodeVariables( String &nodeName, bool isInput )
{
    ASSERT( !_varMap.exists( nodeName ) );
    TensorShape shape = _shapeMap[nodeName];
    int size = tensorSize( shape );

    Vector<Variable> variables;
    for ( int i = 0; i < size; i++ )
    {
        Variable variable = _query.getNewVariable();
        variables.append( variable );
        if ( isInput )
        {
            _query.markInputVariable( variable );
        }
    }
    _varMap.insert( nodeName, variables );
    return variables;
}

List<onnx::NodeProto> OnnxParser::getNodesWithOutput( String &nodeName )
{
    List<onnx::NodeProto> nodes;
    for ( auto node : _network.node() )
    {
        for ( auto outputName : node.output() )
        {
            if ( outputName == nodeName.ascii() )
            {
                nodes.append( node );
                break;
            }
        }
    }
    return nodes;
}

Set<String> OnnxParser::getInputsToNode( onnx::NodeProto &node )
{
    Set<String> inputNames;
    for ( String inputNodeName : node.input() )
    {
        if ( getNodesWithOutput( inputNodeName ).size() >= 1 )
        {
            inputNames.insert( inputNodeName );
        }
    }
    return inputNames;
}

/**
 * @brief Compute the shape and values of a node assuming the input shapes
 * and values have been computed already.
 *
 * @param node Name of node for which we want to compute the output shape
 * @param makeEquations Create Marabou equations for this node if True
 */
void OnnxParser::makeMarabouEquations( onnx::NodeProto &node, bool makeEquations )
{
    auto nodeType = node.op_type().c_str();
    ONNX_LOG(
        Stringf( "Processing node '%s' of type '%s'", node.name().c_str(), nodeType ).ascii() );

    if ( strcmp( nodeType, "Constant" ) == 0 )
    {
        constant( node );
    }
    else if ( strcmp( nodeType, "Identity" ) == 0 )
    {
        identity( node );
    }
    else if ( strcmp( nodeType, "Dropout" ) == 0 )
    {
        dropout( node );
    }
    else if ( strcmp( nodeType, "Cast" ) == 0 )
    {
        cast( node );
    }
    else if ( strcmp( nodeType, "Reshape" ) == 0 )
    {
        reshape( node );
    }
    else if ( strcmp( nodeType, "Flatten" ) == 0 )
    {
        flatten( node );
    }
    else if ( strcmp( nodeType, "Transpose" ) == 0 )
    {
        transpose( node );
    }
    else if ( strcmp( nodeType, "Squeeze" ) == 0 )
    {
        squeeze( node );
    }
    else if ( strcmp( nodeType, "Unsqueeze" ) == 0 )
    {
        unsqueeze( node );
    }
    else if ( strcmp( nodeType, "BatchNormalization" ) == 0 )
    {
        batchNormEquations( node, makeEquations );
    }
    else if ( strcmp( nodeType, "MaxPool" ) == 0 )
    {
        maxPoolEquations( node, makeEquations );
    }
    else if ( strcmp( nodeType, "Conv" ) == 0 )
    {
        convEquations( node, makeEquations );
    }
    else if ( strcmp( nodeType, "Gemm" ) == 0 )
    {
        gemmEquations( node, makeEquations );
    }
    else if ( strcmp( nodeType, "Add" ) == 0 )
    {
        scaleAndAddEquations( node, makeEquations, 1, 1 );
    }
    else if ( strcmp( nodeType, "Sub" ) == 0 )
    {
        scaleAndAddEquations( node, makeEquations, 1, -1 );
    }
    else if ( strcmp( nodeType, "Relu" ) == 0 )
    {
        reluEquations( node, makeEquations );
    }
    else if ( strcmp( nodeType, "LeakyRelu" ) == 0 )
    {
        leakyReluEquations( node, makeEquations );
    }
    else if ( strcmp( nodeType, "MatMul" ) == 0 )
    {
        matMulEquations( node, makeEquations );
    }
    else if ( strcmp( nodeType, "Sigmoid" ) == 0 )
    {
        sigmoidEquations( node, makeEquations );
    }
    else if ( strcmp( nodeType, "Tanh" ) == 0 )
    {
        tanhEquations( node, makeEquations );
    }
    else
    {
        unsupportedError( node );
    }
}

/**
 * @brief Processes a constant node in the network.
 * https://github.com/onnx/onnx/blob/main/docs/Operators.md#Constant
 *
 * @param node The ONNX node
 */
void OnnxParser::constant( onnx::NodeProto &node )
{
    String outputNodeName = node.output()[0];
    const onnx::TensorProto &value = getTensorAttribute( node, "value" )->t();
    const TensorShape shape = shapeOfConstant( value );

    _shapeMap[outputNodeName] = shape;
    insertConstant( outputNodeName, value, shape );
}

/**
 * @brief Processes an identity node in the network.
 * Implements https://github.com/onnx/onnx/blob/main/docs/Operators.md#Identity
 *
 * @param node The ONNX node
 */
void OnnxParser::identity( onnx::NodeProto &node )
{
    String outputNodeName = node.output()[0];
    String inputNodeName = node.input()[0];

    _shapeMap[outputNodeName] = _shapeMap[inputNodeName];
    transferValues( inputNodeName, outputNodeName );
}

/**
 * @brief Processes a dropout node in the network.
 * Implements https://github.com/onnx/onnx/blob/main/docs/Operators.md#Dropout
 *
 * @param node The ONNX node
 */
void OnnxParser::dropout( onnx::NodeProto &node )
{
    if ( node.input().size() == 3 )
    {
        String trainingModeName = node.input()[2];
        if ( !_constantInt32Tensors.exists( trainingModeName ) )
        {
            missingNodeError( trainingModeName );
        }
        else if ( _constantInt32Tensors[trainingModeName][0] )
        {
            // training mode is set to true
            throw MarabouError( MarabouError::ONNX_PARSER_ERROR,
                                "Marabou only supports training_mode=false in Dropout" );
        }
    }

    identity( node );
}

/**
 * @brief Processes a cast node in the network.
 * Implements https://github.com/onnx/onnx/blob/main/docs/Operators.md#Cast
 *
 * @param node The ONNX node
 */
void OnnxParser::cast( onnx::NodeProto &node )
{
    String outputNodeName = node.output()[0];
    String inputNodeName = node.input()[0];
    _shapeMap[outputNodeName] = _shapeMap[inputNodeName];

    // Try to find type to cast to. If not found, raise error.
    onnx::TensorProto_DataType to =
        static_cast<onnx::TensorProto_DataType>( getRequiredIntAttribute( node, "to" ) );

    if ( _varMap.exists( inputNodeName ) )
    {
        // We shouldn't be casting variables to different types, since Marabou assumes variables
        // have double precision
        String errorMessage =
            Stringf( "The node '%s' casts non-constant values which is not supported by Marabou",
                     inputNodeName.ascii() );
        throw MarabouError( MarabouError::ONNX_PARSER_ERROR, errorMessage.ascii() );
    }

    if ( _constantIntTensors.exists( inputNodeName ) )
    {
        Vector<int64_t> tensor = _constantIntTensors[inputNodeName];
        if ( to == onnx::TensorProto_DataType_INT64 )
        {
            _constantIntTensors.insert( outputNodeName, tensor );
        }
        else if ( to == onnx::TensorProto_DataType_FLOAT )
        {
            Vector<double> castTensor = Vector<double>( tensor.size() );
            for ( PackedTensorIndices i = 0; i < tensor.size(); i++ )
            {
                castTensor[i] = static_cast<double>( tensor[i] );
            }
            _constantFloatTensors.insert( outputNodeName, castTensor );
        }
        else
        {
            unsupportedCastError( onnx::TensorProto_DataType_INT64, to );
        }
    }
    else if ( _constantFloatTensors.exists( inputNodeName ) )
    {
        Vector<double> tensor = _constantFloatTensors[inputNodeName];
        if ( to == onnx::TensorProto_DataType_INT64 )
        {
            Vector<int64_t> castTensor = Vector<int64_t>( tensor.size() );
            for ( PackedTensorIndices i = 0; i < tensor.size(); i++ )
            {
                castTensor[i] = static_cast<int64_t>( tensor[i] );
            }
            _constantIntTensors.insert( outputNodeName, castTensor );
        }
        else if ( to == onnx::TensorProto_DataType_FLOAT )
        {
            _constantFloatTensors.insert( outputNodeName, tensor );
        }
        else
        {
            unsupportedCastError( onnx::TensorProto_DataType_FLOAT, to );
        }
    }
    else
    {
        missingNodeError( inputNodeName );
    }
}

/**
 * @brief Processes an reshape node in the network.
 * Implements https://github.com/onnx/onnx/blob/main/docs/Operators.md#Reshape
 *
 * @param node The ONNX node
 */
void OnnxParser::reshape( onnx::NodeProto &node )
{
    // Assume first input is array to be reshaped, second input is the new shape array
    String inputNodeName = node.input()[0];
    String shapeNodeName = node.input()[1];

    String outputNodeName = node.output()[0];

    bool allowZeroes = getIntAttribute( node, "allowzero", 0 ) != 0;

    TensorShape oldShape = _shapeMap[inputNodeName];
    Vector<int64_t> newShapeTemplate = _constantIntTensors[shapeNodeName];
    TensorShape newShape = instantiateReshapeTemplate( oldShape, newShapeTemplate, allowZeroes );
    _shapeMap[outputNodeName] = newShape;

    // Transfer constants/variables
    transferValues( inputNodeName, outputNodeName );
}

/**
 * @brief Processes a "flatten" node in the network.
 * Implements https://github.com/onnx/onnx/blob/main/docs/Operators.md#Flatten
 *
 * @param node The ONNX node
 */
void OnnxParser::flatten( onnx::NodeProto &node )
{
    String outputNodeName = node.output()[0];
    String inputNodeName = node.input()[0];

    // Get parameters
    unsigned int axis = getIntAttribute( node, "axis", 1 );

    // Calculate output shape
    TensorShape inputShape = _shapeMap[inputNodeName];
    int dim1 = 1;
    for ( unsigned int i = 0; i < axis; i++ )
    {
        dim1 *= inputShape[i];
    }
    int dim2 = 1;
    for ( unsigned int i = axis; i < inputShape.size(); i++ )
    {
        dim2 *= inputShape[i];
    }
    TensorShape outputShape;
    outputShape.append( dim1 );
    outputShape.append( dim2 );

    // Transfer constants/variables
    _shapeMap[outputNodeName] = outputShape;
    transferValues( inputNodeName, outputNodeName );
}

/**
 * @brief Processes a "transpose" node in the network.
 * Implements https://github.com/onnx/onnx/blob/main/docs/Operators.md#Transpose
 *
 * @param node The ONNX node
 */
void OnnxParser::transpose( onnx::NodeProto &node )
{
    String inputNodeName = node.input()[0];
    String outputNodeName = node.output()[0];

    TensorShape inputShape = _shapeMap[inputNodeName];

    // Get permutation (default is the reverse permutation)
    Permutation defaultPerm = reversePermutation( inputShape.size() );
    Permutation perm = getNonNegativeIntsAttribute( node, "perm", defaultPerm );

    // Calculate the output shape
    TensorShape outputShape = transposeVector( inputShape, perm );
    _shapeMap[outputNodeName] = outputShape;

    // Update the constant or variable maps
    if ( _varMap.exists( inputNodeName ) )
    {
        Vector<Variable> inputVars = _varMap[inputNodeName];
        _varMap[outputNodeName] = transposeTensor( inputVars, inputShape, perm );
    }
    else if ( _constantIntTensors.exists( inputNodeName ) )
    {
        Vector<int64_t> inputConstant = _constantIntTensors[inputNodeName];
        _constantIntTensors.insert( outputNodeName,
                                    transposeTensor( inputConstant, inputShape, perm ) );
    }
    else if ( _constantFloatTensors.exists( inputNodeName ) )
    {
        Vector<double> inputConstant = _constantFloatTensors[inputNodeName];
        _constantFloatTensors.insert( outputNodeName,
                                      transposeTensor( inputConstant, inputShape, perm ) );
    }
    else
    {
        missingNodeError( inputNodeName );
    }
}

/**
 * @brief Processes a "squeeze" node in the network.
 * Implements https://github.com/onnx/onnx/blob/main/docs/Operators.md#Squeeze
 *
 * @param node The ONNX node
 */
void OnnxParser::squeeze( onnx::NodeProto &node )
{
    String inputNodeName = node.input()[0];
    String outputNodeName = node.output()[0];

    // Get the input shape
    TensorShape inputShape = _shapeMap[inputNodeName];

    // Get the axes to squeeze
    Vector<SignedTensorIndex> axes;
    unsigned int numberOfInputs = node.input().size();
    if ( numberOfInputs == 1 )
    {
        for ( unsigned int i = 0; i < inputShape.size(); i++ )
        {
            if ( inputShape[i] == 1 )
            {
                axes.append( i );
            }
        }
    }
    else if ( numberOfInputs == 2 )
    {
        String axisName = node.input()[1];
        if ( !_constantIntTensors.exists( axisName ) )
        {
            missingNodeError( axisName );
        }
        for ( int64_t axis : _constantIntTensors[axisName] )
        {
            axes.append( static_cast<int>( axis ) );
        }
    }
    else
    {
        unexpectedNumberOfInputs( node, numberOfInputs, 1, 2 );
    }

    // Calculate the output shape
    TensorShape outputShape = Vector<unsigned int>( inputShape );
    for ( SignedTensorIndex signedAxis : axes )
    {
        TensorIndex axis = unsignIndex( inputShape.size(), signedAxis );
        if ( inputShape[axis] != 1 )
        {
            String errorMessage = Stringf( "Invalid axis found on Squeeze node '%s'. Expected "
                                           "dimension %d to be of size 1 but was size %d",
                                           inputNodeName.ascii(),
                                           axis,
                                           inputShape[axis] );
            throw MarabouError( MarabouError::ONNX_PARSER_ERROR, errorMessage.ascii() );
        }
        outputShape.eraseAt( axis );
    }

    // Update the maps
    _shapeMap[outputNodeName] = outputShape;
    _varMap[outputNodeName] = _varMap[inputNodeName];
}


/**
 * @brief Processes a "unsqueeze" node in the network.
 * Implements https://github.com/onnx/onnx/blob/main/docs/Operators.md#Unsqueeze
 *
 * @param node The ONNX node
 */
void OnnxParser::unsqueeze( onnx::NodeProto &node )
{
    String inputNodeName = node.input()[0];
    String outputNodeName = node.output()[0];

    // Get the input shape
    TensorShape inputShape = _shapeMap[inputNodeName];

    // Get the axes to unsqueeze
    String axisName = node.input()[1];
    if ( !_constantIntTensors.exists( axisName ) )
    {
        missingNodeError( axisName );
    }
    Vector<int64_t> axes = _constantIntTensors[axisName];

    // Calculate a sorted list of unsigned indices
    unsigned int outputShapeSize = inputShape.size() + axes.size();
    Vector<TensorIndex> unsignedAxes;
    for ( int64_t index : axes )
    {
        SignedTensorIndex signedIndex = static_cast<int>( index );
        unsignedAxes.append( unsignIndex( outputShapeSize, signedIndex ) );
    }
    unsignedAxes.sort();

    // Calculate the output shape
    TensorShape outputShape = Vector<unsigned int>( inputShape );
    for ( TensorIndex index : unsignedAxes )
    {
        outputShape.insertAt( index, 1 );
    }

    // Update the maps
    _shapeMap[outputNodeName] = outputShape;
    _varMap[outputNodeName] = _varMap[inputNodeName];
}

/**
 * @brief Function to generate equations for a BatchNormalization
 * Implements https://github.com/onnx/onnx/blob/master/docs/Operators.md#batchnormalization.
 * @param node The ONNX node
 */
void OnnxParser::batchNormEquations( onnx::NodeProto &node, bool makeEquations )
{
    String outputNodeName = node.output()[0];
    String inputNodeName = node.input()[0];

    TensorShape inputShape = _shapeMap[inputNodeName];
    ASSERT( inputShape.size() >= 2 );

    unsigned int batchSize = inputShape.get( 0 );
    unsigned int batchLength = tensorSize( inputShape ) / batchSize;
    unsigned int numberOfChannels = inputShape.get( 1 );
    unsigned int channelLength = batchLength / numberOfChannels;


    TensorShape outputShape = inputShape;
    _shapeMap[outputNodeName] = outputShape;

    if ( !makeEquations )
        return;

    // Get inputs
    double epsilon = getFloatAttribute( node, "epsilon", 1e-05 );
    std::string scalesName = node.input()[1];
    std::string biasesName = node.input()[2];
    std::string inputMeansName = node.input()[3];
    std::string inputVariancesName = node.input()[4];
    Vector<double> scales = _constantFloatTensors[scalesName];
    Vector<double> biases = _constantFloatTensors[biasesName];
    Vector<double> inputMeans = _constantFloatTensors[inputMeansName];
    Vector<double> inputVariances = _constantFloatTensors[inputVariancesName];

    ASSERT( scales.size() == numberOfChannels );
    ASSERT( biases.size() == numberOfChannels );
    ASSERT( inputMeans.size() == numberOfChannels );
    ASSERT( inputVariances.size() == numberOfChannels );

    // Get variables
    Vector<Variable> inputVars = _varMap[inputNodeName];
    Vector<Variable> outputVars = makeNodeVariables( outputNodeName, false );
    ASSERT( inputVars.size() == tensorSize( inputShape ) );
    ASSERT( outputVars.size() == tensorSize( outputShape ) );

    for ( unsigned int i = 0; i < inputVars.size(); i++ )
    {
        unsigned int channel = ( i % batchLength ) / channelLength;
        double scale = scales[channel];
        double bias = biases[channel];
        double inputMean = inputMeans[channel];
        double inputVariance = inputVariances[channel];

        Equation e = Equation();
        e.addAddend( -1, outputVars[i] );
        e.addAddend( 1 / sqrt( inputVariance + epsilon ) * scale, inputVars[i] );
        e.setScalar( inputMean / sqrt( inputVariance + epsilon ) * scale - bias );
        _query.addEquation( e );
    }
}

/**
 * @brief Function to generate equations for a MaxPool node.
 * Implements https://github.com/onnx/onnx/blob/main/docs/Operators.md#MaxPool
 *
 * @param node ONNX node representing the MaxPool operation
 * @param makeEquations True if we need to create new variables and add new Relus
 */
void OnnxParser::maxPoolEquations( onnx::NodeProto &node, [[maybe_unused]] bool makeEquations )
{
    String inputNodeName = node.input()[0];
    String outputNodeName = node.output()[0];

    // Extract attributes and define shape
    TensorShape inputShape = _shapeMap[inputNodeName];
    if ( inputShape.size() != 4 )
    {
        String errorMessage = Stringf(
            "Currently the Onnx '%s' operation with inputs of dimensions not equal to '%d'.",
            node.op_type().c_str(),
            inputShape.size() );
        throw MarabouError( MarabouError::ONNX_PARSER_ERROR, errorMessage.ascii() );
    }
    int widthIndex = inputShape.size() - 2;
    int heightIndex = inputShape.size() - 1;
    unsigned int inputWidth = inputShape[widthIndex];
    unsigned int inputHeight = inputShape[heightIndex];

    // Get auto_pad (deprecated)
    String defaultAutoPad = "NOTSET";
    String autoPad = getStringAttribute( node, "auto_pad", defaultAutoPad );
    if ( autoPad != defaultAutoPad )
    {
        unimplementedAttributeError( node, "auto_pad" );
    }

    // Get ceil_mode
    int defaultCeilMode = 0;
    int ceilMode = getIntAttribute( node, "ceil_mode", defaultCeilMode );

    // Get dilations
    Vector<unsigned int> defaultDilations = { 1, 1 };
    Vector<unsigned int> dilations =
        getNonNegativeIntsAttribute( node, "dilations", defaultDilations );
    for ( auto d : dilations )
    {
        if ( d != 1 )
        {
            unimplementedAttributeError( node, "dilations" );
        }
    }

    // Get the kernel shape (required)
    TensorShape defaultKernelShape = { 1, 1 };
    TensorShape kernelShape =
        getNonNegativeIntsAttribute( node, "kernel_shape", defaultKernelShape );

    // Get the pads
    Vector<unsigned int> defaultPads = { 0, 0, 0, 0 };
    Vector<unsigned int> pads = getNonNegativeIntsAttribute( node, "pads", defaultPads );
    if ( pads.size() == 0 )
    {
        String errorMessage =
            Stringf( "Unexpected padding length '%d' for the Onnx '%s' operation.",
                     node.op_type().c_str(),
                     pads.size() );
        throw MarabouError( MarabouError::ONNX_PARSER_ERROR, errorMessage.ascii() );
    }
    int padWidth = pads[0] + pads[1];
    int padHeight = pads[2] + pads[3];

    // Get storage order
    int defaultStorageOrder = 0;
    int storageOrder = getIntAttribute( node, "storage_order", defaultStorageOrder );
    if ( storageOrder != defaultStorageOrder )
    {
        unimplementedAttributeError( node, "storage_order" );
    }

    // Get strides
    Vector<unsigned int> defaultStrides = { 1, 1 };
    Vector<unsigned int> strides = getNonNegativeIntsAttribute( node, "strides", defaultStrides );

    // Calculate the outputs shape
    TensorShape outputShape = Vector<unsigned int>( inputShape );

    float unroundedOutputWidth =
        ( ( (float)( inputWidth + padWidth - ( ( kernelShape[0] - 1 ) * dilations[0] + 1 ) ) ) /
          ( (float)strides[0] ) ) +
        1;
    float unroundedOutputHeight =
        ( ( (float)( inputHeight + padHeight - ( ( kernelShape[1] - 1 ) * dilations[1] + 1 ) ) ) /
          ( (float)strides[1] ) ) +
        1;

    if ( ceilMode == 0 )
    {
        outputShape[widthIndex] = (int)std::floor( unroundedOutputWidth );
        outputShape[heightIndex] = (int)std::floor( unroundedOutputHeight );
    }
    else
    {
        outputShape[widthIndex] = (int)std::ceil( unroundedOutputWidth );
        outputShape[heightIndex] = (int)std::ceil( unroundedOutputHeight );
    }

    _shapeMap[outputNodeName] = outputShape;
    if ( !makeEquations )
        return;

    // Make equations
    Vector<Variable> inputVars = _varMap[inputNodeName];
    Vector<Variable> outputVars = makeNodeVariables( outputNodeName, false );
    for ( TensorIndex i = 0; i < outputShape[widthIndex]; i++ )
    {
        for ( TensorIndex j = 0; j < outputShape[heightIndex]; j++ )
        {
            TensorIndex diStart = strides[0] * i;
            TensorIndex diEnd = std::min( diStart + kernelShape[0], inputWidth );
            TensorIndex djStart = strides[1] * j;
            TensorIndex djEnd = std::min( djStart + kernelShape[1], inputHeight );

            for ( TensorIndex k = 0; k < outputShape[1]; k++ )
            {
                TensorIndices outputVarIndices = { 0, k, i, j };
                Variable outputVar = tensorLookup( outputVars, outputShape, outputVarIndices );

                Set<Variable> maxInputVars = Set<Variable>();
                for ( TensorIndex di = diStart; di < diEnd; di++ )
                {
                    for ( TensorIndex dj = djStart; dj < djEnd; dj++ )
                    {
                        TensorIndices maxInputVarIndices = { 0, k, di, dj };
                        Variable maxInputVar =
                            tensorLookup( inputVars, inputShape, maxInputVarIndices );
                        maxInputVars.insert( maxInputVar );
                    }
                }
                _query.addMaxConstraint( outputVar, maxInputVars );
            }
        }
    }
}

/**
 * @brief Function to generate equations corresponding to
 * a convolution operation.
 * Implements https://github.com/onnx/onnx/blob/main/docs/Operators.md#Conv
 *
 * @param node ONNX node representing the operation
 * @param makeEquations True if we need to create new variables
 */
void OnnxParser::convEquations( onnx::NodeProto &node, [[maybe_unused]] bool makeEquations )
{
    String outputNodeName = node.output()[0];

    // Get input shape information
    // First input should be variable tensor
    String inputNodeName = node.input()[0];
    TensorShape inputShape = _shapeMap[inputNodeName];
    [[maybe_unused]] unsigned int inputChannels = inputShape[1];
    unsigned int inputWidth = inputShape[2];
    unsigned int inputHeight = inputShape[3];

    // Second input should be a weight matrix defining filters
    String filterNodeName = node.input()[1];
    TensorShape filterShape = _shapeMap[filterNodeName];
    unsigned int numberOfFilters = filterShape[0];
    unsigned int filterChannels = filterShape[1];
    unsigned int filterWidth = filterShape[2];
    unsigned int filterHeight = filterShape[3];

    // The number of channels should match between input variable and filters
    ASSERT( inputChannels == filterChannels );

    // Extract convolution stride information
    Vector<unsigned int> defaultStrides = { 1, 1 };
    Vector<unsigned int> strides = getNonNegativeIntsAttribute( node, "strides", defaultStrides );
    unsigned int strideWidth = strides[0];
    unsigned int strideHeight = strides[1];

    // Extract the padding information
    String defaultAutoPad = "NOTSET";
    String autoPad = getStringAttribute( node, "auto_pad", defaultAutoPad );
    unsigned int padLeft, padBottom, padRight, padTop;
    if ( autoPad == "NOTSET" )
    {
        Vector<unsigned int> defaultPads = { 0, 0, 0, 0 };
        Vector<unsigned int> pads = getNonNegativeIntsAttribute( node, "pads", defaultPads );
        padLeft = pads[0];
        padBottom = pads[1];
        padRight = pads[2];
        padTop = pads[3];
    }
    else if ( autoPad == "VALID" )
    {
        // I think this is the right thing to do the spec is very unclear.
        // See https://github.com/onnx/onnx/issues/3254 and
        // https://stackoverflow.com/questions/37674306/what-is-the-difference-between-same-and-valid-padding-in-tf-nn-max-pool-of-t
        // for details.
        padLeft = 0;
        padBottom = 0;
        padRight = 0;
        padTop = 0;
    }
    else if ( autoPad == "SAME_UPPER" || autoPad == "SAME_LOWER" )
    {
        bool padFrontPreferentially = autoPad == "SAME_LOWER";

        Padding horizontalPadding =
            calculatePaddingNeeded( inputWidth, filterWidth, strideWidth, padFrontPreferentially );
        padLeft = horizontalPadding.padFront;
        padRight = horizontalPadding.padBack;

        Padding verticalPadding = calculatePaddingNeeded(
            inputHeight, filterHeight, strideHeight, padFrontPreferentially );
        padBottom = verticalPadding.padFront;
        padTop = verticalPadding.padBack;
    }
    else
    {
        String errorMessage =
            Stringf( "Onnx '%s' operation has an unsupported value '%s' for attribute 'auto_pad'.",
                     node.op_type().c_str(),
                     autoPad.ascii() );
        throw MarabouError( MarabouError::ONNX_PARSER_ERROR, errorMessage.ascii() );
    }

    // Extract the group information (not supported/used)
    int defaultGroup = 1;
    int group = getIntAttribute( node, "group", defaultGroup );
    if ( group != defaultGroup )
    {
        unimplementedAttributeError( node, "group" );
    }

    // Extract the dilations information (not supported/used)
    Vector<unsigned int> defaultDilations = { 1, 1 };
    Vector<unsigned int> dilations =
        getNonNegativeIntsAttribute( node, "dilations", defaultDilations );
    for ( auto d : dilations )
    {
        if ( d != 1 )
        {
            unimplementedAttributeError( node, "dilations" );
        }
    }

    // Compute output shape
    unsigned int outWidth = ( inputWidth - filterWidth + padLeft + padRight ) / strideWidth + 1;
    unsigned int outHeight = ( inputHeight - filterHeight + padBottom + padTop ) / strideHeight + 1;
    unsigned int outChannels = numberOfFilters;
    TensorShape outputShape = { inputShape[0], outChannels, outWidth, outHeight };
    _shapeMap[outputNodeName] = outputShape;

    if ( !makeEquations )
        return;

    // Generate equations
    Vector<Variable> inputVars = _varMap[inputNodeName];
    Vector<double> filter = _constantFloatTensors[filterNodeName];
    Vector<Variable> outputVars = makeNodeVariables( outputNodeName, false );

    // The third input is optional and specifies a bias for each filter
    // Bias is 0 if third input is not given
    Vector<double> biases;
    TensorShape biasShape = { numberOfFilters };
    if ( node.input().size() == 3 )
    {
        String biasName = node.input()[2];
        biases = _constantFloatTensors[biasName];
    }
    else
    {
        for ( unsigned int i = 0; i < numberOfFilters; i++ )
        {
            biases.append( 0 );
        }
    }

    // There is one equation for every output variable
    for ( TensorIndex i = 0; i < outWidth; i++ )
    {
        for ( TensorIndex j = 0; j < outHeight; j++ )
        {
            for ( TensorIndex k = 0; k < outChannels; k++ ) // Out_channel corresponds to filter
                                                            // number
            {
                Equation e = Equation();

                // The equation convolves the filter with the specified input region
                // Iterate over the filter
                for ( TensorIndex di = 0; di < filterWidth; di++ )
                {
                    for ( TensorIndex dj = 0; dj < filterHeight; dj++ )
                    {
                        for ( TensorIndex dk = 0; dk < filterChannels; dk++ )
                        {
                            TensorIndex wIndex = strideWidth * i + di - padLeft;
                            TensorIndex hIndex = strideHeight * j + dj - padBottom;
                            // No need for checking greater than 0 because unsigned ints wrap
                            // around.
                            if ( hIndex < inputHeight && wIndex < inputWidth )
                            {
                                TensorIndices inputVarIndices = { 0, dk, wIndex, hIndex };
                                Variable inputVar =
                                    tensorLookup( inputVars, inputShape, inputVarIndices );
                                TensorIndices weightIndices = { k, dk, di, dj };
                                double weight = tensorLookup( filter, filterShape, weightIndices );
                                e.addAddend( weight, inputVar );
                            }
                        }
                    }
                }

                // Add output variable
                TensorIndices outputVarIndices = { 0, k, i, j };
                Variable outputVar = tensorLookup( outputVars, outputShape, outputVarIndices );
                e.addAddend( -1, outputVar );
                e.setScalar( -biases[k] );
                _query.addEquation( e );
            }
        }
    }
}

/**
 * @brief Function to generate equations corresponding to
 * a GeMM (General Matrix Multiplication) operation.
 * Implements https://github.com/onnx/onnx/blob/main/docs/Operators.md#Gemm
 *
 * @param node ONNX node representing the operation
 * @param makeEquations True if we need to create new variables
 */
void OnnxParser::gemmEquations( onnx::NodeProto &node, bool makeEquations )
{
    String outputNodeName = node.output()[0];

    // Get inputs
    String input1NodeName = node.input()[0];
    String input2NodeName = node.input()[1];
    String biasNodeName = node.input()[2];

    TensorShape input1Shape = _shapeMap[input1NodeName];
    TensorShape input2Shape = _shapeMap[input2NodeName];
    TensorShape biasShape = _shapeMap[biasNodeName];

    ASSERT( input1Shape.size() == 2 );
    ASSERT( input2Shape.size() == 2 );

    // Transpose first two inputs if needed,
    // and save scaling parameters alpha and beta if set.
    int transA = getIntAttribute( node, "transA", 0 );
    int transB = getIntAttribute( node, "transB", 0 );

    Permutation reversePerm = { 1, 0 };
    TensorShape finalInput1Shape =
        transA != 0 ? transposeVector( input1Shape, reversePerm ) : input1Shape;
    TensorShape finalInput2Shape =
        transB != 0 ? transposeVector( input2Shape, reversePerm ) : input2Shape;

    ASSERT( finalInput1Shape[1] == finalInput2Shape[0] );
    TensorShape outputShape = { finalInput1Shape[0], finalInput2Shape[1] };
    _shapeMap[outputNodeName] = outputShape;

    if ( !makeEquations )
        return;

    double alpha = getFloatAttribute( node, "alpha", 1.0 );
    double beta = getFloatAttribute( node, "beta", 1.0 );

    // Assume that first input is variables, second is Matrix for MatMul, and third is bias addition
    Vector<Variable> inputVariables = _varMap[input1NodeName];
    Vector<double> matrix = _constantFloatTensors[input2NodeName];
    Vector<double> biases = _constantFloatTensors[biasNodeName];

    // Transpose inputs
    if ( transA != 0 )
    {
        inputVariables = transposeTensor( inputVariables, input1Shape, reversePerm );
    }
    if ( transB != 0 )
    {
        matrix = transposeTensor( matrix, input2Shape, reversePerm );
    }

    // Create new variables
    Vector<Variable> outputVariables = makeNodeVariables( outputNodeName, false );

    // Generate equations
    for ( TensorIndex i = 0; i < finalInput1Shape[0]; i++ )
    {
        for ( TensorIndex j = 0; j < finalInput2Shape[1]; j++ )
        {
            Equation e = Equation();
            for ( TensorIndex k = 0; k < finalInput1Shape[1]; k++ )
            {
                double coefficient = alpha * tensorLookup( matrix, finalInput2Shape, { k, j } );
                Variable inputVariable = tensorLookup( inputVariables, finalInput1Shape, { i, k } );
                e.addAddend( coefficient, inputVariable );
            }
            // Set the bias
            TensorIndices biasIndices = broadcastIndex( biasShape, outputShape, { i, j } );
            double bias = beta * tensorLookup( biases, biasShape, biasIndices );
            e.setScalar( -bias );

            // Put output variable as the last addend
            Variable outputVariable = tensorLookup( outputVariables, outputShape, { i, j } );
            e.addAddend( -1, outputVariable );
            _query.addEquation( e );
        }
    }
}

/**
 * @brief Function to generate equations corresponding to pointwise Relu
 * Implements https://github.com/onnx/onnx/blob/main/docs/Operators.md#Relu
 *
 * @param node ONNX node representing the Relu operation
 * @param makeEquations True if we need to create new variables and add new Relus
 */
void OnnxParser::reluEquations( onnx::NodeProto &node, bool makeEquations )
{
    String outputNodeName = node.output()[0];
    String inputNodeName = node.input()[0];

    _shapeMap[outputNodeName] = _shapeMap[inputNodeName];
    if ( !makeEquations )
        return;

    // Get variables
    Vector<Variable> inputVars = _varMap[inputNodeName];
    Vector<Variable> outputVars = makeNodeVariables( outputNodeName, false );
    ASSERT( inputVars.size() == outputVars.size() );

    // Generate equations
    for ( PackedTensorIndices i = 0; i < inputVars.size(); i++ )
    {
        int inputVar = inputVars[i];
        int outputVar = outputVars[i];
        _query.addRelu( inputVar, outputVar );
    }
}

/**
 * @brief Function to generate equations corresponding to leaky pointwise Relu
 * Implements https://github.com/onnx/onnx/blob/main/docs/Operators.md#LeakyRelu
 *
 * @param node ONNX node representing the LeakyRelu operation
 * @param makeEquations True if we need to create new variables and add new LeakyRelus
 */
void OnnxParser::leakyReluEquations( onnx::NodeProto &node, bool makeEquations )
{
    String outputNodeName = node.output()[0];
    String inputNodeName = node.input()[0];

    _shapeMap[outputNodeName] = _shapeMap[inputNodeName];
    if ( !makeEquations )
        return;

    // Get alpha attribute
    float alpha = getFloatAttribute( node, "alpha", 0.01 );

    // Get variables
    Vector<Variable> inputVars = _varMap[inputNodeName];
    Vector<Variable> outputVars = makeNodeVariables( outputNodeName, false );
    ASSERT( inputVars.size() == outputVars.size() );

    // Generate equations
    for ( PackedTensorIndices i = 0; i < inputVars.size(); i++ )
    {
        _query.addLeakyRelu( inputVars[i], outputVars[i], alpha );
    }
}

/**
 * @brief Function to generate equations corresponding to addition (and subtraction)
 * Implements https://github.com/onnx/onnx/blob/main/docs/Operators.md#Add (with args 1.0, 1.0)
 * Implements https://github.com/onnx/onnx/blob/main/docs/Operators.md#Sub (with args 1.0, -1.0)
 *
 * @param node ONNX node representing the Add operation
 * @param makeEquations True if we need to create new variables and write Marabou equations
 */
void OnnxParser::scaleAndAddEquations( onnx::NodeProto &node,
                                       bool makeEquations,
                                       double coefficient1,
                                       double coefficient2 )
{
    String outputName = node.output()[0];
    String input1Name = node.input()[0];
    String input2Name = node.input()[1];

    // Get the broadcasted shape
    TensorShape input1Shape = _shapeMap[input1Name];
    TensorShape input2Shape = _shapeMap[input2Name];

    TensorShape outputShape = getMultidirectionalBroadcastShape( input1Shape, input2Shape );
    _shapeMap[outputName] = outputShape;

    if ( !makeEquations )
        return;

    // Decide which inputs are variables and which are constants
    bool input1IsConstant = isConstantNode( input1Name );
    bool input2IsConstant = isConstantNode( input2Name );

    // If both inputs to add are constant, then the output is constant too.
    // No new variables are needed, we could just store the output.
    if ( input1IsConstant && input2IsConstant )
    {
        String errorMessage =
            "Addition of constant tensors not yet supported for command-line Onnx files";
        throw MarabouError( MarabouError::ONNX_PARSER_ERROR, errorMessage.ascii() );
    }

    // If both inputs are variables, then we need a new variable to represent
    // the sum of the two variables
    if ( !input1IsConstant && !input2IsConstant )
    {
        Vector<Variable> outputVariables = makeNodeVariables( outputName, false );
        Vector<Variable> input1Variables = _varMap[input1Name];
        Vector<Variable> input2Variables = _varMap[input2Name];
        if ( input1Variables.size() != input2Variables.size() ||
             input2Variables.size() != outputVariables.size() )
        {
            String errorMessage = "Broadcast support for addition of two non-constant nodes not "
                                  "yet supported for command-line ONNX files";
            throw MarabouError( MarabouError::ONNX_PARSER_ERROR, errorMessage.ascii() );
        }

        for ( PackedTensorIndices i = 0; i < input1Variables.size(); i++ )
        {
            Equation e = Equation();
            e.addAddend( coefficient1, input1Variables[i] );
            e.addAddend( coefficient2, input2Variables[i] );
            e.addAddend( -1, outputVariables[i] );
            e.setScalar( 0.0 );
            _query.addEquation( e );
        }
        return;
    }

    // Otherwise, we are adding constants to variables.
    // We don't need new equations or new variables if the input variable is
    // the output of a linear equation. Instead, we can just edit the scalar
    // term of the existing linear equation. However, if the input variables
    // are not outputs of linear equations (input variables or outputs of
    // activation functions) then we will need new equations.
    String constantName = input1IsConstant ? input1Name : input2Name;
    String variableName = input1IsConstant ? input2Name : input1Name;
    TensorShape inputConstantsShape = input1IsConstant ? input1Shape : input2Shape;
    TensorShape inputVariablesShape = input1IsConstant ? input2Shape : input1Shape;
    Vector<double> inputConstants = _constantFloatTensors[constantName];
    Vector<Variable> inputVariables = _varMap[variableName];
    double constantCoefficient = input1IsConstant ? coefficient1 : coefficient2;
    double variableCoefficient = input1IsConstant ? coefficient2 : coefficient1;

    // Adjust equations to incorporate the constant addition
    unsigned int numberOfEquationsChanged = 0;
    unsigned int numberOfOutputVariables = tensorSize( outputShape );
    for ( PackedTensorIndices i = 0; i < numberOfOutputVariables; i++ )
    {
        TensorIndices outputIndices = unpackIndex( outputShape, i );

        TensorIndices inputVariableIndices =
            broadcastIndex( inputVariablesShape, outputShape, outputIndices );
        Variable inputVariable =
            tensorLookup( inputVariables, inputVariablesShape, inputVariableIndices );

        Equation *equation = _query.findEquationWithOutputVariable( inputVariable );
        if ( equation )
        {
            TensorIndices inputConstantIndices =
                broadcastIndex( inputConstantsShape, outputShape, outputIndices );
            double inputConstant =
                tensorLookup( inputConstants, inputConstantsShape, inputConstantIndices );

            double currentVariableCoefficient = equation->getCoefficient( inputVariable );
            equation->setCoefficient( inputVariable,
                                      variableCoefficient * currentVariableCoefficient );
            equation->setScalar( equation->_scalar - constantCoefficient * inputConstant );

            numberOfEquationsChanged += 1;
        }
    }

    if ( numberOfEquationsChanged == inputVariables.size() )
    {
        // If we changed one equation for every input variable, then
        // we don't need any new equations
        _varMap[outputName] = inputVariables;
    }
    else
    {
        // Otherwise, assert no equations were changed,
        // and we need to create new equations
        ASSERT( numberOfEquationsChanged == 0 );
        Vector<Variable> outputVariables = makeNodeVariables( outputName, false );
        for ( PackedTensorIndices i = 0; i < outputVariables.size(); i++ )
        {
            Equation e = Equation();
            e.addAddend( variableCoefficient, inputVariables[i] );
            e.addAddend( -1, outputVariables[i] );
            e.setScalar( -constantCoefficient * inputConstants[i] );
            _query.addEquation( e );
        }
    }
}

/**
 * @brief Function to generate equations corresponding to matrix multiplication.
 * Implements https://github.com/onnx/onnx/blob/main/docs/Operators.md#MatMul
 *
 * @param node ONNX node representing the MatMul operation
 * @param makeEquations True if we need to create new variables and write Marabou equations
 */
void OnnxParser::matMulEquations( onnx::NodeProto &node, bool makeEquations )
{
    String nodeName = node.output()[0];

    // Get inputs
    String input1Name = node.input()[0];
    String input2Name = node.input()[1];
    TensorShape input1Shape = _shapeMap[input1Name];
    TensorShape input2Shape = _shapeMap[input2Name];

    ASSERT( input1Shape.last() == input2Shape.first() );
    ASSERT( input1Shape.size() <= 2 );
    ASSERT( input2Shape.size() <= 2 );

    // Calculate the output shape
    TensorShape outputShape;
    for ( unsigned int i = 0; i < input1Shape.size() - 1; i++ )
    {
        outputShape.append( input1Shape[i] );
    }
    for ( unsigned int i = 1; i < input2Shape.size(); i++ )
    {
        outputShape.append( input2Shape[i] );
    }

    _shapeMap.insert( nodeName, outputShape );
    if ( !makeEquations )
        return;

    // Assume that at exactly one input is a constant as
    // we cannot represent variable products with linear equations.
    bool input1IsConstant = isConstantNode( input1Name );
    bool input2IsConstant = isConstantNode( input2Name );

    // If both inputs are constant, than the output is constant as well, and we don't need new
    // variables or equations
    if ( input1IsConstant && input2IsConstant )
    {
        String errorMessage = "Matrix multiplication of constant tensors not yet implemented for "
                              "command-line Onnx files";
        throw MarabouError( MarabouError::ONNX_PARSER_ERROR, errorMessage.ascii() );
    }

    if ( !input1IsConstant && !input2IsConstant )
    {
        String errorMessage = "Matrix multiplication of variable tensors is not supported";
        throw MarabouError( MarabouError::ONNX_PARSER_ERROR, errorMessage.ascii() );
    }

    String constantName = input1IsConstant ? input1Name : input2Name;
    TensorShape constantShape = input1IsConstant ? input1Shape : input2Shape;
    Vector<double> constants = _constantFloatTensors[constantName];

    String variableName = input1IsConstant ? input2Name : input1Name;
    Vector<Variable> variables = _varMap[variableName];

    // Create new variables
    Vector<Variable> outputVariables = makeNodeVariables( nodeName, false );

    // Pad the output if needed (matrix-matrix multiplication)
    if ( outputShape.size() == 1 && input2Shape.size() > 1 )
    {
        outputShape.insertHead( 1 );
    }

    unsigned int d1 = input1Shape.size() == 1 ? 1 : input1Shape.first();
    unsigned int d2 = input1Shape.last();
    unsigned int d3 = input2Shape.last();

    // Generate equations
    for ( TensorIndex i = 0; i < d1; i++ )
    {
        // Differentiate between matrix-vector multiplication
        // and matrix-matrix multiplication
        if ( input2Shape.size() == 2 )
        {
            for ( TensorIndex j = 0; j < d3; j++ )
            {
                Equation e;
                for ( TensorIndex k = 0; k < d2; k++ )
                {
                    double constant;
                    Variable variable;
                    if ( input1IsConstant )
                    {
                        constant = tensorLookup( constants, { d1, d2 }, { i, k } );
                        variable = tensorLookup( variables, { d2, d3 }, { k, j } );
                    }
                    else
                    {
                        constant = tensorLookup( constants, { d2, d3 }, { k, j } );
                        variable = tensorLookup( variables, { d1, d2 }, { i, k } );
                    }
                    e.addAddend( constant, variable );
                }

                // Put output variable as the last addend
                Variable outputVariable = tensorLookup( outputVariables, outputShape, { i, j } );
                e.addAddend( -1, outputVariable );
                e.setScalar( 0.0 );
                _query.addEquation( e );
            }
        }
        else
        {
            Equation e;
            for ( TensorIndex k = 0; k < d2; k++ )
            {
                double constant;
                Variable variable;
                if ( input1IsConstant )
                {
                    constant = tensorLookup( constants, { d1, d2 }, { i, k } );
                    variable = variables[k];
                }
                else
                {
                    constant = constants[k];
                    variable = tensorLookup( variables, { d1, d2 }, { i, k } );
                }
                e.addAddend( constant, variable );
            }

            // Put output variable as the last addend last
            Variable outputVariable = outputVariables[i];
            e.addAddend( -1, outputVariable );
            e.setScalar( 0.0 );
            _query.addEquation( e );
        }
    }
}

/**
 * @brief Function to generate equations corresponding to a sigmoid node.
 * Implements https://github.com/onnx/onnx/blob/main/docs/Operators.md#Sigmoid
 *
 * @param node ONNX node representing the Sigmoid operation
 * @param makeEquations True if we need to create new variables and write Marabou equations
 */
void OnnxParser::sigmoidEquations( onnx::NodeProto &node, bool makeEquations )
{
    String outputNodeName = node.output()[0];
    String inputNodeName = node.input()[0];
    _shapeMap[outputNodeName] = _shapeMap[inputNodeName];

    if ( !makeEquations )
        return;

    // Get variables
    Vector<Variable> inputVars = _varMap[inputNodeName];
    Vector<Variable> outputVars = makeNodeVariables( outputNodeName, false );
    ASSERT( inputVars.size() == outputVars.size() );

    // Generate equations
    for ( unsigned int i = 0; i < inputVars.size(); i++ )
    {
        Variable inputVar = inputVars[i];
        Variable outputVar = outputVars[i];
        _query.addSigmoid( inputVar, outputVar );
    }
}

/**
 * @brief Function to generate equations corresponding to a tanh node.
 * Implements https://github.com/onnx/onnx/blob/main/docs/Operators.md#Tanh
 *
 * @param node ONNX node representing the Tanh operation
 * @param makeEquations True if we need to create new variables and write Marabou equations
 */
void OnnxParser::tanhEquations( onnx::NodeProto &node, bool makeEquations )
{
    String outputName = node.output()[0];
    String inputName = node.input()[0];
    TensorShape inputShape = _shapeMap[inputName];

    _shapeMap[outputName] = _shapeMap[inputName];
    if ( !makeEquations )
        return;

    // Get variables
    Vector<Variable> inputVars = _varMap[inputName];
    Vector<Variable> outputVars = makeNodeVariables( outputName, false );
    ASSERT( inputVars.size() == outputVars.size() );

    // Generate equations
    for ( uint i = 0; i < outputVars.size(); i++ )
    {
        _query.addTanh( inputVars[i], outputVars[i] );
    }
}
