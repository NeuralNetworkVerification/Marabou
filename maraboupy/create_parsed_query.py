INPUT_SIZE = 784
OUTPUT_SIZE = 10
NUMBER_OF_METADATA_LINES = 5

START_INDEX_OF_INPUT_VARIABLES = NUMBER_OF_METADATA_LINES + 2 # first line AFTER input variables
START_INDEX_OF_OUTPUT_VARIABLES = START_INDEX_OF_INPUT_VARIABLES + INPUT_SIZE + 1

NUMBER_OF_VARIABLES = 0

START_INDEX_OF_LB = 0
NUMBER_OF_LB = 0

START_INDEX_OF_UB = 0
NUMBER_OF_UB = 0

START_INDEX_OF_EQ = 0
NUMBER_OF_EQ = 0

START_INDEX_OF_CONSTRAINTS = 0
NUMBER_OF_CONSTRAINTS = 0

ELIMINATION_OFFSET = -0.1000000015

def find_global_variables(input_path):
    with open(input_path) as file:
        global NUMBER_OF_VARIABLES, NUMBER_OF_LB, NUMBER_OF_UB, NUMBER_OF_EQ, NUMBER_OF_CONSTRAINTS
        metadata_lines = [int(next(file).rstrip()) for x in range(NUMBER_OF_METADATA_LINES)]

        NUMBER_OF_VARIABLES = metadata_lines[0]
        NUMBER_OF_LB = metadata_lines[1]
        NUMBER_OF_UB = metadata_lines[2]
        NUMBER_OF_EQ = metadata_lines[3]
        NUMBER_OF_CONSTRAINTS = metadata_lines[4]

    global START_INDEX_OF_LB, START_INDEX_OF_UB, START_INDEX_OF_EQ, START_INDEX_OF_CONSTRAINTS

    START_INDEX_OF_LB = START_INDEX_OF_OUTPUT_VARIABLES + OUTPUT_SIZE
    START_INDEX_OF_UB = START_INDEX_OF_LB + NUMBER_OF_LB
    START_INDEX_OF_EQ = START_INDEX_OF_UB + NUMBER_OF_UB
    START_INDEX_OF_CONSTRAINTS = START_INDEX_OF_EQ + NUMBER_OF_EQ

def find_equation_boundaries_of_bad_eq(input_path, last_equation_index, gap, renaming_dict, deleting_set):
    # gaps here are 784, 30
    with open(input_path) as file:
        for index, line in enumerate(file.readlines(), 1):
            line = line.rstrip()

            # if the line is of an equation
            if START_INDEX_OF_EQ + last_equation_index < index < START_INDEX_OF_CONSTRAINTS:
                equation_list = line.split(",")
                equation_number = int(equation_list[0])
                equation_scalar = float(equation_list[2])
                first_var_to_change = int(equation_list[5])

                if equation_scalar == ELIMINATION_OFFSET:
                    # first var to change -> var c
                    update_var_renaming(first_var_to_change, renaming_dict, gap)
                    update_var_to_delete(first_var_to_change, deleting_set, gap)
                    return equation_number, equation_number + gap - 1

def update_var_to_delete(first_var_to_change, deleting_set, gap):
    for offset in range(gap):
        first_variable_to_delete = first_var_to_change + offset
        second_variable_to_delete = first_variable_to_delete + gap

        deleting_set.add(first_variable_to_delete)
        deleting_set.add(second_variable_to_delete)


def update_var_renaming(first_var_to_change, renaming_dict, gap):
    for offset in range(gap):
        var_to_change = first_var_to_change + gap + offset
        new_value = var_to_change - (2 * gap)
        renaming_dict[var_to_change] = new_value


def eq_to_delete(eq_del_boundaries, eq_index):
    eq_in_bounds = True in [eq_index in range(gap[0], gap[1] + 1) for gap in eq_del_boundaries]
    return eq_in_bounds


def get_gap_list(input_path):
    gaps = []
    count = 0
    currently_counting = False
    with open(input_path) as file:
        for index, line in enumerate(file.readlines(), 1):
            line = line.rstrip()
            # if the line is of an equation
            if START_INDEX_OF_EQ < index < START_INDEX_OF_CONSTRAINTS-1:
                equation_list = line.split(",")
                equation_scalar = float(equation_list[2])
                if equation_scalar == ELIMINATION_OFFSET:
                    if currently_counting:
                        count += 1
                    else: # not currently counting - start new batch
                        count = 1
                        currently_counting = True
                else: # finishing round
                    if currently_counting:
                        gaps.append(count)
                        currently_counting = False
                        count = 0

            if index == START_INDEX_OF_CONSTRAINTS - 1:# last index of eq is valid batch
                if currently_counting:
                    count += 1
                    gaps.append(count)
        return gaps


#gap_lst = [784, 30]
def parse_input_file(input_path, gap_list):
    variables_to_rename = {}
    variables_to_delete = set()

    list_of_eq_boundaries_to_delete = []
    last_equation_index = -1
    for gap in gap_list:
        bad_eq_cycle = find_equation_boundaries_of_bad_eq(input_path=input_path, last_equation_index=last_equation_index, gap=gap, renaming_dict=variables_to_rename, deleting_set=variables_to_delete)
        list_of_eq_boundaries_to_delete.append(bad_eq_cycle)
        last_equation_index = bad_eq_cycle[1]

    return variables_to_rename, variables_to_delete, list_of_eq_boundaries_to_delete

def create_new_metadata(total_variables_deleted):
    number_of_sign_variables_deleted = int(total_variables_deleted / 2)

    new_num_of_variables = str(NUMBER_OF_VARIABLES)+"\n"
    # assume bounds are only on SIGN variables
    new_size_of_bounds = str(NUMBER_OF_LB - number_of_sign_variables_deleted + 784)+"\n"
    new_number_of_equations = str(NUMBER_OF_EQ - number_of_sign_variables_deleted)+"\n"
    new_number_of_constraints = str(NUMBER_OF_CONSTRAINTS - number_of_sign_variables_deleted)+"\n"

    new_metadata = [new_num_of_variables, new_size_of_bounds, new_size_of_bounds, new_number_of_equations, new_number_of_constraints]
    return new_metadata

def add_single_line(output_file, line):
    with open(output_file, "a") as output_file:
        output_file.write(line)
        output_file.write("\n")
        output_file.close()

def write_new_bound(output_file, line, renaming_dict, variables_to_delete):
    bound_list = line.split(",")
    variable = int(bound_list[0])
    bound = bound_list[1]
    if variable in variables_to_delete:
        return
    if variable in renaming_dict:
        variable = renaming_dict[variable]
    new_lb = str(variable) + "," + bound
    add_single_line(output_file=output_file.name, line=new_lb)

def write_new_equation(output_file, line, renaming_dict, eq_boundaries_to_delete, last_eq_index):
    equation_list = line.split(",")
    equation_number = int(equation_list[0])

    new_equation_list = equation_list[:3]
    remaining_original_equation_list = equation_list[3:]
    if eq_to_delete(eq_index=equation_number, eq_del_boundaries=eq_boundaries_to_delete):
        return last_eq_index

    for i in range(len(remaining_original_equation_list)):
        if i % 2 == 0: # a variable
            original_var = remaining_original_equation_list[i]
            if int(original_var) in renaming_dict:
                original_var = str(renaming_dict[int(original_var)])
            new_element = original_var
        else: # not a variable
            new_element = remaining_original_equation_list[i]
        new_equation_list.append(new_element)
    new_equation_list[0] = str(last_eq_index + 1)
    new_equation_str = ",".join(new_equation_list)
    add_single_line(output_file=output_file.name, line=new_equation_str)
    return last_eq_index + 1


def write_new_constraint(output_file, line, renaming_dict, variables_to_delete, last_constraint_index):
    constraint_list = line.split(",")
    constraint = constraint_list[1]
    assert (constraint in ["sign","max"]) # todo continue
    f_variable = int(constraint_list[2])
    if f_variable in variables_to_delete:
        return last_constraint_index
    constraint_list[0] = str(last_constraint_index + 1)
    if constraint=="max":
        for b_var_index in range(2,len(constraint_list[2:])):
            if int(constraint_list[b_var_index]) in renaming_dict:
                constraint_list[b_var_index]=str(renaming_dict[int(constraint_list[b_var_index])])

    new_constraint = (",").join(constraint_list)
    add_single_line(output_file=output_file.name, line=new_constraint)
    return last_constraint_index + 1


def update_bounds_for_all_input_variables(output_file, bound_str):
    for input_var in range(784):
        new_bound = str(input_var)+","+bound_str
        add_single_line(output_file=output_file.name, line=new_bound)


def create_new_parsed_ipq (input_path, output_path):
    find_global_variables(input_path=input_path)
    gap_list = get_gap_list(input_path=input_path)

    renaming_dict, variables_to_delete, eq_boundaries_to_delete = parse_input_file(input_path=input_path, gap_list=gap_list)
    total_variables_deleted = len(variables_to_delete)
    updated_metadata = create_new_metadata(total_variables_deleted)

    input_lb_updated = False
    input_ub_updated = False

    # create new file
    with open(output_path, "w") as output_file:
        output_file.writelines(updated_metadata)
        output_file.close()
    with open(input_path) as file:
        last_eq_index, last_constraint_index = -1, -1
        for index, line in enumerate(file.readlines(), 1):
            line = line.rstrip()
            # already added new metadata
            if 1 <= index <= NUMBER_OF_METADATA_LINES:
                continue
            # input variables and output variables are the same as the original input
            elif NUMBER_OF_METADATA_LINES < index < START_INDEX_OF_LB:
                add_single_line(output_file=output_path, line=line)

            elif START_INDEX_OF_LB <= index < START_INDEX_OF_UB:
                if not input_lb_updated:
                    update_bounds_for_all_input_variables(output_file, "0.0")
                    input_lb_updated = True
                write_new_bound(output_file, line, renaming_dict, variables_to_delete)

            elif START_INDEX_OF_UB <= index < START_INDEX_OF_EQ:
                if not input_ub_updated:
                    update_bounds_for_all_input_variables(output_file, "1.0000000000")
                    input_ub_updated = True
                write_new_bound(output_file, line, renaming_dict, variables_to_delete)

            elif START_INDEX_OF_EQ <= index < START_INDEX_OF_CONSTRAINTS:
                last_eq_index = write_new_equation(output_file, line, renaming_dict, eq_boundaries_to_delete, last_eq_index)

            elif START_INDEX_OF_CONSTRAINTS <= index:
                last_constraint_index = write_new_constraint(output_file, line, renaming_dict,variables_to_delete, last_constraint_index)


# if __name__ == '__main__':
#     create_new_parsed_ipq(input_path=micro_input, output_path=micro_output)