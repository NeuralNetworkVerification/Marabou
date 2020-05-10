#!/usr/bin/env python3.7
import pygg
import os
import functools
import subprocess as sub
import tempfile
import itertools as it
from typing import List, Optional

import sys
sys.setrecursionlimit(5000)

gg = pygg.init()

# Must be on path
marabou = "Marabou"
gg.install(marabou)


out_prefix = "subproperty"


def split_outputs(_net: pygg.Value, _prop: pygg.Value, n: int) -> List[str]:
    return [f"{out_prefix}{i}" for i in range(2 ** n)]


@gg.thunk_fn(outputs=split_outputs)
def split(net: pygg.Value, prop: pygg.Value, n: int) -> pygg.OutputDict:
    sub.check_call(
        [gg.bin(marabou).path(),
         "--split-only",
         "--initial-divides",
         str(n),
         "--subproperty-prefix",
         out_prefix,
         net.path(),
         prop.path(),
         "--verbosity",
         "0",
         ]
    )
    return {
      path: gg.file_value(path) \
      for path in split_outputs(net, prop, n)
    }

@gg.thunk_fn()
def solve(
    net: pygg.Value,
    prop: pygg.Value,
    initial_divides: int,
    n: int,
    timeout: float,
    timeout_factor: float,
    fut : int
) -> pygg.Output:
    output = "TIMEOUT"
    if initial_divides == 0:
        args = [
            gg.bin(marabou).path(),
            net.path(),
            prop.path(),
            "--timeout",
            str(int(timeout)),
            "--summary-file",
            "out",
            "--verbosity",
            "0",
        ]
        sub.check_call(args)
        with open("out", "r") as f:
            output = f.read()
        os.remove("out")
    if "unsat" in output:
        return gg.str_value("unsat\n")
    if "sat" in output:
        return gg.str_value("sat\n")
    if "TIMEOUT" in output:
        divides = n if initial_divides == 0 else initial_divides
        split_thunk = gg.thunk(split, net, prop, divides)
        split_os = split_outputs(net, prop, divides)
        sub_solve_thunks = []
        for o in split_os:
            sub_solve_thunks.append(
                gg.thunk(
                    solve,
                    net,
                    split_thunk[o],
                    0,
                    n,
                    timeout * timeout_factor,
                    timeout_factor,
                    fut
                )
            )
        merger = merge if fut != 0 else merge_no_fut
        return functools.reduce(lambda x, y: gg.thunk(merger, x, y), sub_solve_thunks)
    else:
        raise ValueError(f"Unknown output: {output}")

@gg.thunk_fn()
def merge(r1: Optional[pygg.Value], r2: Optional[pygg.Value]) -> pygg.Output:
    if r2 is not None and "sat" in r2.as_str():
        # r2 is SAT, return SAT
        return r2
    elif r1 is not None and "sat" in r1.as_str():
        # see aboce
        return r1
    elif r1 is None or r2 is None:
        # Something is unresolved, and not SATs yet...
        return gg.this()
    else:
        # All is resolved, no SATs
        return r1

@gg.thunk_fn()
def merge_no_fut(r1: pygg.Value, r2: pygg.Value) -> pygg.Output:
    r1_str = r1.as_str()
    r2_str = r2.as_str()
    if "unsat" in r1_str and "unsat" in r2_str:
        return r1
    else:
        return gg.str_value("sat\n")

gg.main()
