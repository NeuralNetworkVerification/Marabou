
import numpy as np
import onnx

##############
## Settings ##
##############

producer_name = "onnx-layer-zoo"
input_name = "x"
output_name = "y"

####################
## Helper methods ##
####################

def make_network(name, node, input_shape, output_shape, aux_nodes):
    input = [onnx.helper.make_tensor_value_info(input_name, onnx.TensorProto.FLOAT, input_shape)]
    output = [onnx.helper.make_tensor_value_info(output_name, onnx.TensorProto.FLOAT, output_shape)]
    graph = onnx.helper.make_graph([node] + aux_nodes, name, input, output)
    model = onnx.helper.make_model(graph, producer_name=producer_name)

    onnx.save(model, f"{name}.onnx")

def make_constant_float_node(name, values):
    value_array = np.array(values).astype(float)
    return onnx.helper.make_node(
        "Constant",
        inputs=[],
        outputs=[name],
        value=onnx.helper.make_tensor(
            name=name,
            data_type=onnx.TensorProto.FLOAT,
            dims=value_array.shape,
            vals=value_array.flatten(),
        ),
    )

def make_constant_int_node(name, values):
    value_array = np.array(values).astype(int)
    return onnx.helper.make_node(
        "Constant",
        inputs=[],
        outputs=[name],
        value=onnx.helper.make_tensor(
            name="const_tensor",
            data_type=onnx.TensorProto.INT64,
            dims=value_array.shape,
            vals=value_array.flatten(),
        ),
    )

############
## Layers ##
############

def constant_node():
    values = np.array([[0, 0.5],[1, 1.5]], dtype=np.float32)
    node = onnx.helper.make_node(
        "Constant",
        inputs=[],
        outputs=[output_name],
        value=onnx.helper.make_tensor(
            name="const_tensor",
            data_type=onnx.TensorProto.FLOAT,
            dims=values.shape,
            vals=values.flatten().astype(float),
        ),
    )
    return ("constant", node, [1], [2, 2], [])

def identity_node():
    node = onnx.helper.make_node(
        "Identity",
        inputs=[input_name],
        outputs=[output_name],
    )
    return ("identity", node, [2,2], [2, 2], [])

def reshape_node():
    shape_node = make_constant_int_node("shape", [1,4])
    node = onnx.helper.make_node(
        "Reshape",
        inputs=[input_name, "shape"],
        outputs=[output_name],
    )
    return ("reshape", node, [2,2], [1,4], [shape_node])

def reshape_node_with_dimension_inference():
    shape_node = make_constant_int_node("shape", [-1,4])
    node = onnx.helper.make_node(
        "Reshape",
        inputs=[input_name, "shape"],
        outputs=[output_name],
    )
    return ("reshape_with_dimension_inference", node, [2,2], [1,4], [shape_node])

def flatten_node():
    node = onnx.helper.make_node(
        "Flatten",
        inputs=[input_name],
        outputs=[output_name],
        axis=2
    )
    return ("flatten", node, [2,2,2,1], [4, 2], [])

def transpose_node():
    node = onnx.helper.make_node(
        "Transpose",
        inputs=[input_name],
        outputs=[output_name],
        perm=[1,0]
    )
    return ("transpose", node, [2,3], [3,2], [])

def batch_normalization_node():
    scale = make_constant_float_node("scale", [0.5, 1, 2])
    bias = make_constant_float_node("bias", [0, 1, 0])
    mean = make_constant_float_node("mean", [5, 6, 7])
    var = make_constant_float_node("var", [0.5, 0.5, 0.5])

    node = onnx.helper.make_node(
        "BatchNormalization",
        inputs=[input_name, "scale", "bias", "mean", "var"],
        outputs=[output_name],
    )

    return ("batchnorm", node, [1, 3, 2, 1], [1, 3, 2, 1], [scale, bias, mean, var])

def max_pool_node():
    node = onnx.helper.make_node(
        "MaxPool",
        inputs=[input_name],
        outputs=[output_name],
        kernel_shape=[3, 3],
        strides=[2, 2],
        ceil_mode=True,
    )
    input_shape = [1, 1, 4, 4]
    output_shape = [1, 1, 2, 2]
    return ("maxpool", node, input_shape, output_shape, [])

def conv_node():
    weights = make_constant_float_node("weights_const", [
        [
            [
                [1.0, 1.0, 1.0],  # (1, 1, 3, 3) tensor for convolution weights
                [1.0, 1.0, 1.0],
                [1.0, 1.0, 1.0],
            ]
        ]
    ])
    node = onnx.helper.make_node(
        "Conv",
        inputs=[input_name, "weights_const"],
        outputs=[output_name],
        kernel_shape=[3, 3],
        # Default values for other attributes: strides=[1, 1], dilations=[1, 1], groups=1
        pads=[1, 1, 1, 1],
    )
    return ("conv", node, [1, 1, 5, 5], [1, 1, 5, 5], [weights])

def gemm_node():
    mul_const_node = make_constant_float_node("mul_const", [[0.5, 1.0], [1.5, 2.0]])
    add_const_node = make_constant_float_node("add_const", [[3, 4.5]])
    node = onnx.helper.make_node(
        "Gemm",
        inputs=[input_name, "mul_const", "add_const"],
        outputs=[output_name],
        alpha=0.25,
        beta=0.5,
        transA=0,
        transB=1,
    )
    return ("gemm", node, [2,2], [2,2], [mul_const_node, add_const_node])

def relu_node():
    node = onnx.helper.make_node(
        "Relu",
        inputs=[input_name],
        outputs=[output_name],
    )

    return ("relu", node, [2,2], [2,2], [])

def add_node():
    const_node = make_constant_float_node("const", [[0.5, 1.0], [1.5, 2.0]])

    node = onnx.helper.make_node(
        "Add",
        inputs=[input_name, "const"],
        outputs=[output_name],
    )

    return ("add", node, [2,2], [2,2], [const_node])

def sub_node():
    const_node = make_constant_float_node("const", np.array([[0.5, 1.0], [1.5, 2.0]]))

    node = onnx.helper.make_node(
        "Sub",
        inputs=[input_name, "const"],
        outputs=[output_name],
    )

    return ("sub", node, [2,2], [2,2], [const_node])

def matmul_node():
    const_node = make_constant_float_node("const", np.array([[0.0, 0.5], [1.5, 2.0], [-1, -2]]))

    node = onnx.helper.make_node(
        "MatMul",
        inputs=[input_name, "const"],
        outputs=[output_name],
    )
    return ("matmul", node, [2,3], [2,2], [const_node])


def sigmoid_node():
    node = onnx.helper.make_node(
        "Sigmoid",
        inputs=[input_name],
        outputs=[output_name],
    )

    return ("sigmoid", node, [2,2], [2,2], [])

##########
## Main ##
##########

if __name__ == "__main__":
    make_network(*constant_node())
    make_network(*identity_node())
    make_network(*reshape_node())
    make_network(*reshape_node_with_dimension_inference())
    make_network(*flatten_node())
    make_network(*transpose_node())
    make_network(*batch_normalization_node())
    make_network(*max_pool_node())
    make_network(*conv_node())
    make_network(*gemm_node())
    make_network(*relu_node())
    make_network(*add_node())
    make_network(*sub_node())
    make_network(*matmul_node())
    make_network(*sigmoid_node())