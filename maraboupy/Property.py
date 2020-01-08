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

    def __init__(self,property_filename):
        if property_filename == "":
            print 'HERE?????'
            self.equations = []
            self.bounds = []
        else:
            print 'HERE!!!!!!'
            self.equations, self.bounds = PropertyParser.parseProperty(property_filename)

    def verify_equations(self,inputs,output):
        for equation in self.equations:
            exec_equation = parser.expr(equation).compile()
            if not eval(exec_equation):
                return False
            return True

    def verify_bounds(self,inputs,output):
        for bound in self.bounds:
            exec_equation = parser.expr(bound).compile()
            if not eval(exec_equation):
                return False
            return True

    def verify_property(self,inputs,output):
        self.verify_bounds(inputs,output)
        self.verify_equations(inputs,output)


