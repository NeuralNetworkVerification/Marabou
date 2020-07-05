
from maraboupy.MarabouNetworkNNet import *
from maraboupy import Marabou

TOL = 1e-8

network_filename = "./resources/nnet/acasxu/ACASXU_experimental_v2a_1_9.nnet"

layer = 2


def test_evaluate_network():
    nnet_object = Marabou.read_nnet(filename=network_filename)

    N = 10
    for i in range(N):
        inputs = nnet_object.createRandomInputsForNetwork()

        output1 = nnet_object.evaluateNNet(inputs, last_layer=-1, normalize_inputs=False,
                                           normalize_outputs=False, activate_output_layer=False)

        output2 = nnet_object.evaluateNNet(inputs, normalize_inputs=False, normalize_outputs=False)

        assert (output1 == output2).all()

        # Compare evaluation with and without Marabou
        without_marabou_output = nnet_object.evaluate(np.array([inputs]),useMarabou=False).flatten()
        without_marabou_output_rounded = np.array([float(round(y,8)) for y in without_marabou_output])

        with_marabou_output = nnet_object.evaluate(np.array([inputs]),useMarabou=True).flatten()
        with_marabou_output_rounded = np.array([float(round(y,8)) for y in with_marabou_output])

        # Assert that all of the above agree, at least up to 10^-8
        assert (output1 == without_marabou_output).all()
        assert (without_marabou_output_rounded == with_marabou_output_rounded).all()

        # Adding input and output normalization

        output1 = nnet_object.evaluateNNet(inputs, last_layer=-1, normalize_inputs=True,
                                           normalize_outputs=True, activate_output_layer=False)

        output2 = nnet_object.evaluateNNet(inputs, normalize_inputs=True, normalize_outputs=True)

        assert (output1 == output2).all()


        # Compare direct evaluation to evaluating to a certain layer, and then from that layer onward

        layer_output = nnet_object.evaluateNNet(inputs, last_layer=layer, normalize_inputs=False,
                                                normalize_outputs=False, activate_output_layer=True)
        output1 = nnet_object.evaluateNNet(layer_output, first_layer=layer)

        output2 = nnet_object.evaluateNNet(inputs, normalize_inputs=False, normalize_outputs=False)


        assert (output1 == output2).all()


        # Same with input and output normalization

        layer_output = nnet_object.evaluateNNet(inputs, last_layer=layer, normalize_inputs=True,
                                                normalize_outputs=False, activate_output_layer=True)
        output1 = nnet_object.evaluateNNet(layer_output, first_layer=layer, normalize_outputs=True)

        output2 = nnet_object.evaluateNNet(inputs, normalize_inputs=True, normalize_outputs=True)

        assert (output1 == output2).all()


def test_write_read_evaluate(tmpdir):
    output_filename = tmpdir.mkdir("output_network").join("ACASXU_experimental_v2a_1_9_output.nnet").strpath

    nnet_object = Marabou.read_nnet(filename=network_filename)
    nnet_object.writeNNet(output_filename)

    nnet_object_a = Marabou.read_nnet(filename=output_filename)

    N = 10
    for i in range(N):
        inputs = nnet_object_a.createRandomInputsForNetwork()

        output1 = nnet_object.evaluateNNet(inputs, normalize_inputs=False, normalize_outputs=False)
        output2 = nnet_object_a.evaluateNNet(inputs, normalize_inputs=False, normalize_outputs=False)
        assert (output1 == output2).all()

        # Compare evaluation with and without Marabou
        without_marabou_output = nnet_object_a.evaluate(np.array([inputs]), useMarabou=False).flatten()
        with_marabou_output = nnet_object_a.evaluate(np.array([inputs]), useMarabou=True).flatten()

        # Assert that all of the above agree up to TOL
        assert (output2 == without_marabou_output).all()
        assert (abs(without_marabou_output - with_marabou_output) < TOL).all()

        # Adding input and output normalization

        output1 = nnet_object.evaluateNNet(inputs, normalize_inputs=True, normalize_outputs=True)
        output2 = nnet_object_a.evaluateNNet(inputs, normalize_inputs=True, normalize_outputs=True)
        assert (output1 == output2).all()



def test_normalize_read_flag(tmpdir):
    '''
    Similar tests to those in test_write_read_evaluate(), but turns normalize flag on when creating the
    MarabouNetworkNNet objects.
    Note that in this case inputs and outputs need to be normalized.
    '''
    output_filename = tmpdir.mkdir("output_network").join("ACASXU_experimental_v2a_1_9_output.nnet").strpath

    nnet_object = Marabou.read_nnet(filename=network_filename, normalize=True)
    nnet_object.writeNNet(output_filename)

    nnet_object_a = Marabou.read_nnet(filename=output_filename, normalize=True)

    N = 10
    for i in range(N):
        inputs = nnet_object_a.createRandomInputsForNetwork()

        output1 = nnet_object.evaluateNNet(inputs, normalize_inputs=True, normalize_outputs=True)
        output2 = nnet_object_a.evaluateNNet(inputs, normalize_inputs=True, normalize_outputs=True)
        assert (output1 == output2).all()

        without_marabou_output = nnet_object_a.evaluate(np.array([inputs]), useMarabou=False).flatten()
        assert (output2 == without_marabou_output).all()

        with_marabou_output = nnet_object_a.evaluate(np.array([inputs]), useMarabou=True).flatten()
        assert (abs(without_marabou_output - with_marabou_output) < TOL).all()


def test_reset_network():
    '''
    Tests resetNetworkFromParameters()
    :return:
    '''
    nnet_object1 = Marabou.read_nnet(filename=network_filename)
    nnet_object2 = MarabouNetworkNNet()

    # Using default parameters
    nnet_object2.resetNetworkFromParameters(weights=nnet_object1.weights, biases=nnet_object1.biases)

    N = 10
    for i in range(N):
        inputs = nnet_object1.createRandomInputsForNetwork()

        output1 = nnet_object1.evaluateNNet(inputs, normalize_inputs=False, normalize_outputs=False)
        output2 = nnet_object2.evaluateNNet(inputs, normalize_inputs=False, normalize_outputs=False)

        # Normalization should not matter with default parameters
        output2_norm = nnet_object2.evaluateNNet(inputs, normalize_inputs=True, normalize_outputs=True)
        assert (abs(output1 - output2) < TOL).all()
        assert (abs(output1 - output2_norm) < TOL).all()

    nnet_object1 = Marabou.read_nnet(filename=network_filename, normalize=True)

    # Using the exact same parameters as in the original network
    nnet_object2.resetNetworkFromParameters(weights=nnet_object1.weights, biases=nnet_object1.biases,
                                            normalize=nnet_object1.normalize,
                                            inputMinimums=nnet_object1.inputMinimums,
                                            inputMaximums=nnet_object1.inputMaximums,
                                            inputMeans=nnet_object1.inputMeans,
                                            inputRanges=nnet_object1.inputRanges,
                                            outputMean=nnet_object1.outputMean,
                                            outputRange=nnet_object1.outputRange)

    # All evaluations with normalization should now give the same results.
    for i in range(N):
        inputs = nnet_object2.createRandomInputsForNetwork()

        output1 = nnet_object1.evaluateNNet(inputs, normalize_inputs=False, normalize_outputs=False)
        output2 = nnet_object2.evaluateNNet(inputs, normalize_inputs=False, normalize_outputs=False)
        assert (abs(output1 - output2) < TOL).all()

        output1_norm = nnet_object1.evaluateNNet(inputs, normalize_inputs=True, normalize_outputs=True)
        output2_norm = nnet_object2.evaluateNNet(inputs, normalize_inputs=True, normalize_outputs=True)
        assert (abs(output1_norm - output2_norm) < TOL).all()


        without_marabou_output1 = nnet_object1.evaluate(np.array([inputs]), useMarabou=False).flatten()
        with_marabou_output1 = nnet_object1.evaluate(np.array([inputs]), useMarabou=True).flatten()
        without_marabou_output2 = nnet_object2.evaluate(np.array([inputs]), useMarabou=False).flatten()
        with_marabou_output2 = nnet_object2.evaluate(np.array([inputs]), useMarabou=True).flatten()

        assert (abs(with_marabou_output1 - with_marabou_output2) < TOL).all()
        assert (abs(without_marabou_output1 - without_marabou_output2) < TOL).all()
        assert (abs(with_marabou_output2 - without_marabou_output2) < TOL).all()
        assert (abs(output1_norm - with_marabou_output2) < TOL).all()
