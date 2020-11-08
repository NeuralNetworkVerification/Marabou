from maraboupy import Marabou
from maraboupy import equivalence
from maraboupy import create_parsed_query
from maraboupy import create_edited_query
import numpy as np
import os
import random
import sys



def build_adversary_dict(net, number_of_examples):
    x_train, y_train, x_test, y_test = equivalence.load_data()
    x_test = equivalence.change_input_dimension(x_test)

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
    x_train, y_train, x_test, y_test = equivalence.load_data()
    x_test = equivalence.change_input_dimension(x_test)

    # create MarabouNetworkTF object
    net = Marabou.read_tf(filename=nn_directory_path, modelType="savedModel_v2", outputName=nn_output)

    # create dictionary of adversaries
    dict_of_adversaries = build_adversary_dict(net=net, number_of_examples=number_of_examples)
    samples_to_check = list(dict_of_adversaries.keys())
    assert (len(samples_to_check) == number_of_examples)

    # save original ipq
    path_to_original_ipq = "../../cluster/" + name + "_ipq_original"
    net.saveQuery(filename=path_to_original_ipq)

    # create an equivalent parsed ipq (without excessive signs, but still withought tight bounds and inequality constraint)
    path_to_parsed_ipq = "../../cluster/" + name + "_ipq_parsed"
    create_parsed_query.create_new_parsed_ipq(input_path=path_to_original_ipq, output_path=path_to_parsed_ipq)

    dir_of_tests_for_delta = "../../cluster/200_ipqs_for_andrew_small_delta_mlp/delta_" + str (delta) # todo update
    os.mkdir(dir_of_tests_for_delta)
    # create final edited query, for each sample chosen
    for i in range(number_of_examples):

        sample_index = samples_to_check[i]
        sample = x_test[sample_index]

        path_to_edited_ipq = dir_of_tests_for_delta + "/" + name + "_ipq_parsed_" + "delta_" \
                             + str(delta) + "_sample_index_" + str(sample_index)

        true_label, runner_up = dict_of_adversaries[sample_index]

        create_edited_query.create_edited_query(input_path_for_parsed_ipq=path_to_parsed_ipq, delta=delta/256.0,
                                                input_test_sample=sample, true_label=true_label,
                                                runner_up_label=runner_up, output_path=path_to_edited_ipq)



if __name__ == '__main__':
    # example input:
        # smallBNN
        # /cs/usr/guyam/Desktop/shortcut_guyam/three_final_models/smallBNN
        # StatefulPartitionedCall/StatefulPartitionedCall/sequential/FC_out_10/MatMul
        # delta - 1
        # examples - 7

    # deep_6 /cs/usr/guyam/Desktop/shortcut_guyam/deep_nn/deep_6_layers_50_10_BNN StatefulPartitionedCall/StatefulPartitionedCall/sequential/FC_out_7/MatMul 0.1 50

    # input parameters
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
