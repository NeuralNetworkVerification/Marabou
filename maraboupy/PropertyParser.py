import re

'''
    Parses a property file using regular expressions
    Replaces occurrences of x?? (where ?? are digits) by x[??]
    Replaces occurrences of y?? (where ?? are digits) by y[??]
    (for convenience of evaluating the expression with python's parser)
    Returns two lists of strings: equations and bounds
'''

def parseProperty(property_filename):

    equations = []
    bounds = []

    reg_input = re.compile(r'[x](\d+)')
    # matches a subdtring of the form x??? where ? are digits

    reg_output = re.compile(r'[y](\d+)')
    # matches a subdtring of the form y??? where ? are digits

    reg_equation = re.compile(r'[+-][xy](\d+) ([+-][xy](\d+) )+(<=|>=|=) [+-]?(\d+(\.\d*)?|\.\d+)([eE][+-]?\d+)?$')
    # matches a string that is a legal equation with input (x??) or output (y??) variables

    reg_bound = re.compile(r'[xy](\d+) (<=|>=|=) [+-]?(\d+(\.\d*)?|\.\d+)([eE][+-]?\d+)?')
    # matches a string which represents a legal bound on an input or an output variable


    try:
        with open(property_filename) as f:
            line = f.readline()
            while(line):
                if reg_equation.match(line): #Equation

                    print('equation')

                    #replace xi by x[i] and yi by y[i]
                    new_str = line.strip()
                    new_str = reg_input.sub(r"x[\1]", new_str)

                    new_str = reg_output.sub(r"y[\1]", new_str)

                    print(new_str)

                    equations.append(new_str)
                else: #New bound
                    assert reg_bound.match(line) #At this point the line has to match a legal bound string

                    print('bound')

                    # replace xi by x[i] and yi by y[i]
                    new_str = line.strip()
                    new_str = reg_input.sub(r"x[\1]", new_str)

                    new_str = reg_output.sub(r"y[\1]", new_str)

                    bounds.append(new_str)
                line = f.readline()
        print('reading property into an nnet file, property: ', property_filename)
        return equations, bounds

    except:
        print('something went wrong while reading the property file', property_filename)

