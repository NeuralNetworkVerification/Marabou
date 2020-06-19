
'''
/* *******************                                                        */
/*! \file PropertyParser.py
 ** \verbatim
 ** Top contributors (to current version):
 ** Alex Usvyatsov
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief
 ** Class that parses a property file into a list of strings representing the individual properties.
 **
 ** [[ Add lengthier description here ]]
 **/
'''



import sys
import re

types_of_property_by_variables = ['x', 'y', 'ws', 'm']
'''
    'x'     : input property (mentions only input variables)
    'y'     : output property (mentions only output variables)
    'ws'    : hidden variable property (mentions only hidden variables)
    'm'     : mixed

'''


def parseProperty(property_filename):

    '''
        Parses a property file using regular expressions
        Replaces occurrences of x?? (where ?? are digits) by x[??]
        Replaces occurrences of y?? (where ?? are digits) by y[??]
        (for convenience of evaluating the expression with python's parser)
        Returns:
             two dictionaries: equations and bounds
                each dictionary has as keys the type (types_of_property_by_variables) of property:
                'x','y','m',(mixed),'ws'

                values are lists of properties of the appropriate type
                    e.g., bounds['x'] is a list of bounds on input variables, where x?? has ben replaced by x[??]
                    so for example, bounds['x'] can look like this: ['x[0] >= 0', 'x[1] <= 0.01']

             a list of all properties, given as a list of tuples of strings:
             (class_of_property,type_of_property,property,index)
                where:
                    class_of_property is 'e' (equation) or 'b' (bound)
                    type_of_property is 'x','y','ws', or 'm' (for 'mixed'),
                    property is a line from the property file (unchanged)
                    index is the index of a bound/equation in the appropriate list

    '''

    properties = {'x': [], 'y': [], 'ws': [], 'm': []}

    equations = {'x': [], 'y': [], 'ws': [], 'm': []}
    bounds = {'x': [], 'y': [], 'ws': [], 'm': []}

    reg_input = re.compile(r'[x](\d+)')
    # matches a substring of the form x??? where ? are digits

    reg_output = re.compile(r'[y](\d+)')
    # matches a substring of the form y??? where ? are digits

    reg_ws = re.compile(r'[w][s][_](\d+)[_](\d+)')
    # matches a substring of the form ws_???_??? where ? are digits

    reg_equation = re.compile(r'[+-][xy](\d+) ([+-][xy](\d+) )+(<=|>=|=) [+-]?(\d+(\.\d*)?|\.\d+)([eE][+-]?\d+)?$')
    # matches a string that is a legal equation with input (x??) or output (y??) variables


    reg_bound = re.compile(r'[xy](\d+) (<=|>=|=) [+-]?(\d+(\.\d*)?|\.\d+)([eE][+-]?\d+)?')
    # matches a string which represents a legal bound on an input or an output variable

    reg_ws_bound = re.compile(r'[w][s][_](\d+)[_](\d+) (<=|>=|=) [+-]?(\d+(\.\d*)?|\.\d+)([eE][+-]?\d+)?')
    # matches a string which represents a legal bound on a hidden variable

    try:
        with open(property_filename) as f:
            line = f.readline()
            while(line):

                type_of_property = ''

                # Computing type_of_property: input/output/ws/mixed
                if (reg_input.search(line)): # input variables present
                    type_of_property = 'x'
                if (reg_output.search(line)): # output variables present
                    type_of_property = 'm' if type_of_property else 'y'
                if (reg_ws.search(line)): # hidden variable present
                    type_of_property = 'm' if type_of_property else 'ws'

                if not type_of_property:
                    print('An expression of an unknown type found while attempting to parse '
                          'property file ', property_filename)
                    print('expression: ', line)
                    sys.exit(1)

                if reg_equation.match(line):  # Equation

                    #replace xi by x[i] and yi by y[i]
                    new_str = line.strip()
                    new_str = reg_input.sub(r"x[\1]", new_str)

                    new_str = reg_output.sub(r"y[\1]", new_str)

                    # Debug
                    print('equation')
                    print(new_str)

                    index = len(equations[type_of_property])

                    equations[type_of_property].append(new_str)

                    class_of_property='e'

                elif reg_bound.match(line):  # I/O Bound

                    # replace xi by x[i] and yi by y[i]
                    new_str = line.strip()
                    new_str = reg_input.sub(r"x[\1]", new_str)

                    new_str = reg_output.sub(r"y[\1]", new_str)

                    print('bound: ', new_str)  #Debug

                    index = len(bounds[type_of_property])

                    bounds[type_of_property].append(new_str)

                    class_of_property = 'b'

                else:
                    assert reg_ws_bound.match(line)  # At this point the line has to match a legal ws bound

                    index = len(bounds['ws'])
                    bounds['ws'].append(line.strip()) # Storing without change

                    class_of_property = 'b'  # Perhaps at some point better add a new type for ws_bound?

                properties[type_of_property].append({'class_of_property': class_of_property,
                                                     'type_of_property': type_of_property,'line': line, 'index': index})

                line = f.readline()
        print('successfully read property file: ', property_filename)
        return equations, bounds, properties

    except:
        print('Something went wrong while parsing the property file', property_filename)
        sys.exit(1)

