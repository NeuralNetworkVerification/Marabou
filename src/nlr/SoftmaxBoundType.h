/*********************                                                        */
/*! \file SoftmaxBoundType.h
** \verbatim
** Top contributors (to current version):
**   Andrew Wu
** This file is part of the Marabou project.
** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**
** [[ Add lengthier description here ]]

**/

#ifndef __SoftmaxBoundType_h__
#define __SoftmaxBoundType_h__

/*
  The type of symbolic bounds to use for the Softmax function.
  Details can be found in "Convex Bounds on the Softmax Function
  with Applications to Robustness Verification" by Wei et al. AISTATS'23
*/
enum class SoftmaxBoundType {
    EXPONENTIAL_RECIPROCAL_DECOMPOSITION = 0,
    LOG_SUM_EXP_DECOMPOSITION = 1
};

#endif // __SoftmaxBoundType_h__
