/*********************                                                        */
/*! \file SymbolicBoundTighteningType.h
** \verbatim
** Top contributors (to current version):
**   Haoze Andrew Wu
** This file is part of the Marabou project.
** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**
** [[ Add lengthier description here ]]

**/

#ifndef __SymbolicBoundTighteningType_h__
#define __SymbolicBoundTighteningType_h__

/*
  MILP solver bound tighening options
*/
enum class SymbolicBoundTighteningType
{
     SYMBOLIC_BOUND_TIGHTENING = 0,
     DEEP_POLY = 1,
     NONE = 2,
};

#endif // __SymbolicBoundTighteningType_h__
