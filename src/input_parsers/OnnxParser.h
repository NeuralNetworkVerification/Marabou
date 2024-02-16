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

#include "InputQuery.h"
#include "List.h"
#include "MString.h"
#include "Map.h"
#include "NetworkParser.h"
#include "Set.h"
#include "TensorUtils.h"
#include "Vector.h"
#include "onnx.proto3.pb.h"

#define ONNX_LOG( x, ... ) LOG( GlobalConfiguration::ONNX_PARSER_LOGGING, "OnnxParser: %s\n", x )


class OnnxParser : public NetworkParser
{
public:
    OnnxParser( const String &path );

    void generateQuery( InputQuery &inputQuery );
    void generatePartialQuery( InputQuery &inputQuery,
                               Set<String> &inputNames,
                               Set<String> &terminalNames );


private:
    // Settings //

    onnx::GraphProto _network;
    Set<String> _inputNames;

    /*
      The set of terminal nodes for the query. Note that these doesn't have to be outputs of
      the network, they can be intermediate nodes.
    */
    Set<String> _terminalNames;

    // State //

    Map<String, TensorShape> _shapeMap;
    Map<String, Vector<Variable>> _varMap;
    Map<String, const Vector<int>> _constantIntTensors;
    Map<String, const Vector<double>> _constantFloatTensors;
    Set<String> _processedNodes;
    unsigned _numberOfFoundInputs;

    // Methods //

    void readNetwork( const String &path );
    Set<String> readInputNames();
    Set<String> readOutputNames();
    void initializeShapeAndConstantMaps();
    void validateUserInputNames( Set<String> &inputNames );
    void validateUserTerminalNames( Set<String> &terminalNames );
    void validateAllInputsAndOutputsFound();

    void
    processGraph( Set<String> &inputNames, Set<String> &terminalNames, InputQuery &inputQuery );
    void processNode( String &nodeName, bool makeEquations );
    void makeMarabouEquations( onnx::NodeProto &node, bool makeEquations );
    Set<String> getInputsToNode( onnx::NodeProto &node );
    List<onnx::NodeProto> getNodesWithOutput( String &nodeName );
    Vector<Variable> makeNodeVariables( String &nodeName, bool isInput );

    bool isConstantNode( String name );
    void transferValues( String oldName, String newName );
    void insertConstant( String name, const onnx::TensorProto &tensor, TensorShape shape );

    void constant( onnx::NodeProto &node );
    void identity( onnx::NodeProto &node );
    void cast( onnx::NodeProto &node );
    void reshape( onnx::NodeProto &node );
    void squeeze( onnx::NodeProto &node );
    void flatten( onnx::NodeProto &node );
    void transpose( onnx::NodeProto &node );
    void batchNormEquations( onnx::NodeProto &node, bool makeEquations );
    void maxPoolEquations( onnx::NodeProto &node, bool makeEquations );
    void convEquations( onnx::NodeProto &node, bool makeEquations );
    void gemmEquations( onnx::NodeProto &node, bool makeEquations );
    void scaleAndAddEquations( onnx::NodeProto &node,
                               bool makeEquations,
                               double coefficient1,
                               double coefficient2 );
    void matMulEquations( onnx::NodeProto &node, bool makeEquations );
    void reluEquations( onnx::NodeProto &node, bool makeEquations );
    void leakyReluEquations( onnx::NodeProto &node, bool makeEquations );
    void sigmoidEquations( onnx::NodeProto &node, bool makeEquations );
    void tanhEquations( onnx::NodeProto &node, bool makeEquations );
};

#endif // __OnnxParser_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
