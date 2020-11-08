from maraboupy import Marabou
from maraboupy import equivalence

original_tiny_BNN_query = "/cs/usr/guyam/Desktop/shortcut_guyam/queries_to_test/tiny_BNN_query"
parsed_tiny_BNN_query = "/cs/usr/guyam/Desktop/shortcut_guyam/queries_to_test/tiny_BNN_query_parsed"

original_BNN_query_new = "/cs/usr/guyam/Desktop/shortcut_guyam/queries_to_test/BNN_query_new"
parsed_BNN_query_new = "/cs/usr/guyam/Desktop/shortcut_guyam/queries_to_test/BNN_query_new_parsed"

largeBNN_original = "/cs/usr/guyam/Desktop/shortcut_guyam/three_final_models/largeBNN_original"
largeBNN_parsed ="/cs/usr/guyam/Desktop/shortcut_guyam/three_final_models/largeBNN_parsed"

mediumBNN_original = "/cs/usr/guyam/Desktop/shortcut_guyam/three_final_models/mediumBNN_original"
mediumBNN_parsed ="/cs/usr/guyam/Desktop/shortcut_guyam/three_final_models/mediumBNN_parsed"

smallBNN_original = "/cs/usr/guyam/Desktop/shortcut_guyam/three_final_models/smallBNN_original"
smallBNN_parsed = "/cs/usr/guyam/Desktop/shortcut_guyam/three_final_models/smallBNN_parsed"

deep_6_layer_original = "/cs/usr/guyam/Desktop/shortcut_guyam/deep_nn/deep_6_original"
deep_6_layer_parsed = "/cs/usr/guyam/Desktop/shortcut_guyam/deep_nn/deep_6_parsed"

deep_11_layer_50_original = "/cs/usr/guyam/Desktop/shortcut_guyam/deep_nn/deep_11_50_original"
deep_11_layer_50_parsed = "/cs/usr/guyam/Desktop/shortcut_guyam/deep_nn/deep_11_50_parsed"

def compare_query_results(first_query, second_query):
    x_train, y_train, x_test, y_test = equivalence.load_data()
    x_test = equivalence.change_input_dimension(x_test)

    for sample_index in range(60,70):

        first_ipq = Marabou.load_query(filename=first_query)
        second_ipq = Marabou.load_query(filename=second_query)

        vec_single_sample = x_test[sample_index] # shape (784, )

        for input_var in range(784):
            first_ipq.setLowerBound(input_var, vec_single_sample[input_var])
            first_ipq.setUpperBound(input_var, vec_single_sample[input_var])

            second_ipq.setLowerBound(input_var, vec_single_sample[input_var])
            second_ipq.setUpperBound(input_var, vec_single_sample[input_var])

        dict_for_original = Marabou.solve_query(ipq=first_ipq, verbose=False)
        dict_for_parsed = Marabou.solve_query(ipq=second_ipq, verbose=False)

        results_for_original = list(dict_for_original[0].values())[784: 794]
        results_for_parsed = list(dict_for_parsed[0].values())[784: 794]

        assert (results_for_original == results_for_parsed)
        assert (results_for_original != [])

        print("test number: ", sample_index)

    print("PASSED TEST")




if __name__ == '__main__':
    compare_query_results(first_query=deep_11_layer_50_original, second_query=deep_11_layer_50_parsed)



    # compare_query_results(first_query=original_BNN_query_new, second_query=parsed_BNN_query_new)
    # compare_query_results(first_query=original_tiny_BNN_query, second_query=parsed_tiny_BNN_query)
    # compare_query_results(first_query=original_BNN_query, second_query=parsed_BNN_query)

# print("yo")