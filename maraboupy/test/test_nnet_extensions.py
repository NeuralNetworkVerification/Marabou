import sys
sys.path.append('./maraboupy/')

try:
    from MarabouNetworkNNetExtensions import *
except ImportError:
    from maraboupy.MarabouNetworkNNetExtensions import *
    
from subprocess import call

property_filename = "./resources/properties/acas_property_4.txt"
network_filename = "./resources/nnet/acasxu/ACASXU_experimental_v2a_1_9.nnet"

layer = 2


def test_split_nnet():

    nnet_object = MarabouNetworkNNet(filename=network_filename)

    nnet_object1, nnet_object2 = splitNNet(marabou_nnet=nnet_object,layer=layer)


    #TESTING THE SPLIT FUNCTIONALITY AND DIFFERENT METHODS OF EVALUATION.

    N = 10

    for i in range(N):
            inputs = createRandomInputsForNetwork(nnet_object)

            layer_output = nnet_object.evaluateNetworkToLayer(inputs, last_layer=layer, normalize_inputs=False,
                                                              normalize_outputs=False, activate_output_layer=True)
            output1 = nnet_object1.evaluateNetworkToLayer(inputs,last_layer=-1, normalize_inputs=False,
                                                          normalize_outputs=False, activate_output_layer=True
                                                        )
            assert (layer_output == output1).all()

            true_output = nnet_object.evaluateNetworkToLayer(inputs, last_layer=-1, normalize_inputs=False,
                                                             normalize_outputs=False)
            output2 = nnet_object2.evaluateNetworkToLayer(layer_output,last_layer=-1, normalize_inputs=False,
                                                          normalize_outputs=False)
            output2b = nnet_object.evaluateNetworkFromLayer(layer_output,first_layer=layer)
            true_outputb = nnet_object.evaluateNetworkFromLayer(inputs)
            true_outputc = nnet_object.evaluateNetwork(inputs,normalize_inputs=False,normalize_outputs=False)

            assert (true_output == output2).all()
            assert (true_output == output2b).all()
            assert (true_outputb == output2b).all()
            assert (true_outputc == output2b).all()

def test_write_to_file():
    output_filename = "./maraboupy/test/ACASXU_experimental_v2a_1_9_output.nnet"

    nnet_object = MarabouNetworkNNet(filename=network_filename)
    nnet_object.writeNNet(output_filename)

    call(['diff',output_filename,network_filename])

def test_split_and_write():
    output_filename = "./maraboupy/test/ACASXU_experimental_v2a_1_9_output.nnet"
    output_filename1 = "./maraboupy/test/ACASXU_experimental_v2a_1_9_output1.nnet"
    output_filename2 = "./maraboupy/test/ACASXU_experimental_v2a_1_9_output2.nnet"

    try:
        nnet_object = MarabouNetworkNNetQuery(filename=network_filename, property_filename=property_filename)
    except NameError:
        warnings.warn('MarabouNetworkNNetQuery not installed, can not test split and and write.')
        return
    nnet_object.tightenBounds()
    nnet_object.writeNNet(output_filename)

    nnet_object1, nnet_object2 = splitNNet(marabou_nnet=nnet_object, layer=layer)

    nnet_object1.writeNNet(output_filename1)
    nnet_object2.writeNNet(output_filename2)

    nnet_object_a = MarabouNetworkNNet(filename=output_filename)
    nnet_object1_a = MarabouNetworkNNet(filename=output_filename1)
    nnet_object2_a = MarabouNetworkNNet(filename=output_filename2)

    # COMPARING RESULTS OF THE NETWORKS CREATED FROM NEW FILES TO THE ORIGINALS ONES.

    N = 10
    for i in range(N):
            inputs = createRandomInputsForNetwork(nnet_object_a)

            layer_output = nnet_object_a.evaluateNetworkToLayer(inputs, last_layer=layer, normalize_inputs=False,
                                                                normalize_outputs=False, activate_output_layer=True)
            output1 = nnet_object1_a.evaluateNetworkToLayer(inputs,last_layer=-1, normalize_inputs=False,
                                                            normalize_outputs=False, activate_output_layer=True)

            assert (layer_output == output1).all()

            true_output = nnet_object_a.evaluateNetworkToLayer(inputs, last_layer=-1, normalize_inputs=False,
                                                               normalize_outputs=False)
            output2 = nnet_object2_a.evaluateNetworkToLayer(layer_output,last_layer=-1, normalize_inputs=False,
                                                            normalize_outputs=False)
            output2b = nnet_object_a.evaluateNetworkFromLayer(layer_output,first_layer=layer)
            true_outputb = nnet_object_a.evaluateNetworkFromLayer(inputs)
            true_outputc = nnet_object_a.evaluateNetwork(inputs,normalize_inputs=False,normalize_outputs=False)
            true_outputd = nnet_object.evaluateNetwork(inputs,normalize_inputs=False,normalize_outputs=False)

            assert (true_output == output2).all()
            assert (output2b == true_outputb).all()
            assert (true_outputc == true_outputd).all()
            assert (true_outputc == output2b).all()
            assert (true_outputc == true_output).all()

            #Test evaluateWithoutMarabou from MarabouNetwork.py
            true_outputf = nnet_object.evaluate(np.array([inputs]),useMarabou=False).flatten().tolist()

            #Test evaluateWithMarabou from MarabouNetwork.py
            true_outpute = nnet_object.evaluate(np.array([inputs])).flatten()
            true_outpute_rounded = np.array([float(round(y,8)) for y in true_outpute])

            # Some inputs lead to different outputs (even though they look the same), when evaluating
            # with and without Marabou

            if not (true_outpute == true_outputc).all():
                   print(true_outpute == true_outputc)
                   print("i=", i, "   input: ", inputs, "   ", "\n", "WithoutMarabou output: ", true_outputc, "\n",
                         "WithMarabou output: ", true_outpute, "\n", "direct output: ", true_outputb, "\n")

            # However, if both are rounded to the same number of digits (say, 8), the results agree:

            true_outputc_rounded = np.array([float(round(y, 8)) for y in true_outputc])

            assert (true_outputc_rounded == true_outpute_rounded).all()

if __name__ == "__main__":
    test_split_nnet()
    test_write_to_file()
    test_split_and_write()
