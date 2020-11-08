from maraboupy import Marabou
import tensorflow as tf
import numpy as np


# NEW_PATH_2 = "/cs/usr/guyam/Desktop/shortcut_guyam/saveFileForamt2" # to dir
# outputName_2 = "StatefulPartitionedCall/StatefulPartitionedCall/sequential/FC_out_10/MatMul"
#
# NEW_PATH_5 = "/cs/usr/guyam/Desktop/shortcut_guyam/saveFileForamt5" # to dir
# outputName_5 ='StatefulPartitionedCall/StatefulPartitionedCall/sequential_4/FC_out_10/MatMul'
# PB_PATH = "/cs/usr/guyam/Desktop/shortcut_guyam/saved_model.pb"
#
# BOP_PATH = "/cs/usr/guyam/Desktop/shortcut_guyam/saveBop" # to dir
# outputName_bop ="StatefulPartitionedCall/StatefulPartitionedCall/sequential/FC_out_10/MatMul"


# PATH_tiny_two_layer = "/cs/labs/guykatz/guyam/models_to_test/tiny_two_layer"
# outputName_tiny_two_layer = "StatefulPartitionedCall/StatefulPartitionedCall/sequential/FC_out_10/MatMul"
#
# PATH_tiny_one_layer = "/cs/labs/guykatz/guyam/models_to_test/tiny_one_layer"
# outputName_tiny_one_layer = "StatefulPartitionedCall/StatefulPartitionedCall/sequential_1/FC_out_10/MatMul"

# todo until 0609

# PATH_finalBNN = "/cs/labs/guykatz/guyam/models_to_test/finalBNN" # to dir
# outputName_finalBNN = "StatefulPartitionedCall/StatefulPartitionedCall/sequential/FC_out_10/MatMul"
#
#
# outputName_hidden_500 = "StatefulPartitionedCall/StatefulPartitionedCall/sequential/FC_out_500/MatMul"
# outputName_hidden_300 = "StatefulPartitionedCall/StatefulPartitionedCall/sequential/FC_out_300/MatMul"
# outputName_hidden_200 = "StatefulPartitionedCall/StatefulPartitionedCall/sequential/FC_out_200/MatMul"
# outputName_hidden_100 = "StatefulPartitionedCall/StatefulPartitionedCall/sequential/FC_out_100/MatMul"
#
#
#
# PATH_small_BNN = "/cs/labs/guykatz/guyam/models_to_test/small_BNN"
# outputName_small_BNN = "StatefulPartitionedCall/StatefulPartitionedCall/sequential_3/FC_out_10/MatMul"
#
#
# PATH_really_small_BNN = "/cs/labs/guykatz/guyam/models_to_test/really_small_BNN"
# outputName_really_small_BNN = "StatefulPartitionedCall/StatefulPartitionedCall/sequential_5/FC_out_10/MatMul"


# TODO -  FINAL THREE - GOLD STANDARD - NN

input_largeBNN = "/cs/labs/guykatz/guyam/three_final_models/largeBNN"
output_largeBNN = "StatefulPartitionedCall/StatefulPartitionedCall/sequential/FC_out_10/MatMul"


input_mediumBNN = "/cs/usr/guyam/Desktop/shortcut_guyam/three_final_models/mediumBNN"
output_mediumBNN = "StatefulPartitionedCall/StatefulPartitionedCall/sequential/FC_out_10/MatMul"

input_smallBNN = "/cs/usr/guyam/Desktop/shortcut_guyam/three_final_models/smallBNN"
output_smallBNN = "StatefulPartitionedCall/StatefulPartitionedCall/sequential/FC_out_10/MatMul"

# todo - FINAL THREE - GOLD STANDARD - INPUT QUERIES

largeBNN_original = "/cs/usr/guyam/Desktop/shortcut_guyam/three_final_models/largeBNN_original"
largeBNN_parsed ="/cs/usr/guyam/Desktop/shortcut_guyam/three_final_models/largeBNN_parsed"

mediumBNN_original = "/cs/usr/guyam/Desktop/shortcut_guyam/three_final_models/mediumBNN_original"
mediumBNN_parsed ="/cs/usr/guyam/Desktop/shortcut_guyam/three_final_models/mediumBNN_parsed"

smallBNN_original = "/cs/usr/guyam/Desktop/shortcut_guyam/three_final_models/smallBNN_original"
smallBNN_parsed = "/cs/usr/guyam/Desktop/shortcut_guyam/three_final_models/smallBNN_parsed"


#TODO - Nina's NN
nina_network_input = "/cs/usr/guyam/Desktop/shortcut_guyam/ninas_nn/bnn_tf_networks-master/mnist/500_quant_0_1/model"
nina_network_output = "StatefulPartitionedCall"

#TODO - DEEP NN
deep_nn_6_input = "/cs/usr/guyam/Desktop/shortcut_guyam/deep_nn/deep_6_layers_50_10_BNN"
deep_nn_6_output = "StatefulPartitionedCall/StatefulPartitionedCall/sequential/FC_out_7/MatMul"

deep_nn_11_50_input = "/cs/usr/guyam/Desktop/shortcut_guyam/deep_nn/deep_11_layer_50_BNN"
deep_nn_11_50_output = "StatefulPartitionedCall/StatefulPartitionedCall/sequential_5/FC_out_12/MatMul"

deep_nn_11_100_input = "/cs/usr/guyam/Desktop/shortcut_guyam/deep_nn/deep_11_layer_100_BNN"
deep_nn_11_100_output = "bla"

deep_2_debug_input = "/cs/usr/guyam/Desktop/shortcut_guyam/deep_nn/deep_2_debug"
deep_2_debug_output = "StatefulPartitionedCall/StatefulPartitionedCall/sequential/FC_out_2/MatMul"


# todo - XNOR

micro_xnor_without_BN_input = "/cs/usr/guyam/Desktop/shortcut_guyam/xnor/micro_xnor_without_BN"
micro_xnor_without_BN_output = "StatefulPartitionedCall/StatefulPartitionedCall/sequential_18/quant_dense_24/MatMul"

# todo - tiniest XNOR
extra_micro_xnor_without_BN_input = "/cs/usr/guyam/Desktop/shortcut_guyam/xnor/extra_micro_xnor_without_BN"
extra_micro_xnor_without_BN_output = "StatefulPartitionedCall/StatefulPartitionedCall/sequential_1/quant_dense_1/MatMul"

# todo - *NEW* tiniest XNOR (WITHOUT SIGN ON INPUT) - DIGIT MNIST
new_extra_micro_xnor_without_BN_input = "/cs/usr/guyam/Desktop/shortcut_guyam/xnor/new_extra_micro_xnor_without_BN"
new_extra_micro_xnor_without_BN_output = "StatefulPartitionedCall/StatefulPartitionedCall/sequential_6/quant_dense_6/MatMul"

# todo *NEW* tiniest FASHION XNOR (WITHOUT SIGN ON INPUT)
fashion_xnor_input = "/cs/usr/guyam/Desktop/shortcut_guyam/xnor/fashion_xnor"
fashion_xnor_output = "StatefulPartitionedCall/StatefulPartitionedCall/sequential_1/quant_dense_1/MatMul"



def load_data():
    (x_train, y_train), (x_test, y_test) = tf.keras.datasets.mnist.load_data()
    #(x_train, y_train), (x_test, y_test) = tf.keras.datasets.fashion_mnist.load_data()
    # Rescale the images from [0,255] to the [0.0,1.0] range.
    x_train, x_test = x_train / 255.0, x_test / 255.0

    print("Number of original training examples:", len(x_train))
    print("Number of original test examples:", len(x_test))

    return x_train, y_train, x_test, y_test

# change dimension of input to vectors of the size 784

def change_input_dimension(X):
    return X.reshape((X.shape[0],X.shape[1]*X.shape[2]))

# def get_ith_vector(x):


def test_nn_equivalence(filename, modelType, outputName, num_of_samples_to_test):
    # pb_net = Marabou.read_tf(NEW_PATH_2, modelType='frozen', outputName=outputName_2) # todo del
    # net = Marabou.read_tf(NEW_PATH_5, modelType='savedModel_v2', outputName=outputName_5)
    net = Marabou.read_tf(filename=filename, modelType=modelType, outputName=outputName)

    x_train, y_train, x_test, y_test = load_data()
    x_test = change_input_dimension(x_test)

    # num_of_samples_to_test = 9

    for i in range(num_of_samples_to_test):

        sample_index = i*1000

        # single_sample = x_test[sample_index] # shape (28, 28)
        vec_single_sample = x_test[sample_index] # shape (784, )

        # output vec with Marabou
        yes_mar_res = net.evaluateWithMarabou(inputValues=[vec_single_sample])

        # output vec without Marabou
        no_mar_res = net.evaluateWithoutMarabou(inputValues=[vec_single_sample])

        # check that both 10-dim vectors are the same
        nn_output_equivalence = False not in ((yes_mar_res==no_mar_res)[0])
        assert(nn_output_equivalence)

        # check if classification is correct
        # classified_correcty = (y_test[sample_index] == np.argmax(yes_mar_res[0]))
        # assert (classified_correcty)

        print("classified: ", np.argmax(yes_mar_res[0]), " ; true label: " ,y_test[sample_index])
    print("PASSED TEST")

#
# def check_all_layers(): # TODO - unmask
#     print("hidden - 1")
#     test_nn_equivalence(filename=PATH_finalBNN, modelType='savedModel_v2',outputName=outputName_hidden_500,num_of_samples_to_test=9)
#     print("hidden - 2")
#     test_nn_equivalence(filename=PATH_finalBNN, modelType='savedModel_v2',outputName=outputName_hidden_300,num_of_samples_to_test=9)
#     print("hidden - 3")
#     test_nn_equivalence(filename=PATH_finalBNN, modelType='savedModel_v2',outputName=outputName_hidden_200,num_of_samples_to_test=9)
#     print("hidden - 4")
#     test_nn_equivalence(filename=PATH_finalBNN, modelType='savedModel_v2',outputName=outputName_hidden_100,num_of_samples_to_test=9)
#     print("last layer")
#     test_nn_equivalence(filename=PATH_finalBNN, modelType='savedModel_v2',outputName=outputName_finalBNN,num_of_samples_to_test=9)



def search_for_adversary(filename, modelType, outputName, num_iterations):

    net = Marabou.read_tf(filename=filename, modelType=modelType, outputName=outputName)

    x_train, y_train, x_test, y_test = load_data()
    x_test = change_input_dimension(x_test)

    for sample_idx in range(num_iterations):
        vec_single_sample = x_test[sample_idx] # shape (784, )

        # output vec with Marabou
        yes_mar_res = net.evaluateWithMarabou(inputValues=[vec_single_sample])

        print("test sample idx: ", str(sample_idx))
        print("output_vec: ", yes_mar_res)
        print("classified: ", np.argmax(yes_mar_res[0]), " ; true label: " ,y_test[sample_idx])
        print("***")



def classify(filename, modelType, outputName, x_vec):

    # for ninas - savedModelTags = ["model"]
    net = Marabou.read_tf(filename=filename, modelType=modelType, outputName=outputName)

    yes_mar_res = net.evaluateWithMarabou(inputValues=[x_vec])

    print("output_vec: ", yes_mar_res)
    print("classified: ", np.argmax(yes_mar_res[0]))
    print("***")


    # no_mar_res = net.evaluateWithoutMarabou(inputValues=[x_vec]) # todo delete
    # print("output_vec: ", no_mar_res) # todo delete
    return yes_mar_res

if __name__ == '__main__':

    x_train, y_train, x_test, y_test = load_data()
    x_test = change_input_dimension(x_test)

    # sample = x_test[9]
    #good_sample_indices = [2938, 2943, 2951, 2958]
    bad_sample_indices = [2937, 2940, 2941, 2944]
    for bad_index in bad_sample_indices:
        sample = x_test[bad_index]
        classify(filename=deep_nn_6_input, modelType="savedModel_v2", outputName=deep_nn_6_output, x_vec=sample)

    # test_nn_equivalence(filename=deep_nn_11_50_input, modelType="savedModel_v2", outputName=deep_nn_11_50_output, num_of_samples_to_test=10)
    #search_for_adversary(filename=input_smallBNN, modelType="savedModel_v2", outputName=output_smallBNN, num_iterations=10)

    #test_nn_equivalence(filename=new_extra_micro_xnor_without_BN_input, modelType="savedModel_v2", outputName=new_extra_micro_xnor_without_BN_output, num_of_samples_to_test=10)
    #search_for_adversary(filename=extra_micro_xnor_without_BN_input, modelType="savedModel_v2", outputName=extra_micro_xnor_without_BN_output, num_iterations=10)
    #classify(filename=extra_micro_xnor_without_BN_input, modelType="savedModel_v2", outputName=extra_micro_xnor_without_BN_output, x_vec=sample)

    # classify(filename=deep_2_debug_input, modelType="savedModel_v2", outputName=deep_2_debug_output, x_vec=sample)
    # test_nn_equivalence(filename=deep_2_debug_input, modelType="savedModel_v2", outputName=deep_2_debug_output, num_of_samples_to_test=10)
    # test_nn_equivalence(filename=deep_nn_6_input, modelType="savedModel_v2", outputName=deep_nn_6_output, num_of_samples_to_test=10)
    # search_for_adversary(filename=deep_nn_6_input, modelType="savedModel_v2", outputName=deep_nn_6_output, num_iterations=10)

# best_count = 0
# best_image_id = 0
# for sample_idx in range(len(x_test)):
#     current_non_zeros = np.count_nonzero(x_test[sample_idx])
#     if current_non_zeros > best_count:
#         best_count = current_non_zeros
#         best_image_id = sample_idx
# print("test")
# print("the best image is: ", best_image_id)
# print("with non-zeros: ", best_count)


# print(np.count_nonzero(x_test[9]))
# print(

# vec_single_sample = x_test[9] # shape (784, )
#
# ipq = Marabou.load_query(filename=smallBNN_parsed)
#
# for input_var in range(784): # todo remove when using the PARSED file
#     ipq.setLowerBound(input_var, vec_single_sample[input_var])
#     ipq.setUpperBound(input_var, vec_single_sample[input_var])
#
# dict_of_results = Marabou.solve_query(ipq=ipq, verbose=True)
#
# results = list(dict_of_results[0].values())[784: 794]
# # print("the results are: ", results)
# # [-8.0, -22.0, -22.0, -6.0, 10.0, -12.0, -20.0, 2.0, -10.0, 92.0] # for largeBNN
#
#
# # todo - NN version
# classify(filename=input_mediumBNN, modelType='savedModel_v2',outputName=output_mediumBNN, x_vec=vec_single_sample)
# classify(filename=input_largeBNN, modelType='savedModel_v2',outputName=output_largeBNN, x_vec=vec_single_sample)



# search_for_adversary(filename=input_smallBNN, modelType='savedModel_v2', outputName=output_smallBNN, num_iterations=10)
# search_for_adversary(filename=input_mediumBNN, modelType='savedModel_v2', outputName=output_mediumBNN, num_iterations=10)
# search_for_adversary(filename=input_largeBNN, modelType='savedModel_v2', outputName=output_largeBNN, num_iterations=10)


# test_nn_equivalence(filename=input_mediumBNN,modelType='savedModel_v2', outputName=output_mediumBNN, num_of_samples_to_test=10 )

# test_nn_equivalence(filename=input_largeBNN,modelType='savedModel_v2', outputName=output_largeBNN, num_of_samples_to_test=1 )

#search_for_adversary(filename=PATH_finalBNN, modelType="savedModel_v2", outputName=outputName_finalBNN, num_iterations=10)

# test_nn_equivalence(filename=PATH_tiny_one_layer, modelType="savedModel_v2", outputName=outputName_tiny_one_layer, num_of_samples_to_test=1)

# check_all_layers()

# test_nn_equivalence(filename=PATH_tiny_two_layer, modelType="savedModel_v2", outputName=outputName_tiny_two_layer, num_of_samples_to_test=1)


#test_nn_equivalence(filename=PATH_really_small_BNN, modelType="savedModel_v2", outputName=outputName_really_small_BNN, num_of_samples_to_test=1)

# search_for_adversary(filename=PATH_really_small_BNN, modelType="savedModel_v2", outputName=outputName_really_small_BNN, num_iterations=1)


# test_nn_equivalence(filename=PATH_small_BNN, modelType="savedModel_v2", outputName=outputName_small_BNN, num_of_samples_to_test=9)
# search_for_adversary(filename=PATH_small_BNN, modelType="savedModel_v2", outputName=outputName_small_BNN, num_iterations=10)

# artificial_example = np.zeros(784)
# check_all_layers()
# search_for_adversary(filename=PATH_finalBNN, modelType='savedModel_v2',outputName=outputName_finalBNN, num_iterations=10)



# TODO - CHECK variables which are not weights!!!
