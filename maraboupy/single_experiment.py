from maraboupy import Marabou
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
#from time import process_time
import time
import os


def run_single_experiment(ipq, experiment_number):
    timeoutTime = 7200 #NUMBER_OF_HOURS * 3600
    ipq_obj = Marabou.load_query(filename=ipq)
    # options = Marabou.createOptions(numWorkers=5, snc=True, timeoutInSeconds=timeoutTime) # was with , timeoutInSeconds= NUMBER_OF_HOURS *3600
    andrew_options = Marabou.createOptions(numWorkers=16, snc=True, timeoutInSeconds=timeoutTime, restoreTreeStates=False,
                                           initialDivides=4,  initialTimeout=0, splittingStrategy="polarity",
                                           splitThreshold=120)
    # options = Marabou.createOptions(timeoutInSeconds=timeoutTime)
    start_time = time.time()#process_time()

    log_path = OUTPUT_DIRECTORY + "/experiment_" + str(experiment_number) + ".log"
    dict_for_original = Marabou.solve_query(ipq=ipq_obj, filename=log_path, verbose=True, options=andrew_options)
    # dict_for_original = Marabou.solve_query(ipq=ipq_obj, filename=log_path, verbose=True)

    total_time = time.time() - start_time # was process time - time

    adversary, nn_output = list(dict_for_original[0].values())[: 784],  list(dict_for_original[0].values())[784: 794]

    print("the total time is: ", total_time)
    if len(adversary)==0:
        print("unsat")
    else:
        reshaped_sample_to_check = np.reshape(adversary, newshape=(28,28))
        plt.imshow(reshaped_sample_to_check)
        plt.colorbar()

    with open(OUTPUT_DIRECTORY +"/experiment_summary", "a") as output_file:
        output_file.write("experiment: " + str(experiment_number) + "\n")
        output_file.write("ipq: " + ipq + "\n")
        output_file.write("time: " + str(total_time) + "\n")
        output_file.write("adversary input:\n")
        output_file.write(str(adversary)+ "\n")
        output_file.write("log written to: " + log_path + "\n")
        output_file.write("********\n")
        output_file.close()


if __name__ == '__main__':
    # todo parameter
    #PATH_TO_FINAL_IPQ = "/cs/usr/guyam/Desktop/shortcut_guyam/cluster/delta_5.0/deep_6_ipq_parsed_delta_5.0_sample_index_9"
    #INPUT_CLUSTER_QUERIES_DIR = "/cs/usr/guyam/Desktop/shortcut_guyam/cluster/delta_8.0" # deep 6
    INPUT_CLUSTER_QUERIES_DIR = "/cs/labs/guykatz/guyam/cluster/xnor_flatten"
    # INPUT_CLUSTER_QUERIES_DIR = "/cs/usr/guyam/Desktop/shortcut_guyam/cluster/delta_0.0001"
    # INPUT_CLUSTER_QUERIES_DIR = "/cs/usr/guyam/Desktop/shortcut_guyam/cluster/forTOcheck"
    # INPUT_CLUSTER_QUERIES_DIR = "/cs/usr/guyam/Desktop/shortcut_guyam/cluster/delta_15.0" # smallBNN

    #NUMBER_OF_HOURS = 1
    OUTPUT_DIRECTORY = "/cs/labs/guykatz/guyam/experiments/xnor_before_submission"
        #"/cs/labs/guykatz/guyam/experiments/final_exp_deep_6_delta_1_all_flags_on"
    # OUTPUT_DIRECTORY = "/cs/labs/guykatz/guyam/experiments/final_exp_deep_6_delta_epsilon_all_flags_on"
    # OUTPUT_DIRECTORY = "/cs/labs/guykatz/guyam/experiments/final_exp_deep_6_all_off_except_gurobi_delta_5"
    #OUTPUT_DIRECTORY = "/cs/labs/guykatz/guyam/experiments/final_exp_deep_6_all_off_delta_3" # deep 6 final exp
    #OUTPUT_DIRECTORY = "/cs/usr/guyam/Desktop/shortcut_guyam/experiments/deep_6" # deep 6
    #OUTPUT_DIRECTORY = "/cs/usr/guyam/Desktop/shortcut_guyam/experiments/smallBNN" # small BNN

    list_of_queries = os.listdir(INPUT_CLUSTER_QUERIES_DIR)
    for experiment_number, ipq in enumerate(sorted(list_of_queries)):
        print(ipq) # todo delete
        run_single_experiment(ipq=INPUT_CLUSTER_QUERIES_DIR + "/" + ipq, experiment_number=experiment_number)



