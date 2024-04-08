/*********************                                                        */
/*! \file SoIInitializationstrategy.h
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

#ifndef __SoIInitializationStrategy_h__
#define __SoIInitializationStrategy_h__

enum class SoIInitializationStrategy {
    // When initialize the SoI function, add the cost term corresponding to the
    // activation pattern of the input assignment.
    INPUT_ASSIGNMENT,
    // When initialize the SoI function, add the cost term corresponding to the
    // activation pattern of the current assignment.
    CURRENT_ASSIGNMENT,
};

#endif // __SoIInitializationStrategy_h__
