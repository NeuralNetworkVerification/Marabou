/*********************                                                        */
/*! \file InfeasibleQueryException.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __InfeasibleQueryException_h__
#define __InfeasibleQueryException_h__

class InfeasibleQueryException
{
public:
  // if non-negative, variable whose lower bound is greater than upper bound
  // if negative, query is infeasible but not clear which particular variable to blame
  InfeasibleQueryException( int var): _var( var ) {}
  int _var;
};

#endif // __InfeasibleQueryException_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
