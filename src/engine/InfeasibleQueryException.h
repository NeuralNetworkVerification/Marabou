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

#include "List.h"

class InfeasibleQueryException
{
public:
  InfeasibleQueryException( const List<unsigned>& explanations ){
    _explanations = explanations;
  }
  List<unsigned> _explanations;
};

#endif // __InfeasibleQueryException_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
