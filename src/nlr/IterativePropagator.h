/*********************                                                        */
/*! \file IterativePropagator.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze (Andrew) Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#ifndef __IterativePropagator_h__
#define __IterativePropagator_h__

#include "GurobiWrapper.h"
#include "LayerOwner.h"
#include "MILPFormulator.h"
#include "ParallelSolver.h"

#include <atomic>
#include <mutex>

namespace NLR {

#define IterativePropagator_LOG(x, ...) LOG(GlobalConfiguration::PREPROCESSOR_LOGGING, "Iterativepropagator: %s\n", x)

class IterativePropagator : public ParallelSolver
{
public:
    enum MinOrMax {
        MIN = 0,
        MAX = 1,
    };

    IterativePropagator( LayerOwner *layerOwner );
    ~IterativePropagator();

    void optimizeBoundsWithIterativePropagation( const Map<unsigned, Layer *> &layers );

    /*
      When optimizing, we compute lower and upper bounds for each
      varibale. If a cutoff value is set, once one of these bounds
      crosses the cutoff value we do not attempt to optimize further
    */
    void setCutoff( double cutoff );

private:
    LayerOwner *_layerOwner;
    MILPFormulator _milpFormulator;
    bool _cutoffInUse;
    double _cutoffValue;

    /*
      Optimize for the min/max value of variableName with respect to the constraints
      encoded in gurobi. If the query is infeasible, *infeasible is set to true.
    */
    static double optimizeWithGurobi( GurobiWrapper &gurobi, MinOrMax minOrMax,
                                      String variableName, double cutoffValue,
                                      std::atomic_bool *infeasible = NULL );

    /*
      Tighten the upper- and lower- bound of a varaible
    */
    static void tightenSingleVariableBounds( ThreadArgument &argument );
};

} // namespace NLR

#endif // __MILPFormulator_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
