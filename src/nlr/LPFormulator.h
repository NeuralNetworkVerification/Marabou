/*********************                                                        */
/*! \file LPFormulator.h
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

#ifndef __LPFormulator_h__
#define __LPFormulator_h__

#include "GurobiWrapper.h"
#include "LayerOwner.h"
#include <climits>

namespace NLR {

#define LPFormulator_LOG(x, ...) LOG(GlobalConfiguration::PREPROCESSOR_LOGGING, "LP Preprocessor: %s\n", x)

class LPFormulator
{
public:
    enum MinOrMax {
        MIN = 0,
        MAX = 1,
    };

    LPFormulator( LayerOwner *layerOwner );
    ~LPFormulator();

    /*
      Perform bound tightening based on LP-relaxation. Use these calls
      if the LPFormulator is used in stand-alone mode. The process can
      also be performed incrementally, which means that the underlying
      LP model is adjusted from the previous call, instead of being
      constructed from scratch
    */
    void optimizeBoundsWithLpRelaxation( const Map<unsigned, Layer *> &layers );
    void optimizeBoundsWithIncrementalLpRelaxation( const Map<unsigned, Layer *> &layers );

    /*
      When optimizing, we compute lower and upper bounds for each
      varibale. If a cutoff value is set, once one of these bounds
      crosses the cutoff value we do not attempt to optimize further
    */
    void setCutoff( double cutoff );

    /*
      Calls for creating an LP relaxation instance and solving it for
      a particular variable. These calls are useful if invoked as part
      of a larger procedure, e.g. as part of a MILP-based bound
      tightening
    */
    void createLPRelaxation( const Map<unsigned, Layer *> &layers,
                             GurobiWrapper &gurobi,
                             unsigned lastLayer = UINT_MAX);

    double solveLPRelaxation( const Map<unsigned, Layer *> &layers,
                              MinOrMax minOrMax,
                              String variableName,
                              unsigned lastLayer = UINT_MAX );
    void addLayerToModel( GurobiWrapper &gurobi, const Layer *layer );

private:
    LayerOwner *_layerOwner;
    bool _cutoffInUse;
    double _cutoffValue;
    GurobiWrapper _gurobi;

    void addInputLayerToLpRelaxation( GurobiWrapper &gurobi,
                                      const Layer *layer );

    void addReluLayerToLpRelaxation( GurobiWrapper &gurobi,
                                     const Layer *layer );

    void addSignLayerToLpRelaxation( GurobiWrapper &gurobi,
                                     const Layer *layer );

    void addWeightedSumLayerToLpRelaxation( GurobiWrapper &gurobi,
                                            const Layer *layer );
};

} // namespace NLR

#endif // __LPFormulator_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
