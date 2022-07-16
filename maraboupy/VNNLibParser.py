'''
Top contributors (to current version):
    - Andrew Wu
This file is part of the Marabou project.
Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
in the top-level source directory) and their institutional affiliations.
All rights reserved. See the file COPYING in the top-level source
directory for licensing information.
'''

from maraboupy.MarabouUtils import *

from os.path import isfile

def readVNNLibFile(filename, network):
    assert(isfile(filename))
    varMap = dict()
    counter = 0
    for lst in network.inputVars:
        for var in lst.flatten():
            varMap["X_{}".format(counter)] = var
            counter += 1
    assert(len(network.outputVars) == 1)
    for i, var in enumerate(network.outputVars[0].flatten()):
        varMap["Y_{}".format(i)] = var

    with open(filename, 'r') as in_file:
        line = in_file.readline()
        openParanthesis, currentConstraint = parseLine(line.strip(),
                                                       network, 0, "")
        while True:
            if openParanthesis == 0:
                processConstraint(currentConstraint, network, varMap)
                line = in_file.readline()
                if not line:
                    break
                openParanthesis, currentConstraint = parseLine(line.strip(),
                                                               network, 0, "")
            else:
                line = in_file.readline()
                if not line:
                    break
                openParanthesis, currentConstraint = parseLine(line.strip(),
                                                               network,
                                                               openParanthesis,
                                                               currentConstraint)
        assert(openParanthesis == 0)
    return

# Returns number of open paranthesis in the first argument
def parseLine(line, network, openParanthesis, currentConstraint):
    assert(openParanthesis >= 0)
    if  line == "" or line[0] == ';':
        assert(openParanthesis == 0)
        return openParanthesis, None
    elif "declare-const" in line:
        assert(openParanthesis == 0)
        return 0, None
    else:
        numLeft = line.count('(')
        numRight = line.count(')')
        return openParanthesis + numLeft - numRight, currentConstraint + " " + line

def processConstraint(constraint, network, varMap):
    if constraint == None:
        return
    constraint = constraint.strip()
    assert(constraint[0] == "(")
    assert(constraint[-1] == ")")
    operator = constraint[1:-1].split("(")[0].split()[0]
    if operator == "assert":
        processConstraint(constraint[7:-1], network, varMap)
    elif operator == "<=":
        leq = getLeqConstraint(constraint[3:-1] , network, varMap)
        network.addEquation(leq)
    elif operator == ">=":
        geq = getGeqConstraint(constraint[3:-1] , network, varMap)
        network.addEquation(geq)
    elif operator == "or":
        disjuncts = getDisjuncts(constraint[3:-1], network, varMap)
        network.addDisjunctionConstraint(disjuncts)

def getLeqConstraint(constraint, network, varMap):
    constraint = constraint.strip()
    operands = constraint.split()
    assert(len(operands) == 2)
    eq = Equation(MarabouCore.Equation.LE)
    eq.setScalar(0)
    operand1 = operands[0]
    operand2 = operands[1]
    if operand1 in varMap:
        eq.addAddend(1, varMap[operand1])
    if operand2 in varMap:
        eq.addAddend(-1, varMap[operand2])
    else:
        scalar = float(operand2)
        eq.setScalar(scalar)
    return eq

def getGeqConstraint(constraint, network, varMap):
    constraint = constraint.strip()
    operands = constraint.split()
    assert(len(operands) == 2)
    eq = Equation(MarabouCore.Equation.GE)
    eq.setScalar(0)
    operand1 = operands[0]
    operand2 = operands[1]
    if operand1 in varMap:
        eq.addAddend(1, varMap[operand1])
    if operand2 in varMap:
        eq.addAddend(-1, varMap[operand2])
    else:
        scalar = float(operand2)
        eq.setScalar(scalar)
    return eq

def toMarabouEquation(equation):
    eq = MarabouCore.Equation(equation.EquationType)
    eq.setScalar(equation.scalar)
    for (c, x) in equation.addendList:
        eq.addAddend(c, x)
    return eq

def getDisjuncts(constraint, network, varMap):
    # returns list of list of constraints
    tokens = constraint.strip().split()
    disjuncts = []
    conjuncts = ""
    inConj = False
    for token in tokens:
        if token == "(and":
            inConj = True
            continue
        elif inConj and "))" in token:
            conjuncts += " " + token[:-1]
            disjuncts.append(getConjuncts(conjuncts, network, varMap))
            conjuncts = ""
            inConj = False
        elif inConj:
            conjuncts += " " + token
    return disjuncts

def getConjuncts(conjuncts, network, varMap):
    conjuncts = conjuncts.strip().split(")")
    conjunction = []
    for conjunct in conjuncts:
        conjunct = conjunct.strip()
        if "<=" in conjunct:
            eq = getLeqConstraint(conjunct[3:], network, varMap)
            conjunction.append(toMarabouEquation(eq))
        elif ">=" in conjunct:
            eq = getGeqConstraint(conjunct[3:], network, varMap)
            conjunction.append(toMarabouEquation(eq))
    return conjunction
