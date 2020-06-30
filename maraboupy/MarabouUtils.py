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

from maraboupy import MarabouCore

class Equation:
    """
    Python class to conveniently represent MarabouCore.Equation
    """
    def __init__(self, EquationType=MarabouCore.Equation.EQ):
        """
        Construct empty equation
        """
        self.addendList = []
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

