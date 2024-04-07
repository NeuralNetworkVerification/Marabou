/*********************                                                        */
/*! \file SoISearchStrategy.h
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

#ifndef __SoISearchStrategy_h__
#define __SoISearchStrategy_h__

enum class SoISearchStrategy {
    // Flip the SoI cost term of a random ReLU,
    // accept the change with certain probability
    MCMC,
    // Flip the SoI cost term to reduce the overall soi greedily.
    // When hitting plateau, flip a random ReLU
    WALKSAT,
};

#endif // __SoISearchStrategy_h__
