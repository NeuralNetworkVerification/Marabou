'''
/* *******************                                                        */
/*! \file AcasUtils.py
 ** \verbatim
 ** Top contributors (to current version):
 **   Christopher Lazarus
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

import numpy as np
import math

# These discrete points define a grid that spans the 5D input space.
# This grid can be made more refined if you want to make the region more precise, but this will result in more points
ranges = [499, 800, 2000, 3038, 5316, 6450, 7200, 7950, 8725, 10633, 13671, 16709, 19747,
          22785, 25823, 28862, 31900, 34938, 37976, 41014, 48608, 60760]
thetas = np.linspace(-np.pi, np.pi, 41)
psis = np.linspace(-np.pi, np.pi, 41)
sos = [100, 136, 186, 254, 346, 472, 645, 880, 1200]
sis = [0, 100, 142, 203, 290, 414, 590, 841, 1200]

numStates = len(ranges) * len(thetas) * len(psis) * len(sos) * len(sis)
numActions = 5


def rescaleInput(x):
    inputRanges = [66287.1, 6.28318530718, 6.28318530718, 1100.0, 1200.0]
    inputMeans = [21770.2, 0.0, 0.0, 468.777778, 420.0]
    ret = []
    for i in range(len(x)):
        ret += [(x[i] - inputMeans[i]) / inputRanges[i]]
    return ret


def rescaleOutput(x):
    outputRange = 399.086622997
    outputMean = 8.2091785953
    return [y * outputRange + outputMean for y in x]


def getInputLowerBound(i):
    inputMins = [-0.320142, -0.5, -0.5, -0.3352525, -0.35]
    return inputMins[i]


def getInputUpperBound(i):
    inputMaxes = [0.679858, 0.5, 0.5, 0.6647475, 0.65]
    return inputMaxes[i]


def collisionSoon(r, th, psi, v_own, v_int, timeLimit=50.0, rangeLimit=4000.0):
    '''
    Function that determines if the encounter geometry will result in a collision soon if
    both aircraft fly in the current heading direction and at the same speed.

    "Collision soon" is defined by the distance between aircraft at closest point of approach (CPA)
    being under some limit and at some time under some limit. Nominally, the limits are defined as 4000 ft
    and 50.0 seconds. 

    The property to be checked: If there is a "collision soon", the network should never 
    alert COC (in other words, output_0 should never be the lower cost output).
    This applies to the tau=0, pra=1 network.

    The first five inputs describe the encounter geometry.
    The output is True if a collision will occur soon, or False otherwise.
    '''
    relx = r * np.cos(th)  # x-coordinate of intruder relative to ownship
    rely = r * np.sin(th)  # y-coordinate of intruder relative to ownship
    dx = v_int * np.cos(psi) - v_own  # x-velocity of intruder relative to ownship
    dy = v_int * np.sin(psi)  # y-velocity of intruder relative to ownship

    timeToCPA = -1
    rangeCPA = r
    if dx * dx + dy * dy > 0.0:  # Make sure relative speed is not 0

        timeToCPA = (-relx * dx - rely * dy) / (dx * dx + dy * dy)  # Time to intruder's closest point of approach
        time = np.minimum(timeLimit, timeToCPA)  # Cap the time to CPA to the timeLimit
        xT = relx + dx * time  # x-coordinate at time of closest point of approach
        yT = rely + dy * time  # y-coordinate at time of closest point of appraoch
        rangeCPA = np.sqrt(xT * xT + yT * yT)  # distance to intruder at closest point of approach

    # If timeToCPA is negative, than the aircraft are moving apart. Therefore, the minimum separation occurs now
    if timeToCPA < 0:
        rangeCPA = r

    if rangeCPA < rangeLimit:
        return True
    return False


def getCollisionList():
    ind = 0
    collisionList = np.zeros((numStates, numActions))
    for r in ranges:
        for th in thetas:
            for psi in psis:
                for so in sos:
                    for si in sis:
                        # Save state if the geometry results in a collision soon
                        if collisionSoon(r, th, psi, so, si):
                            collisionList[ind, :] = [r, th, psi, so, si]
                        ind += 1

    # Remove all of the rows of zeros from the list of points
    collisionList = np.array([collisionList[i, :] for i in range(len(collisionList)) if collisionList[i, 0] > 1.0])

    # Rescale inputs for network
    collisionList = [rescaleInput(x) for x in collisionList]
    return collisionList
