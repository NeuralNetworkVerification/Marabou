/*********************                                                        */
/*! \file TrailEntry.h
** \verbatim
** Top contributors (to current version):
**  Aleksandar Zeljic
** This file is part of the Marabou project.
** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**
** [[ Add lengthier description here ]]
**/

#ifndef __TrailEntry_h__
#define __TrailEntry_h__

#include "PiecewiseLinearCaseSplit.h"

/*
  A trail entry consists of the pointer to a PiecewiseLinearConstraint and
  phase designation.
*/
class TrailEntry
{
public:
  PiecewiseLinearConstraint * _pwlConstraint;
  PhaseStatus _phase;

  PiecewiseLinearCaseSplit getPiecewiseLinearCaseSplit() const
  {
      return _pwlConstraint->getCaseSplit( _phase );
  }

  inline void markInfeasible()
  {
      _pwlConstraint->markInfeasible( _phase );
  }

  inline bool isFeasible() const
  {
      return _pwlConstraint->isFeasible();
  }

 TrailEntry(PiecewiseLinearConstraint * pwlc, PhaseStatus phase )
     : _pwlConstraint( pwlc )
     , _phase( phase )
    {}

  ~TrailEntry() {};
};

#endif // TrailEntry.h

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
