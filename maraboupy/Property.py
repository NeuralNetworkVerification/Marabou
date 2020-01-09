import PropertyParser
import parser


'''
    Python class that represents a property 
    Contains two lists of strings: equations and properties
    Currently only supports properties that mention input and output variables only
    input variable i is referred to as inputs[i]
    output variable i is referred to as output[i]
    Evaluates the expressions (equations and bounds) with python's parser, by 
    turning the string into an executable expression
'''


class Property:

    def __init__(self,property_filename,compute_executable_bounds = False, compute_executable_equations = False):
        """
        Creates an object of type Property

        Main argument: property file path

        Computes lists of equations and bounds (stored as strings) that can be parsed and evaluated by python's parser

        NOTE: asserts that all equations and bounds are of the legal form, and only mention input (x) and
        output (y) variables.
        NOTE: changes the names of variables  xi to x[i] and yi to y[i]

        If requested, also computes lists of executable expressions (for efficiency of evaluation later)

        Attributes:
            self.equations = []         list of equations (strings)
            self.bounds = []            list of bounds (strings)
            self.exec_equations = []    list of equations (executable)
            self.exec_bounds = []       list of bounds (executable)


        """
        self.exec_equations = []
        self.exec_bounds = []

        if property_filename == "":
            print('No property file!')
            self.equations = []
            self.bounds = []
        else:
            self.equations, self.bounds = PropertyParser.parseProperty(property_filename)
        if compute_executable_bounds:
            self.compute_executable_bounds()
        if compute_executable_equations:
            self.compute_executable_equations()



    def compute_executable_bounds(self):
        """
        Computes the list of executable bounds for efficiency of evaluation
        NOTE: Does nothing if the list is already non-empty
        """
        if not self.exec_bounds:
            for bound in self.bounds:
                exec_equation = parser.expr(bound).compile()
                self.exec_bounds.append(exec_equation)

    def compute_executable_equations(self):
        """
        Computes the list of executable equations for efficiency of evaluation
        NOTE: Does nothing if the list is already non-empty
        """
        if not self.exec_equations:
            for equation in self.equations:
                exec_equation = parser.expr(equation).compile()
                self.exec_equations.append(exec_equation)

    def compute_executables(self):
        self.compute_executable_equations()
        self.compute_executable_bounds()


    def verify_equations(self,x,y):
        """
        Returns True iff all the equations hold on the input and the output variables
        :param x: list (inputs)
        :param y: list (outputs)
        :return: bool

        NOTE: x and y are lists (or np arrays) and they are used in the evaluation function,
        since they are encoded into the expressions in the lists equation and bounds
        """
        for equation in self.equations:
            exec_equation = parser.expr(equation).compile()
            return eval(exec_equation)

    def verify_bounds(self,x,y):
        """
        Returns True iff all the bounds hold on the input and the output variables
        :param x: list (inputs)
        :param y: list (outputs)
        :return: bool

        NOTE: x and y are lists (or np arrays) and they are used in the evaluation function,
        since they are encoded into the expressions in the lists equation and bounds
        """
        for bound in self.bounds:
            exec_equation = parser.expr(bound).compile()
            return eval(exec_equation)

    def verify_property(self,x,y):
        """
        Returns True iff the property holds on the input and the output variables
        :param x: list (inputs)
        :param y: list (outputs)
        :return: bool

        NOTE: x and y are lists (or np arrays) and they are used in the evaluation function,
        since they are encoded into the expressions in the lists equation and bounds
        """
        return self.verify_bounds(x, y) and self.verify_equations(x, y)


    def verify_equations_exec(self,x,y):
        """
        Returns True iff all the equations hold on the input and the output variables
        Verifies using the precomputed executable list
        Asserts that the list is non-empty

        :param x: list (inputs)
        :param y: list (outputs)
        :return: bool

        NOTE: x and y are lists (or np arrays) and they are used in the evaluation function,
        since they are encoded into the expressions in the lists equation and bounds
        """
        assert self.exec_equations
        for exec_equation in self.exec_equations:
            return eval(exec_equation)

    def verify_bounds_exec(self,x,y):
        """
        Returns True iff all the bounds hold on the input and the output variables
        Verifies using the precomputed executable list
        Asserts that the list is non-empty

        :param x: list (inputs)
        :param y: list (outputs)
        :return: bool

        NOTE: x and y are lists (or np arrays) and they are used in the evaluation function,
        since they are encoded into the expressions in the lists equation and bounds
        """
        assert self.exec_bounds
        for exec_equation in self.exec_bounds:
            return eval(exec_equation)

    def verify_property_exec(self,inputs,output):
        """
        Returns True iff the property holds on the input and the output variables
        Verifies using the precomputed executable list
        Asserts that the list is non-empty

        :param x: list (inputs)
        :param y: list (outputs)
        :return: bool

        NOTE: x and y are lists (or np arrays) and they are used in the evaluation function,
        since they are encoded into the expressions in the lists equation and bounds
        """
        return self.verify_bounds_exec(x, y) and self.verify_equations_exec(x, y)


