/*********************                                                        */
/*! \file LPSolverType.h
** \verbatim
** Top contributors (to current version):
**   Haoze Wu
** This file is part of the Marabou project.
** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**
** [[ Add lengthier description here ]]

**/

#ifndef __LPSolverType_h__
#define __LPSolverType_h__

enum class LPSolverType {
    // Native simplex implementation. Open-sourced, more agressive
    // bound derivation, and will support proof-production.
    NATIVE,

    // Not open-sourced. Use this for optimal run-time performance.
    GUROBI,
};

#endif // __LPSolverType_h__
