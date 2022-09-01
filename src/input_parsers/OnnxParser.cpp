/*********************                                                        */
/*! \file OnnxParser.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Matthew Daggitt
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
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
 **   This explains why all the methods takes the names of things as inputs
 **   rather than passing references to onnx::NodeProto as one might expect.
 **   Likewise, this is why we store the shape data in a separate data structure
 **   rather than simply retrieving it when needed.
**/

#include "OnnxParser.h"
#include "FloatUtils.h"
#include "InputParserError.h"
#include "InputQuery.h"
#include "MString.h"
#include "ReluConstraint.h"
#include "onnx.proto3.pb.h"
#include <fstream>
#include <assert.h>
#include <iostream>
#include <math.h>

/******************
 * Public methods *
 ******************/

OnnxParser::OnnxParser(const String &path)
{
    // open file and move current position in file to the end
    std::ifstream input(path.ascii(), std::ios::ate | std::ios::binary);
    // get current position in file
    std::streamsize size = input.tellg();
    // move to start of file
    input.seekg(0, std::ios::beg);

    // read raw data
    Vector<char> buffer(size);
    input.read(buffer.data(), size);

    // parse protobuf
    onnx::ModelProto model;
    model.ParseFromArray(buffer.data(), size);
    _network = model.graph();

    initialiseMaps();
}
/**
 * @brief Generates the variables for a query over the whole network.
 *
 * @param query The query object to be populated.
 */
void OnnxParser::generateQuery( InputQuery& query )
{
    Set<String> inputNames = readInputNames();
    String outputName = readOutputName();
    processGraph(inputNames, outputName, query);
}

/**
 * @brief Generates the variables for a query over only a subset of the network.
 *
 * @param query The query object to be populated
 * @param inputNames The set of input nodes. Note these don't have to be an actual input to the
 * network, they can be intermediate nodes.
 * @param outputName The output node. Note that again this doesn't have to be an actual output of
 * the network, it can be an intermediate node.
 */
void OnnxParser::generatePartialQuery( InputQuery& query, Set<String>& inputNames, String& outputName)
{
    validateUserInputNames(inputNames);
    validateUserOutputNames(outputName);
    processGraph(inputNames, outputName, query);
}

/*************
 * Utilities *
 *************/

void illTypedAttributeError(onnx::NodeProto &node, const onnx::AttributeProto& attr, onnx::AttributeProto_AttributeType expectedType)
{
    String errorMessage = Stringf("Expected attribute %s on Onnx node of type %s to be of type %s but is actually of type %s", attr.name(), node.op_type().c_str(), expectedType, attr.type());
    throw MarabouError(MarabouError::ONNX_PARSE_ERROR, errorMessage.ascii());
}

void missingAttributeError(onnx::NodeProto &node, String attributeName)
{
    String errorMessage = Stringf("Onnx node of type %s is missing the expected attribute %s", node.op_type().c_str(), attributeName.ascii());
    throw MarabouError(MarabouError::ONNX_PARSE_ERROR, errorMessage.ascii());
}

void unimplementedError(onnx::NodeProto &node)
{
    String errorMessage = Stringf("Onnx operation %s not yet implemented for command line support. Should be relatively easy to add.", node.op_type().c_str());
    throw MarabouError(MarabouError::ONNX_PARSE_ERROR, errorMessage.ascii());
}

void unsupportedError(onnx::NodeProto &node)
{
    String errorMessage = Stringf("Onnx operation %s not currently supported by Marabou", node.op_type().c_str());
    throw MarabouError(MarabouError::ONNX_PARSE_ERROR, errorMessage.ascii());
}

template <typename T>
T lookup2D(Vector<T> array, int i1, int i2, [[maybe_unused]] int rows, int cols)
{
    assert(array.size() == (uint) (rows * cols));
    return array[i1 * cols + i2];
}

void checkTensorDataSourceIsInternal(const onnx::TensorProto& tensor)
{
    if (tensor.data_location() == onnx::TensorProto_DataLocation_EXTERNAL)
    {
        String errorMessage = Stringf("External data locations not yet implemented for command line Onnx support");
        throw MarabouError(MarabouError::ONNX_PARSE_ERROR, errorMessage.ascii());
    }
}

void checkTensorDataType(const onnx::TensorProto& tensor, int32_t expectedDataType)
{
    int32_t actualDataType = tensor.data_type();
    if (actualDataType != expectedDataType)
    {
        std::string actualName = onnx::TensorProto_DataType_Name(actualDataType);
        std::string expectedName = onnx::TensorProto_DataType_Name(actualDataType);
        String errorMessage = Stringf("Expected tensor '%s' to be of type %s but actually of type %s", expectedName.c_str(), actualName.c_str());
        throw MarabouError(MarabouError::ONNX_PARSE_ERROR, errorMessage.ascii());
    }
}

TensorShape shapeOfInput(onnx::ValueInfoProto &input)
{
    TensorShape result;
    for (auto dim : input.type().tensor_type().shape().dim())
    {
        int size = dim.dim_value();
        // This is copied from the Python code, but it feels like an
        // error should be thrown if the size is non-positive.
        int adjustedSize = size > 0 ? size : 1;
        result.append(adjustedSize);
    }
    return result;
}

TensorShape shapeOfConstant(onnx::TensorProto &constant)
{
    TensorShape result;
    for (auto dim : constant.dims())
    {
        result.append(dim);
    }
    return result;
}

int tensorSize(TensorShape shape)
{
    int size = 1;
    for (int dimSize : shape)
    {
        size *= dimSize;
    }
    return size;
}

TensorShape getBroadcastShape(TensorShape shape1, TensorShape shape2)
{
    TensorShape output;
    auto it1 = shape1.rbegin();
    auto it2 = shape2.rbegin();
    while (it1 != shape1.rend() || it2 != shape2.rend())
    {
        int d1 = it1 == shape1.rend() ? 1 : *(it1++);
        int d2 = it2 == shape2.rend() ? 1 : *(it2++);

        if (d1 != d2)
        {
            String errorMessage = Stringf("Mismatch in tensor dimensions found while broadcasting (%d v %d)", d1, d2);
            throw MarabouError(MarabouError::ONNX_PARSE_ERROR, errorMessage.ascii());
        }

        output.insertHead(d1);
    }
    return output;
}

/**
 * @brief Fills in the variables (values 0 and -1) in the new shape template.
 * See https://github.com/onnx/onnx/blob/main/docs/Operators.md#Reshape for a
 * description of the variables.
 *
 * @param oldShape the previous shape of the tensor
 * @param newShapeTemplate the template for the new shape of the tensor
 * @param allowZero whether or not zero-valued dimensions are allowed. If they
 * are not then all zeroes are replaced with the corresponding value in the
 * old shape.
 * @return
 */
TensorShape instantiateReshapeTemplate(TensorShape oldShape, TensorShape newShapeTemplate, bool allowZero)
{
    TensorShape newShape;
    int inferredIndex = -1;

    for (uint i = 0; i < newShapeTemplate.size(); i++)
    {
        int dimSize = newShapeTemplate[i];
        if (dimSize == -1)
        {
            // Then this dimension should be inferred.
            // Temporarily set the dimSize to be 1 so that we
            // can use the tensorSize function to compute the
            // size so far.
            inferredIndex = i;
            dimSize = 1;
        }
        else if (dimSize == 0)
        {
            if (!allowZero)
            {
                if (i < oldShape.size())
                {
                    dimSize = oldShape[i];
                }
            }
        }
        newShape.append(dimSize);
    }

    if (inferredIndex != -1)
    {
        newShape[inferredIndex] = tensorSize(oldShape) / tensorSize(newShape);
    }
    return newShape;
}

void checkEndianness()
{
    int num = 1;
    bool systemIsLittleEndian = *(char *)&num == 1;
    if (! systemIsLittleEndian)
    {
        String errorMessage = "Support for Onnx files on non-little endian systems is not currently implemented on the command line";
        throw MarabouError(MarabouError::ONNX_PARSE_ERROR, errorMessage.ascii());
    }
}

const onnx::AttributeProto* findAttribute(onnx::NodeProto& node, String name, onnx::AttributeProto_AttributeType expectedType)
{
    for (const onnx::AttributeProto& attr : node.attribute())
    {
        if (attr.name() == name.ascii())
        {
            if (attr.type() != expectedType)
            {
                illTypedAttributeError(node, attr, expectedType);
            }
            return &attr;
        }
    }
    return nullptr;
}

float getFloatAttribute(onnx::NodeProto& node, String name, float defaultValue)
{
    const onnx::AttributeProto* attr = findAttribute(node, name, onnx::AttributeProto_AttributeType_FLOAT);
    if (attr == nullptr)
    {
        return defaultValue;
    }
    return (*attr).f();
}

int getIntAttribute(onnx::NodeProto& node, String name, int defaultValue)
{
    const onnx::AttributeProto* attr = findAttribute(node, name, onnx::AttributeProto_AttributeType_INT);
    if (attr == nullptr)
    {
        return defaultValue;
    }
    return (*attr).f();
}

const onnx::TensorProto& getTensorAttribute(onnx::NodeProto& node, String name)
{
    const onnx::AttributeProto* attr = findAttribute(node, name, onnx::AttributeProto_AttributeType_INT);
    if (attr == nullptr)
    {
        missingAttributeError(node, name);
    }
    return (*attr).t();
}

Vector<int> getIntsAttribute(onnx::NodeProto& node, String name, Vector<int>& defaultValue)
{
    const onnx::AttributeProto* attr = findAttribute(node, name, onnx::AttributeProto_AttributeType_INT);
    if (attr == nullptr)
    {
        return defaultValue;
    }

    Vector<int> result;
    for (int i = 0; i < (*attr).ints_size(); i++)
    {
        result.append((*attr).ints(i));
    }
    return result;
 }

/*******************
 * Private methods *
 *******************/

/**
 * @brief Initialises the mapping from node names to nodes and
 * checks the invariant that there are no duplicate names.
 */
void OnnxParser::initialiseMaps()
{
    Set<std::string> names;
    for (const onnx::NodeProto& node : _network.node())
    {
        const std::string name = node.name();
        if (names.exists(name))
        {
            String errorMessage = "nodes in Onnx network must have a unique name";
            throw MarabouError(MarabouError::ONNX_PARSE_ERROR, errorMessage.ascii());
        }
        else
        {
            _nodeMap.insert(name, &node);
            names.insert(name);
        }
    }

    for (const onnx::TensorProto& initializer : _network.initializer())
    {
        const std::string name = initializer.name();
        if (names.exists(name))
        {
            String errorMessage = "nodes in Onnx network must have a unique name";
            throw MarabouError(MarabouError::ONNX_PARSE_ERROR, errorMessage.ascii());
        }
        else
        {
            _constantMap.insert(name, &initializer);
            names.insert(name);
        }
    }
}

void OnnxParser::validateUserInputNames(Set<String>& inputNames)
{
    // Validate the provided inputs
    for (String inputName : inputNames)
    {
        if ( !_nodeMap.exists(inputName) )
        {
            String errorMessage = Stringf("Input %s not found in graph!", inputName.ascii());
            throw MarabouError( MarabouError::ONNX_PARSE_ERROR, errorMessage.ascii() );
        }
    }
}

void OnnxParser::validateUserOutputNames(String &outputName)
{
    for (auto node : _network.node())
    {
        for (String outputNodeName : node.output())
        {
            if (outputName == outputNodeName)
            {
                return;
            }
        }
    }

    String errorMessage = Stringf("Output %s not found in graph!", outputName);
    throw MarabouError( MarabouError::ONNX_PARSE_ERROR, errorMessage.ascii() );
}

Set<String> OnnxParser::readInputNames()
{
    assert(_network.input().size() >= 1);

    Set<String> initializerNames;
    for (auto &initNode : _network.initializer())
    {
        std::cout << "\ninitialiser: " + initNode.name();
        initializerNames.insert(initNode.name());
    }

    Set<String> inputNames;
    for (auto &inputNode : _network.input())
    {
        std::cout << "\ninput: " + inputNode.name();
        inputNames.insert(inputNode.name());
    }

    return Set<String>::difference(inputNames, initializerNames);
}

String OnnxParser::readOutputName()
{
    if (_network.output().size() > 1)
    {
        String message = "Your model has multiple outputs defined\n";
        message += "Please specify the name of the output you want to consider using the 'outputName' argument\n";
        message += "Possible options:";
        for (auto output : _network.output())
        {
            message += " " + output.name();
        }
        throw MarabouError(MarabouError::ONNX_PARSE_ERROR, message.ascii());
    }
    std::cout << "\noutput: " + _network.output()[0].name();
    return _network.output()[0].name();
}

void OnnxParser::initialiseShapeMap()
{
    // Add shapes for inputs
    for (auto input : _network.input())
    {
        String inputName = input.name();
        _shapeMap.insert(inputName, shapeOfInput(input));

        // If this is one of the specified inputs, create new variables
        if (_inputNames.exists(inputName))
        {
            _processedNodes.insert(inputName);
            _numberOfFoundInputs += 1;
            Vector<Variable> nodeVars = makeNodeVariables(inputName, true);
        }
    }

    // Add shapes for constants
    for (auto constant : _network.initializer())
    {
        String constantName = constant.name();
        _shapeMap.insert(constantName, shapeOfConstant(constant));
        _processedNodes.insert(constantName);
    }
}

void OnnxParser::validateAllInputsAndOutputsFound()
{
    // By this point, all input variables need to have been found
    if (_numberOfFoundInputs != _inputNames.size())
    {
        String errorMessage = "These input variables could not be found:";
        for (String inputName : _inputNames)
        {
            if (!_varMap.exists(inputName))
            {
                String space = " ";
                errorMessage += space + inputName;
            }
        }
        throw MarabouError(MarabouError::ONNX_PARSE_ERROR, errorMessage.ascii());
    }

    // Mark the output variables
    for (Variable var : _varMap[_outputName])
    {
        _outputVars.append(var);
    }
}

void OnnxParser::processGraph(Set<String> &inputNames, String &outputName, InputQuery& query)
{
    _inputNames = inputNames;
    _outputName = outputName;

    initialiseShapeMap();
    processNode(outputName, true);
    validateAllInputsAndOutputsFound();
    getMarabouQuery(query);
}

/**
 * @brief Recursively processes the structure of the graph, adding the relevant
 * constraints for each node as it goes. Takes in a string rather than a NodeProto
 * as we initially pass in the output which is *not* a NodeProto.
 *
 * @param node
 * @param makeEquations
 */
void OnnxParser::processNode(String &nodeName, bool makeEquations)
{
    if (_processedNodes.exists(nodeName))
    {
        return;
    }

    if (_inputNames.exists(nodeName))
    {
        _numberOfFoundInputs += 1;
        // If an inputName is an intermediate layer of the network, we don't need to create Marabou
        // equations for its inputs. However, we still need to call makeMarabouEquations in order to
        // compute shapes. We just need to set the makeEquations flag to false
        makeEquations = false;
    }

    _processedNodes.insert(nodeName);

    List<onnx::NodeProto> nodes = getNodesWithOutput(nodeName);
    assert(nodes.size() == 1);
    onnx::NodeProto& node = nodes.front();

    // First recursively process the input nodes.
    // This ensures that shapes and values of a node's inputs have been computed first.
    for (auto inputNode : getInputsToNode(node))
    {
        processNode(inputNode, makeEquations);
    }

    // Compute node's shape and create Marabou equations as needed
    makeMarabouEquations(node, makeEquations);

    // Create new variables when we find one of the inputs
    if (_inputNames.exists(nodeName))
    {
        Vector<Variable> vars = makeNodeVariables(nodeName, true);
    }
}

/**
 * @brief Assuming the node's shape is known, return a set of new variables in the same shape
 */
Vector<Variable> OnnxParser::makeNodeVariables( String &nodeName, bool isInput )
{
    assert(!_varMap.exists(nodeName));
    TensorShape shape = _shapeMap[nodeName];
    int size = tensorSize(shape);

    Vector<Variable> variables;
    for (int i = 0; i < size; i++)
    {
        Variable variable = getNewVariable();
        variables.append(variable);
        if (isInput)
        {
            _inputVars.append(variable);
        }
    }
    _varMap.insert(nodeName, variables);
    return variables;
}

List<onnx::NodeProto> OnnxParser::getNodesWithOutput(String &nodeName)
{
    List<onnx::NodeProto> nodes;
    for (auto node : _network.node())
    {
        for (auto outputName : node.output())
        {
            if (outputName == nodeName.ascii())
            {
                nodes.append(node);
                break;
            }
        }
    }
    return nodes;
}

Set<String> OnnxParser::getInputsToNode(onnx::NodeProto &node)
{
    Set<String> inputNames;
    for (String inputNodeName : node.input())
    {
        if (getNodesWithOutput(inputNodeName).size() >= 1)
        {
            inputNames.insert(inputNodeName);
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
void OnnxParser::makeMarabouEquations(onnx::NodeProto &node, bool makeEquations)
{
    printf("\nProcessing %s", node.name().c_str());
    auto nodeType = node.op_type().c_str();

    if (strcmp(nodeType, "Constant") == 0)
    {
        constant(node);
    }
    else if (strcmp(nodeType, "Identity") == 0)
    {
        identity(node);
    }
    else if (strcmp(nodeType, "Cast") == 0)
    {
        cast(node);
    }
    else if (strcmp(nodeType, "Reshape") == 0)
    {
        reshape(node);
    }
    else if (strcmp(nodeType, "Flatten") == 0)
    {
        flatten(node);
    }
    else if (strcmp(nodeType, "Transpose") == 0)
    {
        transpose(node);
    }
    else if (strcmp(nodeType, "BatchNormalization") == 0)
    {
        batchNormEquations(node, makeEquations);
    }
    else if (strcmp(nodeType, "MaxPool") == 0)
    {
        maxPoolEquations(node, makeEquations);
    }
    else if (strcmp(nodeType, "Conv") == 0)
    {
        convEquations(node, makeEquations);
    }
    else if (strcmp(nodeType, "Gemm") == 0)
    {
        gemmEquations(node, makeEquations);
    }
    else if (strcmp(nodeType, "Add") == 0)
    {
        scaleAndAddEquations(node, makeEquations, 1, 1);
    }
    else if (strcmp(nodeType, "Sub") == 0)
    {
        scaleAndAddEquations(node, makeEquations, 1, -1);
    }
    else if (strcmp(nodeType, "Relu") == 0)
    {
        reluEquations(node, makeEquations);
    }
    else if (strcmp(nodeType, "MatMul") == 0)
    {
        matMulEquations(node, makeEquations);
    }
    else if (strcmp(nodeType, "Sigmoid") == 0)
    {
        sigmoidEquations(node, makeEquations);
    }
    else
    {
        unsupportedError(node);
    }
}

/**
 * @brief Processes a constant node in the network.
 * https://github.com/onnx/onnx/blob/main/docs/Operators.md#Constant
 *
 * @param node The ONNX node
 */
void OnnxParser::constant( onnx::NodeProto& node )
{
    String outputNodeName = node.output()[0];
    const onnx::TensorProto& value = getTensorAttribute(node, "value");
    _constantMap[outputNodeName] = &value;
}

/**
 * @brief Processes an identity node in the network.
 * Implements https://github.com/onnx/onnx/blob/main/docs/Operators.md#Identity
 *
 * @param node The ONNX node
 */
void OnnxParser::identity( onnx::NodeProto& node )
{
    String outputNodeName = node.output()[0];
    String inputNodeName = node.input()[0];

    _shapeMap[outputNodeName] = _shapeMap[inputNodeName];
    if (_varMap.exists(inputNodeName))
    {
        _varMap[outputNodeName] = _varMap[inputNodeName];
    }
    else if (_constantMap.exists(inputNodeName))
    {
        _constantMap[outputNodeName] = _constantMap[inputNodeName];
    }
}


/**
 * @brief Processes a cast node in the network.
 * Implements https://github.com/onnx/onnx/blob/main/docs/Operators.md#Cast
 *
 * @param node The ONNX node
 */
void OnnxParser::cast( onnx::NodeProto& node )
{
    // See https://github.com/NeuralNetworkVerification/Marabou/blob/76b8eaf23518ca468c2cf05b742e3b4c858a64c3/maraboupy/MarabouNetworkONNX.py#L294
    // for reference implementation
    unimplementedError(node);
}

/**
 * @brief Processes an reshape node in the network.
 * Implements https://github.com/onnx/onnx/blob/main/docs/Operators.md#Reshape
 *
 * @param node The ONNX node
 */
void OnnxParser::reshape( onnx::NodeProto& node )
{
    String outputNodeName = node.output()[0];
    String inputNode1Name = node.input()[0];
    String inputNode2Name = node.input()[1];

    // Assume first input is array to be reshaped, second input is the new shape array
    TensorShape oldShape = _shapeMap[inputNode1Name];
    TensorShape newShapeTemplate;
    for (int dim : getTensorIntValues(*_constantMap[inputNode2Name]))
    {
        newShapeTemplate.append(dim);
    }
    bool allowZeroes = getIntAttribute(node, "allowzero", 0) != 0;
    TensorShape newShape = instantiateReshapeTemplate(oldShape, newShapeTemplate, allowZeroes);
    _shapeMap[outputNodeName] = newShape;

    // Transfer constants/variables
    if (_varMap.exists(inputNode1Name))
    {
        _varMap[outputNodeName] = _varMap[inputNode1Name];
    }
    else if (_constantMap.exists(inputNode1Name))
    {
        _constantMap[outputNodeName] = _constantMap[inputNode1Name];
    }
}

/**
 * @brief Processes a "flatten" node in the network.
 * Implements https://github.com/onnx/onnx/blob/main/docs/Operators.md#Flatten
 *
 * @param node The ONNX node
 */
void OnnxParser::flatten( onnx::NodeProto& node )
{
    String outputNodeName = node.output()[0];
    String inputNodeName = node.input()[0];

    // Get parameters
    int axis = getIntAttribute(node, "axis", 1);

    // Calculate output shape
    TensorShape inputShape = _shapeMap[inputNodeName];
    int dim1 = 1;
    for (int i = 0; i < axis; i++)
    {
        dim1 *= inputShape[i];
    }
    int dim2 = 1;
    for (uint i = axis; i < inputShape.size(); i++)
    {
        dim2 *= inputShape[i];
    }

    TensorShape outputShape;
    outputShape.append(dim1);
    outputShape.append(dim2);
    _shapeMap[outputNodeName] = outputShape;

    // Transfer constants/variables
    if (_varMap.exists(inputNodeName))
    {
        _varMap[outputNodeName] = _varMap[inputNodeName];
    }
    else if (_constantMap.exists(inputNodeName))
    {
        _constantMap[outputNodeName] = _constantMap[inputNodeName];
    }
}

/**
 * @brief Processes a "transpose" node in the network.
 * Implements https://github.com/onnx/onnx/blob/main/docs/Operators.md#Transpose
 *
 * @param node The ONNX node
 */
void OnnxParser::transpose( onnx::NodeProto& node )
{
    // See https://github.com/NeuralNetworkVerification/Marabou/blob/76b8eaf23518ca468c2cf05b742e3b4c858a64c3/maraboupy/MarabouNetworkONNX.py#L397
    unimplementedError(node);
}

/**
 * @brief Function to generate equations for a BatchNormalization
 * Implements https://github.com/onnx/onnx/blob/master/docs/Operators.md#batchnormalization.
 * @param node The ONNX node
 */
void OnnxParser::batchNormEquations( onnx::NodeProto& node, bool makeEquations )
{
    String outputNodeName = node.output()[0];
    String inputNodeName = node.input()[0];
    _shapeMap[outputNodeName] = _shapeMap[inputNodeName];

    if (!makeEquations)
    {
        return;
    }

    // Get inputs
    double epsilon = getFloatAttribute(node, "epsilon", 1e-05);
    Vector<double> scales         = getTensorFloatValues(*_constantMap[node.input()[1]]);
    Vector<double> biases         = getTensorFloatValues(*_constantMap[node.input()[2]]);
    Vector<double> inputMeans     = getTensorFloatValues(*_constantMap[node.input()[3]]);
    Vector<double> inputVariances = getTensorFloatValues(*_constantMap[node.input()[4]]);

    // Get variables
    Vector<Variable> inputVars = _varMap[inputNodeName];
    Vector<Variable> outputVars = makeNodeVariables(outputNodeName, false);
    assert (inputVars.size() == outputVars.size());

    for (uint i = 0; inputVars.size(); i++)
    {
        Equation e = Equation();
        e.addAddend(-1, outputVars[i]);
        e.addAddend(1 / sqrt(inputVariances[i] + epsilon) * scales[i], inputVars[i]);
        e.setScalar(inputMeans[i] / sqrt(inputVariances[i] + epsilon) * scales[i] - biases[i]);
        addEquation(e);
    }
}

/**
 * @brief Function to generate equations for a MaxPool node.
 * Implements https://github.com/onnx/onnx/blob/main/docs/Operators.md#MaxPool
 *
 * @param node ONNX node representing the MaxPool operation
 * @param makeEquations True if we need to create new variables and add new Relus
 */
void OnnxParser::maxPoolEquations( onnx::NodeProto& node, [[maybe_unused]] bool makeEquations )
{
    // See https://github.com/NeuralNetworkVerification/Marabou/blob/76b8eaf23518ca468c2cf05b742e3b4c858a64c3/maraboupy/MarabouNetworkONNX.py
    // for reference implementation
    unimplementedError(node);
}

/**
 * @brief Function to generate equations corresponding to
 * a convolution operation.
 * Implements https://github.com/onnx/onnx/blob/main/docs/Operators.md#Conv
 *
 * @param node ONNX node representing the operation
 * @param makeEquations True if we need to create new variables
 */
void OnnxParser::convEquations( onnx::NodeProto& node, [[maybe_unused]] bool makeEquations )
{
    // See https://github.com/NeuralNetworkVerification/Marabou/blob/76b8eaf23518ca468c2cf05b742e3b4c858a64c3/maraboupy/MarabouNetworkONNX.py
    // for reference implementation
    unimplementedError(node);
}

/**
 * @brief Function to generate equations corresponding to
 * a GeMM (General Matrix Multiplication) operation.
 * Implements https://github.com/onnx/onnx/blob/main/docs/Operators.md#Gemm
 *
 * @param node ONNX node representing the operation
 * @param makeEquations True if we need to create new variables
 */
void OnnxParser::gemmEquations( onnx::NodeProto& node, [[maybe_unused]] bool makeEquations )
{
    // See https://github.com/NeuralNetworkVerification/Marabou/blob/76b8eaf23518ca468c2cf05b742e3b4c858a64c3/maraboupy/MarabouNetworkONNX.py
    // for reference implementation
    unimplementedError(node);
}

/**
 * @brief Function to generate equations corresponding to pointwise Relu
 * Implements https://github.com/onnx/onnx/blob/main/docs/Operators.md#Relu
 *
 * @param node ONNX node representing the Relu operation
 * @param makeEquations True if we need to create new variables and add new Relus
 */
void OnnxParser::reluEquations( onnx::NodeProto& node, bool makeEquations )
{
    String outputNodeName = node.output()[0];
    String inputNodeName = node.input()[0];

    _shapeMap[outputNodeName] = _shapeMap[inputNodeName];
    if (!makeEquations)
    {
        return;
    }

    // Get variables
    Vector<Variable> inputVars  = _varMap[inputNodeName];
    Vector<Variable> outputVars = makeNodeVariables(outputNodeName, false);
    assert (inputVars.size() == outputVars.size());

    // Generate equations
    for (size_t i = 0; i < inputVars.size(); i++)
    {
        int inputVar = inputVars[i];
        int outputVar = outputVars[i];
        addRelu(inputVar, outputVar);
        setLowerBound(outputVar, 0.0f);
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
void OnnxParser::scaleAndAddEquations( onnx::NodeProto& node, bool makeEquations, double coefficient1, double coefficient2 )
{
    String outputName = node.output()[0];
    String input1Name = node.input()[0];
    String input2Name = node.input()[1];

    // Get the broadcasted shape
    TensorShape input1Shape = _shapeMap[input1Name];
    TensorShape input2Shape = _shapeMap[input2Name];

    TensorShape outputShape = getBroadcastShape(input1Shape, input2Shape);
    _shapeMap[outputName] = outputShape;

    if (!makeEquations)
    {
        return;
    }

    // Decide which inputs are variables and which are constants


    bool input1IsConstant = _constantMap.exists(input1Name);
    bool input2IsConstant = _constantMap.exists(input2Name);

    // If both inputs to add are constant, then the output is constant too
    // No new variables are needed, we could just store the output
    if (input1IsConstant && input2IsConstant)
    {
        String errorMessage = "Addition of constant tensors not yet supported for command-line Onnx files";
        throw MarabouError(MarabouError::ONNX_PARSE_ERROR, errorMessage.ascii());
    }

    // If both inputs are variables, then we need a new variable to represent
    // the sum of the two variables
    if (!input1IsConstant && !input2IsConstant)
    {
        Vector<Variable> outputVariables = makeNodeVariables(outputName, false);
        Vector<Variable> input1Variables = _varMap[input1Name];
        Vector<Variable> input2Variables = _varMap[input2Name];
        assert( input1Variables.size() == input2Variables.size() );
        assert( input2Variables.size() == outputVariables.size() );

        for (size_t i = 0; i < input1Variables.size(); i++)
        {
            Equation e = Equation();
            e.addAddend(coefficient1, input1Variables[i]);
            e.addAddend(coefficient2, input2Variables[i]);
            e.addAddend(-1, outputVariables[i]);
            e.setScalar(0.0);
            addEquation(e);
        }
        return;
    }

    // Otherwise, we are adding constants to variables.
    // We don't need new equations or new variables if the input variable is
    // the output of a linear equation. Instead, we can just edit the scalar
    // term of the existing linear equation. However, if the input variables
    // are not outputs of linear equations (input variables or outputs of
    // activation functions) then we will need new equations.
    Vector<double> inputConstants = getTensorFloatValues(*_constantMap[input1IsConstant ? input1Name : input2Name]);
    Vector<Variable> inputVariables = _varMap[input1IsConstant ? input2Name : input1Name];
    double constantCoefficient = input1IsConstant ? coefficient1 : coefficient2;
    double variableCoefficient = input1IsConstant ? coefficient2 : coefficient1;
    assert( inputConstants.size() == inputVariables.size() );

    // Adjust equations to incorporate the constant addition
    long unsigned int numberOfEquationsChanged = 0;
    for (size_t i = 0; i < inputVariables.size(); i++)
    {
        Variable inputVariable = inputVariables[i];
        double inputConstant = inputConstants[i];
        int equationIndex = findEquationWithOutputVariable(inputVariable);
        if (equationIndex != -1)
        {
            Equation equation = _equationList[equationIndex];
            double currentVariableCoefficient = equation.getCoefficient(inputVariable);
            equation.setCoefficient(inputVariable, variableCoefficient * currentVariableCoefficient);
            equation.setScalar(equation._scalar - constantCoefficient * inputConstant);
            _equationList[equationIndex] = equation;
            numberOfEquationsChanged += 1;
        }
    }

    if (numberOfEquationsChanged == inputVariables.size())
    {
        // If we changed one equation for every input variable, then
        // we don't need any new equations
        _varMap[outputName] = inputVariables;
    }
    else
    {
        // Otherwise, assert no equations were changed,
        // and we need to create new equations
        assert (numberOfEquationsChanged == 0);
        Vector<Variable> outputVariables = makeNodeVariables(outputName, false);
        for (size_t i = 0; i < outputVariables.size(); i++)
        {
            Equation e = Equation();
            e.addAddend(variableCoefficient, inputVariables[i]);
            e.addAddend(-1, outputVariables[i]);
            e.setScalar(-constantCoefficient*inputConstants[i]);
            addEquation(e);
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
void OnnxParser::matMulEquations(onnx::NodeProto &node, bool makeEquations)
{
    String nodeName = node.output()[0];

    // Get inputs and determine which inputs are constants and which are variables
    String input1Name = node.input()[0];
    String input2Name = node.input()[1];
    TensorShape input1Shape = _shapeMap[input1Name];
    TensorShape input2Shape = _shapeMap[input2Name];

    assert( input1Shape.last() == input2Shape.first() );
    assert( input1Shape.size() <= 2 );
    assert( input2Shape.size() <= 2 );

    // Calculate the output shape
    TensorShape outputShape;
    if (input1Shape.size() >= 1)
    {
        outputShape.append(input1Shape.first());
    }
    if (input2Shape.size() >= 1)
    {
        outputShape.append(input2Shape.last());
    }

    _shapeMap.insert(nodeName, outputShape);
    if (!makeEquations)
    {
        return;
    }

    // Assume that at exactly one input is a constant as
    // we cannot represent variable products with linear equations.
    bool input1IsConstant = _constantMap.exists(input1Name);
    bool input2IsConstant = _constantMap.exists(input2Name);

    //If both inputs are constant, than the output is constant as well, and we don't need new variables or equations
    if (input1IsConstant && input2IsConstant)
    {
        String errorMessage = "Matrix multiplication of constant tensors not yet implemented for command-line Onnx files";
        throw MarabouError(MarabouError::ONNX_PARSE_ERROR, errorMessage.ascii());
    }

    if (!input1IsConstant && !input2IsConstant)
    {
        String errorMessage = "Matrix multiplication of variable tensors is not supported";
        throw MarabouError(MarabouError::ONNX_PARSE_ERROR, errorMessage.ascii());
    }

    String constantName = input1IsConstant ? input1Name : input2Name;
    Vector<double> constants = getTensorFloatValues(*_constantMap[constantName]);

    String variableName = input1IsConstant ? input2Name : input1Name;
    Vector<Variable> variables = _varMap[variableName];

    // Create new variables
    Vector<Variable> outputVariables = makeNodeVariables(nodeName, false);

    // Pad the output if needed (matrix-matrix multiplication)
    if (outputShape.size() == 1 && input2Shape.size() > 1)
    {
        outputShape.insertHead(1);
    }

    int d1 = input1Shape.size() == 1 ? 1 : input1Shape.first();
    int d2 = input1Shape.last();
    int d3 = input2Shape.last();

    // Generate equations
    for (int i = 0; i < d1; i++)
    {
        // Differentiate between matrix-vector multiplication
        // and matrix-matrix multiplication
        if (input2Shape.size() == 2)
        {
            for (int j = 0; j < d3; j++)
            {
                Equation e;
                for (int k = 0; k < d2; k++)
                {
                    double constant;
                    Variable variable;
                    if (input1IsConstant)
                    {
                        constant = lookup2D(constants, i, k, d1, d2);
                        variable = lookup2D(variables, k, j, d2, d3);
                    }
                    else
                    {
                        constant = lookup2D(constants, k, j, d2, d3);
                        variable = lookup2D(variables, i, k, d1, d2);
                    }
                    e.addAddend(constant, variable);
                }

                // Put output variable as the last addend last
                Variable outputVariable = lookup2D(outputVariables, i , j , d1, d3);
                e.addAddend(-1, outputVariable);
                e.setScalar(0.0);
                addEquation(e);
            }
        }
        else
        {
            Equation e;
            for (int k = 0; k < d2; k++)
            {
                double constant;
                Variable variable;
                if (input1IsConstant)
                {
                    constant = lookup2D(constants, i, k, d1, d2);
                    variable = variables[k];
                }
                else
                {
                    constant = constants[k];
                    variable = lookup2D(variables, i, k, d1, d2);
                }
                e.addAddend(constant, variable);
            }

            // Put output variable as the last addend last
            Variable outputVariable = outputVariables[i];
            e.addAddend(-1, outputVariable);
            e.setScalar(0.0);
            addEquation(e);
        }
    }
}

/**
 * @brief Function to generate equations corresponding to a sigmoid node.
 * Implements https://github.com/onnx/onnx/blob/main/docs/Operators.md#Sigmoid
 *
 * @param node ONNX node representing the MatMul operation
 * @param makeEquations True if we need to create new variables and write Marabou equations
 */
void OnnxParser::sigmoidEquations(onnx::NodeProto &node, bool makeEquations)
{
    String outputNodeName = node.output()[0];
    String inputNodeName = node.input()[0];
    _shapeMap[outputNodeName] = _shapeMap[inputNodeName];

    if(!makeEquations)
    {
        return;
    }

    // Get variables
    Vector<Variable> inputVars  = _varMap[inputNodeName];
    Vector<Variable> outputVars = makeNodeVariables(outputNodeName, false);
    assert(inputVars.size() == outputVars.size());

    // Generate equations
    for (uint i = 0; i < inputVars.size(); i++)
    {
        addSigmoid(inputVars[i], outputVars[i]);
    }
    for (Variable v : outputVars)
    {
        setLowerBound(v, 0.0);
        setUpperBound(v, 1.0);
    }
}

Vector<double> OnnxParser::getTensorFloatValues(const onnx::TensorProto& tensor)
{
    checkTensorDataSourceIsInternal(tensor);
    checkTensorDataType(tensor, onnx::TensorProto_DataType_FLOAT);

    int size = tensorSize(_shapeMap[tensor.name()]);
    std::string raw_data = tensor.raw_data();
    Vector<double> result;
    if (raw_data.size() != 0)
    {
        checkEndianness();
        const char* bytes = raw_data.c_str();
        const float* floats = reinterpret_cast<const float*>(bytes);
        for (int i = 0; i < size; i++)
        {
            result.append(*(floats + i));
        }
    }
    else
    {
        for (int i = 0; i < size; i++)
        {
            result.append(tensor.float_data(i));
        }
    }
    return result;
}

Vector<int> OnnxParser::getTensorIntValues(const onnx::TensorProto& tensor)
{
    checkTensorDataSourceIsInternal(tensor);
    checkTensorDataType(tensor, onnx::TensorProto_DataType_INT64);

    int size = tensorSize(_shapeMap[tensor.name()]);
    std::string raw_data = tensor.raw_data();
    Vector<int> result;
    if (raw_data.size() != 0)
    {
        checkEndianness();
        const char* bytes = raw_data.c_str();
        const int* ints = reinterpret_cast<const int*>(bytes);
        for (int i = 0; i < size; i++)
        {
            result.append(*(ints + i));
        }
    }
    else
    {
        for (int i = 0; i < size; i++)
        {
            result.append(tensor.int64_data(i));
        }
    }
    return result;
}