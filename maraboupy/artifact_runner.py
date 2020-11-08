from maraboupy.MarabouCore import *
import tensorflow as tf
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from maraboupy import MarabouUtils
from maraboupy import Marabou
import sys
#from tensorflow.examples.tutorials.mnist import input_data


# todo fill this
# constants for the Marabou framework run in the "split and conquer" mode

# number of CPUs available on the machine. It is highly advised not to choose the maximal number
NUMBER_OF_CPU = 4

# number of initial splits. A recommended number is log2 (NUMBER_OF_CPU) divides.
INITIAL_DIVIDES = 2

# todo - document and change to "PUT_PATH_HERE"
BNN_ARTIFACT_PARENT_DIRECTORY = "/cs/labs/guykatz/guyam"

# todo - document and change to "PUT_PATH_HERE"
PATH_FOR_INPUT_QUERY= "/cs/labs/guykatz/guyam/bnn_artifact/mlp_experiment/300_ipqs/delta_10.0/deep_6_ipq_parsed_delta_10.0_sample_index_1036"
#PATH_FOR_INPUT_QUERY= "/cs/labs/guykatz/guyam/bnn_artifact/xnor_experiment/300_ipqs/delta_5.0/fashion_xnor_ipq_parsed_delta_5.0_sample_index_8036"
#PATH_FOR_INPUT_QUERY= "/cs/labs/guykatz/guyam/bnn_artifact/xnor_experiment/300_ipqs/delta_3.0/fashion_xnor_ipq_parsed_delta_3.0_sample_index_1497"


# path for the log file of a single Marabou run - on the 6-layers BNN mlp nn
PATH_FOR_LOG_OUTPUT_OF_RUN = "/cs/labs/guykatz/guyam/testing_folder_to_delete" #todo change to "PUT_PATH_HERE"


def load_sample_manually(test_type, sample_index): # todo document
    """

    :param test_type:
    :param index_of_input_query_file:
    :return:
    """
    assert (test_type=="mlp" or test_type == "xnor")

    if test_type == "mlp":
        mnist_path_ending = "/bnn_artifact/mlp_experiment/digit_test_set.csv"
        mnist_digit_df = pd.read_csv(BNN_ARTIFACT_PARENT_DIRECTORY + mnist_path_ending)

        label_and_sample = np.array(mnist_digit_df)[sample_index]
        sample = label_and_sample[1::]

    elif test_type == "xnor":
        xnor_path_ending = "/bnn_artifact/xnor_experiment/fashion_test_set.csv"
        mnist_fashion_df = pd.read_csv(BNN_ARTIFACT_PARENT_DIRECTORY + xnor_path_ending)
        sample = np.array(mnist_fashion_df)[sample_index]

    # normalization of values
    sample = sample / 255.0
    return sample



def run_marabou_on_single_sample(input_query_file, output_directory, sample_index): # todo document
    """

    :param input_query_file:
    :param output_directory:
    :param sample_index:
    :return:
    """
    loaded_input_query = Marabou.load_query(filename=input_query_file)

    options = Marabou.createOptions(numWorkers=NUMBER_OF_CPU, snc=True, timeoutInSeconds=5000, restoreTreeStates=False,
                                    initialDivides=INITIAL_DIVIDES,  initialTimeout=5, sncSplittingStrategy="polarity")

    marabou_variable_assignment = Marabou.solve_query(ipq=loaded_input_query,
                                                      filename=output_directory + "/log_for_sample_" +
                                                               str(sample_index), verbose=True, options=options)

    adversary = list(marabou_variable_assignment[0].values())[: 784]
    return adversary



def verify_single_sample(test_type, input_query_file, output_directory): # todo conitnue documenting
    """
    This function receives a string representing a path for one of the 300 randomized input query text files, and also
    a string representing the output directory. The function parses
    :param input_query_file:
    :param output_directory:
    :return:
    """
    # save original image
    index_of_input_query_file = int(input_query_file[-4:])
    sample_to_check = load_sample_manually(test_type, index_of_input_query_file)
    original_sample = np.reshape(sample_to_check, newshape=(28,28))
    plt.imshow(original_sample)
    plt.colorbar()
    plt.savefig(output_directory + '/' + test_type + '_test_index_' + str(index_of_input_query_file))

    # find adversary
    adversary = run_marabou_on_single_sample (input_query_file = input_query_file, output_directory = output_directory,
                                              sample_index = index_of_input_query_file)
    # if an adversary exists - save the image
    if len(adversary) > 0:
        print(adversary) # todo delete
        reshaped_adversary = np.reshape(adversary, newshape=(28,28))
        plt.imshow(reshaped_adversary)
        plt.savefig(output_directory + '/adversary_for_index_' + str(index_of_input_query_file))
    else:
        print("no sat assignment was found, see the log file")
    return



if __name__ == '__main__':
    assert (len(sys.argv) == 2) # todo update - that we have 4 arguments (input / output paths as well)
    test_type = sys.argv[1]

    verify_single_sample(test_type=test_type, input_query_file=PATH_FOR_INPUT_QUERY,
                         output_directory=PATH_FOR_LOG_OUTPUT_OF_RUN)

