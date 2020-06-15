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

class LPFormulator
{
public:
    enum MinOrMax {
        MIN = 0,
        MAX = 1,
    };

    LPFormulator( LayerOwner *layerOwner );
    ~LPFormulator();

    void optimizeBoundsWithLpRelaxation( const Map<unsigned, Layer *> &layers );

    void createLPRelaxation( const Map<unsigned, Layer *> &layers,
                             GurobiWrapper &gurobi,
                             unsigned lastLayer = UINT_MAX);

    double solveLPRelaxation( const Map<unsigned, Layer *> &layers,
                              MinOrMax minOrMax,
                              String variableName,
                              unsigned lastLayer = UINT_MAX );
private:
    LayerOwner *_layerOwner;

    void addInputLayerToLpRelaxation( GurobiWrapper &gurobi,
                                      const Layer *layer );

    void addReluLayerToLpRelaxation( GurobiWrapper &gurobi,
                                     const Layer *layer );

    void addWeightedSumLayerToLpRelaxation( GurobiWrapper &gurobi,
                                            const Layer *layer );

    static void log( const String &message );
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
