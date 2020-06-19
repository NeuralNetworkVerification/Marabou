/*********************                                                        */
/*! \file MILPFormulator.h
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

#ifndef __MILPFormulator_h__
#define __MILPFormulator_h__

#include "GurobiWrapper.h"
#include "LayerOwner.h"

namespace NLR {

class MILPFormulator
{
public:
    enum MinOrMax {
        MIN = 0,
        MAX = 1,
    };

    MILPFormulator( LayerOwner *layerOwner );
    ~MILPFormulator();

    void optimizeBoundsWithMILPEncoding( const Map<unsigned, Layer *> &layers );
    void optimizeBoundsWithIncrementalMILPEncoding( const Map<unsigned, Layer *> &layers );

    /*
      When optimizing, we compute lower and upper bounds for each
      varibale. If a cutoff value is set, once one of these bounds
      crosses the cutoff value we do not attempt to optimize further
    */
    void setCutoff( double cutoff );

private:
    LayerOwner *_layerOwner;
    LPFormulator _lpFormulator;
    unsigned _signChanges;
    unsigned _tighterBoundCounter;
    unsigned _cutoffs;
    bool _cutoffInUse;
    double _cutoffValue;

    bool tightenLowerBound( GurobiWrapper &gurobi,
                            Layer *layer,
                            unsigned neuron,
                            unsigned variable,
                            double &currentLb,
                            double &newLb );

    bool tightenUpperBound( GurobiWrapper &gurobi,
                            Layer *layer,
                            unsigned neuron,
                            unsigned variable,
                            double &currentUb,
                            double &newUb );

    void createMILPEncoding( const Map<unsigned, Layer *> &layers,
                             GurobiWrapper &gurobi,
                             unsigned lastLayer = UINT_MAX );

    void addLayerToModel( GurobiWrapper &gurobi, const Layer *layer );

    void addReluLayerToMILPFormulation( GurobiWrapper &gurobi,
                                        const Layer *layer );

    void addNeuronToModel( GurobiWrapper &gurobi,
                           const Layer *layer,
                           unsigned neuron );

    double solveMILPEncoding( const Map<unsigned, Layer *> &layers,
                              MinOrMax minOrMax,
                              String variableName,
                              unsigned lastLayer = UINT_MAX );

    void storeUbIfNeeded( Layer *layer,
                          unsigned neuron,
                          unsigned variable,
                          double newUb );

    void storeLbIfNeeded( Layer *layer,
                          unsigned neuron,
                          unsigned variable,
                          double newLb );

    static void log( const String &message );
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
