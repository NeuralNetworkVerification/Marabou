/*********************                                                        */
/*! \file OnnxParser.h
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
 **/

#ifndef __OnnxParser_h__
#define __OnnxParser_h__

#include "InputQueryBuilder.h"
#include "List.h"
#include "MString.h"
#include "Map.h"
#include "Query.h"
#include "Set.h"
#include "TensorUtils.h"
#include "Vector.h"
#include "onnx.proto3.pb.h"

#define ONNX_LOG( x, ... ) LOG( GlobalConfiguration::ONNX_PARSER_LOGGING, "OnnxParser: %s\n", x )


class OnnxParser
{
public:
    static void parse( InputQueryBuilder &query,
                       const String &path,
                       const Set<String> inputNames,
                       const Set<String> outputNames );


private:
    // Settings //
    OnnxParser( InputQueryBuilder &query,
                const String &path,
                const Set<String> inputNames,
                const Set<String> terminalNames );

    InputQueryBuilder &_query;
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
    Map<String, const Vector<int64_t>> _constantIntTensors;
    Map<String, const Vector<double>> _constantFloatTensors;
    Map<String, const Vector<int32_t>> _constantInt32Tensors;
    Set<String> _processedNodes;
    unsigned _numberOfFoundInputs;

    // Methods //

    const Set<String> readInputNames();
    const Set<String> readOutputNames();
    void validateUserInputNames( const Set<String> &inputNames );
    void validateUserTerminalNames( const Set<String> &terminalNames );

    void readNetwork( const String &path );
    void initializeShapeAndConstantMaps();
    void validateAllInputsAndOutputsFound();

    void processGraph();
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
    void dropout( onnx::NodeProto &node );
    void cast( onnx::NodeProto &node );
    void reshape( onnx::NodeProto &node );
    void squeeze( onnx::NodeProto &node );
    void unsqueeze( onnx::NodeProto &node );
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
