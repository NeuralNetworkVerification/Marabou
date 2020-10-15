/*********************                                                        */
/*! \file MILPSolverBoundTighteningType.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**
** [[ Add lengthier description here ]]

**/

#ifndef __MILPSolverBoundTighteningType_h__
#define __MILPSolverBoundTighteningType_h__

/*
  MILP solver bound tighening options
*/
enum MILPSolverBoundTighteningType
{
     // Only encode pure linear constraints in the underlying
     // solver, in a way that over-approximates the query
     LP_RELAXATION = 0,
     LP_RELAXATION_INCREMENTAL = 1,
     // Encode linear and integer constraints in the underlying
     // solver, in a way that completely captures the query but is
     // more expensive to solve
     MILP_ENCODING = 2,
     MILP_ENCODING_INCREMENTAL = 3,
     // Option to have no MILP bound tightening performed
     NONE = 4,
};

#endif // __MILPSolverBoundTighteningType_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
