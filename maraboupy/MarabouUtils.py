'''
/* *******************                                                        */
/*! \file MarabouUtils.py
 ** \verbatim
 ** Top contributors (to current version):
 **   Christopher Lazarus, Shantanu Thakoor, Kyle Julian
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief [[ Add one-line brief description here ]]
 **
 ** [[ Add lengthier description here ]]
 **/
'''

from . import MarabouCore
import numpy as np

class Equation:
    """
    Python class to conveniently represent MarabouCore.Equation
    """
    def __init__(self, EquationType=MarabouCore.Equation.EQ):
        """
        Construct empty equation
        """
        self.addendList = []
        self.participatingVariables = set()
        self.scalar = None
        self.EquationType = EquationType

    def setScalar(self, x):
        """
        Set scalar of equation
        Arguments:
            x: (float) scalar RHS of equation
        """
        self.scalar = x

    def addAddend(self, c, x):
        """
        Add addend to equation
        Arguments:
            c: (float) coefficient of addend
            x: (int) variable number of variable in addend
        """
        self.addendList += [(c, x)]
        self.participatingVariables.update([x])

    def getParticipatingVariables(self):
        """
        Returns set of variables participating in this equation
        """
        return self.participatingVariables

    def participatingVariable(self, var):
        """
        Check if the variable participates in this equation
        Arguments:
            var: (int) variable number to check
        """
        return var in self.getParticipatingVariables()

    def replaceVariable(self, x, xprime, c):
        """
        Replace x with xprime + c
        Arguments:
            x: (int) old variable to be replaced in this equation
            xprime: (int) new variable to be added, does not participate in this equation
            c: (float) difference between old and new variable
        """
        assert self.participatingVariable(x)
        assert not self.participatingVariable(xprime)
        for i in range(len(self.addendList)):
            if self.addendList[i][1] == x:
                coeff = self.addendList[i][0]
                self.addendList[i] = (coeff, xprime)
                self.setScalar(self.scalar - coeff*c)
                self.participatingVariables.remove(x)
                self.participatingVariables.update([xprime])

def addEquality(network, vars, coeffs, scalar):
    """
    Function to conveniently add equality constraint to network
    \sum_i vars_i*coeffs_i = scalar
    Arguments:
        network: (MarabouNetwork) to which to add constraint
        vars: (list) of variable numbers
        coeffs: (list) of coefficients
        scalar: (float) representing RHS of equation
    """
    assert len(vars)==len(coeffs)
    e = Equation()
    for i in range(len(vars)):
        e.addAddend(coeffs[i], vars[i])
    e.setScalar(scalar)
    network.addEquation(e)

def addInequality(network, vars, coeffs, scalar):
    """
    Function to conveniently add inequality constraint to network
    \sum_i vars_i*coeffs_i <= scalar
    Arguments:
        network: (MarabouNetwork) to which to add constraint
        vars: (list) of variable numbers
        coeffs: (list) of coefficients
        scalar: (float) representing RHS of equation
    """
    assert len(vars)==len(coeffs)
    e = Equation(MarabouCore.Equation.LE)
    for i in range(len(vars)):
        e.addAddend(coeffs[i], vars[i])
    e.setScalar(scalar)
    network.addEquation(e)

def addComplementOutputSets(network, var_tuples):
    """
    Construct multiple complement output sets and "OR"s them together.
    Arguments:
        network: (MarabouNetwork) to which to add constraint
        List of tuples of the form:
            [(LB: (float) specifying the lower bound,
             UB: (float) specifying the upper bound,
             x: (int) specifying the variable), 
             (,,), 
             ...]
    """
    assert len(var_tuples) > 1
    LB,UB,x = var_tuples.pop(0)
    Y = addComplementOutputSet(network, LB, UB, x)
    for (LB,UB,x) in var_tuples:
        Z = addComplementOutputSet(network, LB, UB, x)
        Q = network.getNewVariable()
        network.addMaxConstraint({Y,Z}, Q)
        Y = Q
    # set the lower bound enforcing the "OR" on all the complement output set 
    # constraints
    network.setLowerBound(Q, 0.0)
    return True

def addComplementOutputSet(network, LB, UB, x):
    """
    Function to convert an output specification of staying within a set defined 
    by a lower bound and upper bound to its complement appropriate for Marabou. 
    Returns a variable. May be "OR"ed with other variables or a lower bound of 0
    may be set on the output of this function.
    Arguments:
        network: (MarabouNetwork) to which to add constraint
        LB: (float) specifying the lower bound
        UB: (float) specifying the upper bound
        x: (int) specifying the variable
    """
    # define x_l = l - x
    x_l = network.getNewVariable()
    eq = Equation()
    eq.addAddend(1.0, x_l)
    eq.addAddend(1.0, x)
    eq.setScalar(LB)
    network.addEquation(eq)
    # define x_u = x - u
    x_u = network.getNewVariable()
    eq = Equation()
    eq.addAddend(1.0, x_u)
    eq.addAddend(-1.0, x)
    eq.setScalar(-UB)
    network.addEquation(eq)
    #
    # For a validity interface we would want both x_l and x_u to be negative, but
    # for Marabou's satisfiability interface, we assert that one of the
    # constraints must be in violation, meaning one on them is greater than zero
    # max(x_l, x_u) > 0
    Y = network.getNewVariable()
    network.addMaxConstraint({x_l,x_u}, Y)
    #network.setLowerBound(Y, 0.0)
    print("Instead of setting a bound on ", x, " we are creating x_l:",x_l," and x_u:",x_u," and Y:",Y,"")#and setting a bound on Y")
    return Y