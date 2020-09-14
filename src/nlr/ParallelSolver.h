/*********************                                                        */
/*! \file ParallelSolver.h
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

#ifndef __ParallelSolver_h__
#define __ParallelSolver_h__

#include "Debug.h"
#include "GurobiWrapper.h"

#include <atomic>
#include <boost/lockfree/queue.hpp>
#include <boost/chrono.hpp>
#include <mutex>

namespace NLR {

class Layer;
class LayerOwner;
class MinOrMax;

class ParallelSolver
{
public:

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

    /*
      Free all the solvers in the SolverQueue
    */
    static void clearSolverQueue( SolverQueue &freeSolvers )
    {
        // Remove the solvers
        GurobiWrapper *freeSolver;
        while ( freeSolvers.pop( freeSolver ) )
            delete freeSolver;
    }

    static void enqueueSolver( SolverQueue &solvers, GurobiWrapper *solver )
    {
        if ( !solvers.push( solver ) )
        {
            ASSERT( false );
        }
    }
};

} // namespace NLR

#endif // __ParallelSolver_h__
