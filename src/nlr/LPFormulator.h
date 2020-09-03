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

#include <atomic>
#include <boost/lockfree/queue.hpp>
#include <boost/chrono.hpp>
#include <mutex>

namespace NLR {

#define LPFormulator_LOG(x, ...) LOG(GlobalConfiguration::PREPROCESSOR_LOGGING, "LP Preprocessor: %s\n", x)

class LPFormulator
{
public:
    enum MinOrMax {
        MIN = 0,
        MAX = 1,
    };

    typedef boost::lockfree::queue
    <GurobiWrapper *, boost::lockfree::fixed_sized<true>> SolverQueue;

    /*
      Arguments for the spawned thread. This is needed because Boost::thread does
      not seem to support functions with more than 7 arguments.
    */
    struct ThreadArgument{

        ThreadArgument( GurobiWrapper *gurobi, Layer *layer,
                        unsigned index, double currentLb, double currentUb,
                        bool cutoffInUse, double cutoffValue,
                        LayerOwner *layerOwner, SolverQueue &freeSolvers,
                        std::mutex &mtx, std::atomic_bool &infeasible,
                        std::atomic_uint &tighterBoundCounter,
                        std::atomic_uint &signChanges,
                        std::atomic_uint &cutoffs )
        : _gurobi( gurobi )
        , _layer( layer )
        , _index( index )
        , _currentLb(currentLb )
        , _currentUb(currentUb )
        , _cutoffInUse( cutoffInUse )
        , _cutoffValue( cutoffValue )
        , _layerOwner( layerOwner )
        , _freeSolvers( freeSolvers )
        , _mtx( mtx )
        , _infeasible( infeasible )
        , _tighterBoundCounter( tighterBoundCounter )
        , _signChanges( signChanges )
        , _cutoffs( cutoffs )
        {
        }

        GurobiWrapper *_gurobi;
        Layer *_layer;
        unsigned _index;
        double _currentLb;
        double _currentUb;
        bool _cutoffInUse;
        double _cutoffValue;
        LayerOwner *_layerOwner;
        SolverQueue &_freeSolvers;
        std::mutex &_mtx;
        std::atomic_bool &_infeasible;
        std::atomic_uint &_tighterBoundCounter;
        std::atomic_uint &_signChanges;
        std::atomic_uint &_cutoffs;
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
                             unsigned lastLayer = UINT_MAX );

    double solveLPRelaxation( GurobiWrapper &gurobi,
                              const Map<unsigned, Layer *> &layers,
                              MinOrMax minOrMax, String variableName,
                              unsigned lastLayer = UINT_MAX );

    void addLayerToModel( GurobiWrapper &gurobi, const Layer *layer );

private:

    LayerOwner *_layerOwner;
    bool _cutoffInUse;
    double _cutoffValue;

    void addInputLayerToLpRelaxation( GurobiWrapper &gurobi,
                                      const Layer *layer );

    void addReluLayerToLpRelaxation( GurobiWrapper &gurobi,
                                     const Layer *layer );

    void addSignLayerToLpRelaxation( GurobiWrapper &gurobi,
                                     const Layer *layer );

    void addWeightedSumLayerToLpRelaxation( GurobiWrapper &gurobi,
                                            const Layer *layer );

    /*
      Optimize for the min/max value of variableName with respect to the constraints
      encoded in gurobi. If the query is infeasible, *infeasible is set to true.
    */
    static double optimizeWithGurobi( GurobiWrapper &gurobi, MinOrMax minOrMax,
                                      String variableName, double cutoffValue,
                                      std::atomic_bool *infeasible = NULL );

    /*
      Tighten the upper- and lower- bound of a varaible with LPRelaxation
    */
    static void tightenSingleVariableBoundsWithLPRelaxation( ThreadArgument &argument );

    /*
      Free all the solvers in the SolverQueue
    */
    static void clearSolverQueue( SolverQueue &freeSolvers );

    static void enqueue( SolverQueue &solvers, GurobiWrapper *solver );
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
