/*********************                                                        */
/*! \file OnnxParser.h
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
 **/

#ifndef __OnnxParser_h__
#define __OnnxParser_h__

#include "onnx.proto3.pb.h"
#include "Map.h"
#include "List.h"
#include "Vector.h"
#include "Set.h"
#include "MString.h"
#include "InputQuery.h"
#include "NetworkParser.h"
#include "TensorUtils.h"

#define ONNX_LOG(x, ...) LOG(GlobalConfiguration::ONNX_PARSER_LOGGING, "OnnxParser: %s\n", x)


class OnnxParser : public NetworkParser
{
public:
    OnnxParser( const String& path );
    void generateQuery( InputQuery &inputQuery );
    void generatePartialQuery( InputQuery &inputQuery , Set<String> &inputNames, String &outputName );

private:

    onnx::GraphProto _network;
    Set<String> _inputNames;
    String _outputName;
    Set<String> _processedNodes;
    unsigned _numberOfFoundInputs;

    Map<String, const onnx::NodeProto*> _nodeMap;
    Map<String, const onnx::TensorProto*> _constantMap;

    Map<String, TensorShape> _shapeMap;
    Map<String, Vector<Variable>> _varMap;

    void readNetwork( const String& path );
    Set<String> readInputNames();
    String readOutputName();
    void initializeMaps();
    void initializeShapeMap();
    void validateUserInputNames( Set<String>& inputNames) ;
    void validateUserOutputNames( String& outputName );
    void validateAllInputsAndOutputsFound();

    void processGraph( Set<String>& inputNames, String& outputName , InputQuery &inputQuery );
    void processNode( String& nodeName, bool makeEquations );
    void makeMarabouEquations ( onnx::NodeProto& node, bool makeEquations );
    Set<String> getInputsToNode( onnx::NodeProto& node );
    List<onnx::NodeProto> getNodesWithOutput( String& nodeName );
    Vector<Variable> makeNodeVariables ( String& nodeName, bool isInput );

    Vector<double> getTensorFloatValues( const onnx::TensorProto& tensor );
    Vector<int> getTensorIntValues( const onnx::TensorProto& tensor );

    void constant( onnx::NodeProto& node );
    void identity( onnx::NodeProto& node );
    void cast( onnx::NodeProto& node );
    void reshape( onnx::NodeProto& node );
    void flatten( onnx::NodeProto& node );
    void transpose( onnx::NodeProto& node );
    void batchNormEquations( onnx::NodeProto& node, bool makeEquations );
    void maxPoolEquations( onnx::NodeProto& node, bool makeEquations );
    void convEquations( onnx::NodeProto& node, bool makeEquations );
    void gemmEquations( onnx::NodeProto& node, bool makeEquations );
    void scaleAndAddEquations( onnx::NodeProto& node, bool makeEquations, double coefficient1, double coefficient2 );
    void matMulEquations( onnx::NodeProto& node, bool makeEquations );
    void reluEquations( onnx::NodeProto& node, bool makeEquations );
    void sigmoidEquations( onnx::NodeProto& node, bool makeEquations );

    void missingNodeError( String& missingNodeName );
};

#endif // __OnnxParser_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
