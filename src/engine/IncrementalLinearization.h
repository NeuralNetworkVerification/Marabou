/*********************                                                        */
/*! \file IncrementalLinearization.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Teruhiro Tagomori
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifndef __IncrementalLinearization_h__
#define __IncrementalLinearization_h__

#include "GurobiWrapper.h"
#include "InputQuery.h"
#include "ITableau.h"
#include "MILPEncoder.h"
#include "MStringf.h"
#include "Map.h"

#define INCREMENTAL_LINEARIZATION_LOG(x, ...) LOG(GlobalConfiguration::INCREMENTAL_LINEARIZATION_LOGGING, "IncrementalLinearization: %s\n", x)

class IncrementalLinearization
{
public:    
    IncrementalLinearization( MILPEncoder &milpEncoder);

    /*
      Solve with incremental linarizations.
      Only for the purpose of TranscendetalConstarints
    */
    IEngine::ExitCode solveWithIncrementalLinearization( GurobiWrapper &gurobi, List<TranscendentalConstraint *> tsConstraints, double timeoutInSeconds );

private:
    /*
      MILPEncoder
    */
    MILPEncoder &_milpEncoder;

    /*
      add new linear constraints
    */
    bool incrementLinearConstraint( GurobiWrapper &gurobi, TranscendentalConstraint *constraint, const Map<String, double> &assignment);
};

#endif // __IncrementalLinearization_h__
