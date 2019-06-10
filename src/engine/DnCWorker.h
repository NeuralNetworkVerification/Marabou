/*********************                                                        */
/*! \file DnCWorker.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifndef __DnCWorker_h__
#define __DnCWorker_h__

#include "DivideStrategy.h"
#include "Engine.h"
#include "EngineState.h"
#include "PiecewiseLinearCaseSplit.h"
#include "QueryDivider.h"
#include "Statistics.h"

#include <atomic>
#include <vector>

class QueryDivider;


class DnCWorker
{
public:
  DnCWorker();

  void run();
};

#endif // __DnCWorker_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
