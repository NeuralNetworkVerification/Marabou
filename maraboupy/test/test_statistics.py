# Supress warnings caused by tensorflow
import warnings
warnings.filterwarnings('ignore', category = DeprecationWarning)
warnings.filterwarnings('ignore', category = PendingDeprecationWarning)

import pytest
from maraboupy import MarabouCore
from maraboupy.Marabou import createOptions

def test_statistics():
    """
    Test that a query generated from Maraboupy can be saved and loaded correctly and return sat
    """
    ipq = MarabouCore.InputQuery()
    ipq.setNumberOfVariables(1)
    ipq.setLowerBound(0, -1)
    ipq.setUpperBound(0, 1)

    opt = createOptions(verbosity = 0) # Turn off printing
    exitCode, vals, stats = MarabouCore.solve(ipq, opt, "")
    assert(stats.getUnsignedAttribute(MarabouCore.StatisticsUnsignedAttribute.NUM_SPLITS) == 0)
    assert(stats.getLongAttribute(MarabouCore.StatisticsLongAttribute.NUM_MAIN_LOOP_ITERATIONS) == 2)
    assert(stats.getDoubleAttribute(MarabouCore.StatisticsDoubleAttribute.MAX_DEGRADATION) == 0)
