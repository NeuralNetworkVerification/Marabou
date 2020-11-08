import linecache
import tensorflow as tf

NUM_OF_INPUT_VARIABLES = 784
NUM_OF_OUTPUT_VARIABLES = 10
NUMBER_OF_METADATA_LINES = 5


def change_input_dimension(X):
    return X.reshape((X.shape[0],X.shape[1]*X.shape[2]))


def add_single_line(output_file, line):
    with open(output_file, "a") as output_file:
        output_file.write(line)
        output_file.write("\n")
        output_file.close()


def write_new_bound(output_file, line, new_bound):
    bound_list = line.split(",")
    variable = int(bound_list[0])
    new_line = str(variable) + "," + str(new_bound)
    add_single_line(output_file=output_file, line=new_line)


def create_to_equation(index_of_new_equation, true_label, runner_up_label):
    # creates an equation of type "854,2,0,786[RU],-1.0000000000,793[TRUE LABEL],1.0000000000"

    output_variables = list(range(784, 794))
    runner_up_label_var = output_variables[runner_up_label]
    true_label_var = output_variables[true_label]

    new_equation = str(int(index_of_new_equation)) + ",2,0," + str(runner_up_label_var) + ",-1.0000000000," + str(true_label_var) +",1.0000000000"
    return new_equation



def create_edited_query(input_path_for_parsed_ipq, delta, input_test_sample, true_label, runner_up_label, output_path):
    num_of_variables = linecache.getline(input_path_for_parsed_ipq, 1)
    num_of_lb = linecache.getline(input_path_for_parsed_ipq, 2)
    num_of_ub = linecache.getline(input_path_for_parsed_ipq, 3)
    num_of_equations = linecache.getline(input_path_for_parsed_ipq, 4)
    num_of_constraints = linecache.getline(input_path_for_parsed_ipq, 5)

    line_of_lb_start = NUMBER_OF_METADATA_LINES + 2 + NUM_OF_INPUT_VARIABLES + NUM_OF_OUTPUT_VARIABLES + 1
    line_of_ub_start = line_of_lb_start + int(num_of_lb)
    line_of_last_equation = line_of_ub_start + int(num_of_ub) + int(num_of_equations) - 1

    updated_metadata = [num_of_variables, num_of_lb, num_of_ub, str(int(num_of_equations) + 1)+"\n", num_of_constraints]

    # create array of new bounds
    new_input_bounds = []
    for i in range(784):
        fixed_lower_bound = max(0, input_test_sample[i]-abs(delta))
        fixed_upper_bound = min(1, input_test_sample[i]+abs(delta))
        new_input_bounds.append([fixed_lower_bound, fixed_upper_bound])

    # print(line_of_lb_start) # todo delete
    # print(line_of_ub_start) # todo delete
    # print(line_of_last_equation) # todo delete

    with open(output_path, "w") as output_file:
        output_file.writelines(updated_metadata)
        output_file.close()

    with open(input_path_for_parsed_ipq) as file:
        for index, line in enumerate(file.readlines(), 1):
            line = line.rstrip()
            # already added new metadata
            if 1 <= index <= NUMBER_OF_METADATA_LINES:
                continue

            # update tightest lower bound
            elif line_of_lb_start <= index <= line_of_lb_start + 783:
                index_of_variable = index - line_of_lb_start
                write_new_bound(output_path, line, new_input_bounds[index_of_variable][0])

            # update tightest upper bound
            elif line_of_ub_start <= index <= line_of_ub_start + 783:
                index_of_variable = index - line_of_ub_start
                write_new_bound(output_path, line, new_input_bounds[index_of_variable][1])

            # single new equation - represnting adversary quality
            elif index ==line_of_last_equation:
                add_single_line(output_path, line)
                new_equation = create_to_equation(num_of_equations, true_label, runner_up_label)
                add_single_line(output_path, new_equation)

                #print(new_equation) # todo delete

            else:
                add_single_line(output_path, line)


if __name__ == '__main__':

    parsed_iq_file_path = "/cs/usr/guyam/Desktop/shortcut_guyam/deep_6_parsed"
    #parsed_iq_file_path = "/cs/usr/guyam/Desktop/shortcut_guyam/smallBNN_parsed_draft"

    (x_train, y_train), (x_test, y_test) = tf.keras.datasets.mnist.load_data()
    x_train, x_test = x_train / 255.0, x_test / 255.0
    x_test = change_input_dimension(x_test)
    sample = x_test[9]

    TRUE_LABEL = 9
    RUNNER_UP_LABEL = 2
    DELTA = 5/255.0


    create_edited_query(input_path_for_parsed_ipq=parsed_iq_file_path, delta=DELTA, input_test_sample=sample, true_label=TRUE_LABEL,
                        runner_up_label=RUNNER_UP_LABEL, output_path = parsed_iq_file_path+"_edited")
