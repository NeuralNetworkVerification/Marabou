from maraboupy import Marabou
from maraboupy import equivalence
from maraboupy import create_parsed_query
from maraboupy import create_edited_query
import numpy as np
import tensorflow as tf
import os
import random
import sys



def build_adversary_dict(net, number_of_examples, x_test, y_test):

    dict_of_examples = {}
    examples_found = 0

    current_random_index = random.randint(0, 9000) #* 1000
    # current_random_index = 9 # todo delete

    while examples_found < number_of_examples:

        output_vec = net.evaluateWithMarabou(inputValues=x_test[current_random_index])[0]
        classification = np.argmax(output_vec)
        # highest_output_val = output_vec[classification] # todo added
        true_label = y_test[current_random_index]

        # if found an input that the nn classifies correctly
        if classification == true_label:

            output_vec[classification] = - np.inf
            runner_up = np.argmax(output_vec)
            # runner_up_val = output_vec[runner_up] # todo added
            # if abs(highest_output_val-runner_up_val) <= 4: # thershold
            dict_of_examples[current_random_index] = [true_label, runner_up]
            examples_found += 1

        current_random_index += 1

    return dict_of_examples



def main_runner(name, nn_directory_path, nn_output, number_of_examples, delta):
    # load all the data and change to 784-dim vectors
    (x_train, y_train), (x_test, y_test) = tf.keras.datasets.fashion_mnist.load_data()
    x_test=x_test / 255.0
    x_test = equivalence.change_input_dimension(x_test)

    # create MarabouNetworkTF object
    net = Marabou.read_tf(filename=nn_directory_path, modelType="savedModel_v2", outputName=nn_output)

    # create dictionary of adversaries
    dict_of_adversaries = build_adversary_dict(net=net, number_of_examples=number_of_examples,
                                               x_test=x_test, y_test=y_test)
    samples_to_check = list(dict_of_adversaries.keys())
    assert (len(samples_to_check) == number_of_examples)

    inputVars = list(range(784))

    for inputVar in inputVars:
        fixed_lower_bound = 0
        fixed_upper_bound = 1

        net.setLowerBound(inputVar, fixed_lower_bound)
        net.setUpperBound(inputVar, fixed_upper_bound)

    # save original ipq
    path_to_original_ipq = "../../cluster/" + name + "_ipq_original"
    net.saveQuery(filename=path_to_original_ipq)

    # skipped parsed ipq

    dir_of_tests_for_delta = "../../cluster/300_ipqs_for_andew_small_delta_xnor/delta_" + str (delta) # todo update
    os.mkdir(dir_of_tests_for_delta)
    # create final edited query, for each sample chosen
    for i in range(number_of_examples):

        sample_index = samples_to_check[i]
        sample = x_test[sample_index]

        path_to_edited_ipq = dir_of_tests_for_delta + "/" + name + "_ipq_parsed_" + "delta_" \
                             + str(delta) + "_sample_index_" + str(sample_index)

        true_label, runner_up = dict_of_adversaries[sample_index]

        create_edited_query.create_edited_query(input_path_for_parsed_ipq=path_to_original_ipq, delta=delta/256.0,
                                                input_test_sample=sample, true_label=true_label,
                                                runner_up_label=runner_up, output_path=path_to_edited_ipq)



if __name__ == '__main__':
    # example input:
    # smallBNN
    # /cs/usr/guyam/Desktop/shortcut_guyam/three_final_models/smallBNN
    # StatefulPartitionedCall/StatefulPartitionedCall/sequential/FC_out_10/MatMul
    # delta - 1
    # examples - 7

    #fashion_xnor /cs/usr/guyam/Desktop/shortcut_guyam/xnor/fashion_xnor StatefulPartitionedCall/StatefulPartitionedCall/sequential_1/quant_dense_1/MatMul 4 5

    #fashion_xnor
    #fashion_xnor_input = "/cs/usr/guyam/Desktop/shortcut_guyam/xnor/fashion_xnor"
    #fashion_xnor_output  = "StatefulPartitionedCall/StatefulPartitionedCall/sequential_1/quant_dense_1/MatMul"
    #delta - 4
    #examples - 5



    # input parameters
    #print(sys.argv)
    assert(len(sys.argv) == 6)
    NAME, DIRECTORY_OF_NN, NN_OUTPUT = sys.argv[1:4]
    DELTA = float(sys.argv[4])
    NUMBER_OF_EXAMPLES = int(sys.argv[5])

    # sanity check - nn uploaded ok
    equivalence.test_nn_equivalence(filename=DIRECTORY_OF_NN, modelType="savedModel_v2", outputName=NN_OUTPUT,
                                    num_of_samples_to_test=10)

    # create <number_of_example> edited ipq files, for given nn and delta
    main_runner(name = NAME, nn_directory_path=DIRECTORY_OF_NN, nn_output=NN_OUTPUT,
                number_of_examples=NUMBER_OF_EXAMPLES, delta=DELTA)
