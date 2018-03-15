class Equation:
    """
    Python class to conveniently represent MarabouCore.Equation
    """
    def __init__(self):
        """
        Construct empty equation
        """
        self.addendList = []
        self.auxVar = None
        self.scalar = None
    
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

    def markAuxiliaryVariable(self, aux):
        """
        Mark variable as auxiliary for this equation
        Arguments:
            aux: (int) variable number of variable to mark
        """
        self.auxVar = aux

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
    aux = network.getNewVariable()
    network.setLowerBound(aux, 0.0)
    network.setUpperBound(aux, 0.0)
    e.markAuxiliaryVariable(aux)
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
    e = Equation()
    for i in range(len(vars)):
        e.addAddend(coeffs[i], vars[i])
    e.setScalar(scalar)
    aux = network.getNewVariable()
    network.setLowerBound(aux, 0.0)
    e.markAuxiliaryVariable(aux)
    e.addAddend(1.0, aux)
    network.addEquation(e)