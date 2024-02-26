"""
Top contributors (to current version):
    - Min Wu

This file is part of the Marabou project.
Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
in the top-level source directory) and their institutional affiliations.
All rights reserved. See the file COPYING in the top-level source
directory for licensing information.

MarabouPythonic defines the Pythonic API of Marabou.
Assuming neurons x y z and constants a b c d, formats like the following are supported:
x >= a
y <= b
z == c
a * x <= b
x * a + b >= c * y - d
- a * x + b * y + c * z == d
 a * (x * b + c) <= d
"""

from typing import Optional, DefaultDict
from collections import defaultdict
from dataclasses import dataclass


@dataclass()
class VarAffine:
    """
    Define affine transformation of neurons in a neural network.
    Assuming neuron x and constants a b, an example affine transformation is a * x + b.

    Attributes:
        varCoeffs: neurons (int) and their coefficients (float), e.g., {x, a} denotes a * x.
        const: constant of the affine transformation, e.g., b.
    """
    varCoeffs: DefaultDict[int, float]
    scalar: float

    def __add__(self, other):
        """
        Addition of a neuron's affine transformation with a constant or another affine transformation.
        e.g., a * x + b + b' or a * x + b + a' * x' + b'.

        :param other: the addend could be a constant (float, int) or an affine transformation.
        :return: the result of addition is an affine transformation of neuron(s).
        """
        assert isinstance(other, float) or isinstance(other, int) or isinstance(other, VarAffine)
        if isinstance(other, float) or isinstance(other, int):
            return VarAffine(self.varCoeffs, self.scalar + other)
        elif isinstance(other, VarAffine):
            varCoeffs = defaultdict(float)
            for variable in self.varCoeffs:
                varCoeffs[variable] += self.varCoeffs[variable]
            for variable in other.varCoeffs:
                varCoeffs[variable] += other.varCoeffs[variable]
            return VarAffine(varCoeffs, self.scalar + other.scalar)

    def __sub__(self, other):
        """
        Subtraction of a neuron's affine transformation with a constant or another affine transformation.
        e.g., a * x + b - b' or a * x + b - a' * x' - b'.

        :param other: the subtrahend could be a constant (float, int) or an affine transformation.
        :return: the result of subtraction is an affine transformation of neuron(s).
        """
        assert isinstance(other, float) or isinstance(other, int) or isinstance(other, VarAffine)
        if isinstance(other, float) or isinstance(other, int):
            return VarAffine(self.varCoeffs, self.scalar - other)
        elif isinstance(other, VarAffine):
            varCoeffs = defaultdict(float)
            for variable in self.varCoeffs:
                varCoeffs[variable] += self.varCoeffs[variable]
            for variable in other.varCoeffs:
                varCoeffs[variable] -= other.varCoeffs[variable]
            return VarAffine(varCoeffs, self.scalar - other.scalar)

    def __radd__(self, other):
        """
        Reverse addition of a neuron's affine transformation with a constant or another affine transformation.
        e.g., b' + a * x + b or a' * x' + b' + a * x + b.
        Note: reverse addition between two affine transformations will use the pre-defined addition.

        :param other: the addend could be a constant (float, int) or an affine transformation.
        :return: the result of reverse addition is an affine transformation of neuron(s).
        """
        assert isinstance(other, float) or isinstance(other, int) or isinstance(other, VarAffine)
        if isinstance(other, float) or isinstance(other, int):
            return VarAffine(self.varCoeffs, self.scalar + other)
        elif isinstance(other, VarAffine):
            raise NotImplementedError

    def __rsub__(self, other):
        """
        Reverse subtraction of a neuron's affine transformation with a constant or another affine transformation.
        e.g., b' - a * x - b or a' * x' + b' - a * x - b.
        Note: reverse subtraction between two affine transformations will use the pre-defined subtraction.

        :param other: the minuend could be a constant (float, int) or an affine transformation.
        :return: the result of reverse subtraction is an affine transformation of neuron(s).
        """
        assert isinstance(other, float) or isinstance(other, int) or isinstance(other, VarAffine)
        if isinstance(other, float) or isinstance(other, int):
            return - self + other
        elif isinstance(other, VarAffine):
            raise NotImplementedError

    def __mul__(self, other):
        """
        Multiplication of a neuron's affine transformation with a constant.
        e.g., (a * x + b) * b' is supported while (a * x + b) * (a' * x' + b') is not.

        :param other: the multiplier could only be a constant.
        :return: the result of multiplication is an affine transformation of neuron(s).
        """
        assert isinstance(other, float) or isinstance(other, int) or isinstance(other, VarAffine)
        if isinstance(other, float) or isinstance(other, int):
            varCoeffs = defaultdict(float)
            for variable in self.varCoeffs:
                varCoeffs[variable] += other * self.varCoeffs[variable]
                # varCoeffs[variable] = other * self.varCoeffs[variable]
            return VarAffine(varCoeffs, other * self.scalar)
        elif isinstance(other, VarAffine):
            raise NotImplementedError("Only linear constraints are supported.")

    def __rmul__(self, other):
        """
        Reverse multiplication of a neuron's affine transformation with a constant.
        e.g., b' * (a * x + b) is supported while (a' * x' + b') * (a * x + b) is not.
        Note: reverse multiplication between two affine transformations will use the pre-defined multiplication.

        :param other: the multiplicand could only be a constant.
        :return: the result of reverse multiplication is an affine transformation of neuron(s).
        """
        return self.__mul__(other)

    def __neg__(self):
        """
        Negation of a neuron's affine transformation.
        e.g., - (a * x + b) is - a * x - b.
        Note: negation is implemented via multiplying the affine transformation with -1.

        :return: the result of negation is an affine transformation of neuron(s).
        """
        return self.__mul__(-1)

    def __le__(self, other):
        """
        The <= operator between an affine transformation and a constant or between two affine transformations.
        e.g., a * x + b <= b' or a * x + b <= a' * x' + b'.
        Note: for cases like x + b <= b', it will set b' - b as x's upper bound;
        for cases like a * x + b <= a' * x' + b', it will set a * x + b - (a' * x' + b') <= 0 as the inequality.

        :param other: the right operand could be a constant (float, int) or an affine transformation.
        :return: an instance of the VarConstraint class.
        """
        assert isinstance(other, float) or isinstance(other, int) or isinstance(other, VarAffine)
        if list(self.varCoeffs.values()) == [1] and (isinstance(other, float) or isinstance(other, int)):
            return VarConstraint(self, isEquality=False,
                                 lowerBound=None,
                                 upperBound=other - self.scalar)
        else:
            return VarConstraint(self - other, isEquality=False,
                                 lowerBound=None,
                                 upperBound=None)

    def __ge__(self, other):
        """
        The >= operator between an affine transformation and a constant or between two affine transformations.
        e.g., a * x + b >= b' or a * x + b >= a' * x' + b'.
        Note: for cases like x + b >= b', it will set b' - b as x's lower bound;
        for cases like a * x + b >= a' * x' + b', it will set a' * x' + b' - (a * x + b) <= 0 as the inequality.

        :param other: the right operand could be a constant (float, int) or an affine transformation.
        :return: an instance of the VarConstraint class.
        """
        assert isinstance(other, float) or isinstance(other, int) or isinstance(other, VarAffine)
        if list(self.varCoeffs.values()) == [1] and (isinstance(other, float) or isinstance(other, int)):
            return VarConstraint(self, isEquality=False,
                                 lowerBound=other - self.scalar,
                                 upperBound=None)
        else:
            return VarConstraint(other - self, isEquality=False,
                                 lowerBound=None,
                                 upperBound=None)

    def __eq__(self, other):
        """
        The == operator between an affine transformation and a constant or between two affine transformations.
        e.g., a * x + b == b' or a * x + b == a' * x' + b'.
        Note: the == operator is implemented via setting a' * x' + b' - (a * x + b) == 0 as the equality.

        :param other: the right operand could be a constant (float, int) or an affine transformation.
        :return: an instance of the VarConstraint class.
        """
        return VarConstraint(self - other, isEquality=True,
                             lowerBound=None,
                             upperBound=None)


@dataclass()
class VarConstraint:
    """
    Define constraints on neurons of a neural network.
    Assuming neuron x, constants a b, and an affine transformation a * x + b,
    then example constraints are a * x + b <= or >= or == a' * x' + b'.

    Attributes:
        combination: an instance of the VarAffine class, i.e., affine transformation of neuron(s)
        isEquality: whether it is an equality or inequality, i.e., True if ==; False if >= or <=.
        lowerBound: whether it is a lower bound, e.g., True if x + b >= b'.
        upperBound: whether it is an upper bound, e.g., True if x + b <= b'.
    """
    combination: VarAffine
    isEquality: bool
    lowerBound: Optional[float]
    upperBound: Optional[float]


def Var(index):
    """
    Enable Var(i) to get the affine transformation of the i-th neuron in a neural network.

    :param index: the index of the neuron in the network
    :return: an instance of the VarAffine class, i.e., affine transformation of neuron(s)
    """
    return VarAffine(defaultdict(float, {index: 1.0}), 0)
