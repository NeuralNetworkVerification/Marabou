
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




import re


def parseProperty(property_filename):

    '''
        Parses a property file using regular expressions
        Replaces occurrences of x?? (where ?? are digits) by x[??]
        Replaces occurrences of y?? (where ?? are digits) by y[??]
        (for convenience of evaluating the expression with python's parser)
        Returns two lists of strings: equations and bounds
    '''

    equations = []
    bounds = []

    reg_input = re.compile(r'[x](\d+)')
    # matches a substring of the form x??? where ? are digits

    reg_output = re.compile(r'[y](\d+)')
    # matches a substring of the form y??? where ? are digits

    reg_equation = re.compile(r'[+-][xy](\d+) ([+-][xy](\d+) )+(<=|>=|=) [+-]?(\d+(\.\d*)?|\.\d+)([eE][+-]?\d+)?$')
    # matches a string that is a legal equation with input (x??) or output (y??) variables

    reg_bound = re.compile(r'[xy](\d+) (<=|>=|=) [+-]?(\d+(\.\d*)?|\.\d+)([eE][+-]?\d+)?')
    # matches a string which represents a legal bound on an input or an output variable


    try:
        with open(property_filename) as f:
            line = f.readline()
            while(line):
                if reg_equation.match(line): #Equation


                    #replace xi by x[i] and yi by y[i]
                    new_str = line.strip()
                    new_str = reg_input.sub(r"x[\1]", new_str)

                    new_str = reg_output.sub(r"y[\1]", new_str)

                    # Debug
                    print('equation')
                    print(new_str)

                    equations.append(new_str)
                else: #New bound
                    assert reg_bound.match(line) #At this point the line has to match a legal bound


                    # replace xi by x[i] and yi by y[i]
                    new_str = line.strip()
                    new_str = reg_input.sub(r"x[\1]", new_str)

                    new_str = reg_output.sub(r"y[\1]", new_str)

                    print('bound: ', new_str) #Debug



                    bounds.append(new_str)
                line = f.readline()
        print('successfully read property file: ', property_filename)
        return equations, bounds

    except:
        print('something went wrong while reading the property file', property_filename)

