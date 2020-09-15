/*********************                                                        */
/*! \file NetworkLevelReasoner.cpp
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

#include "InfeasibleQueryException.h"
#include "ParallelSolver.h"
#include "Layer.h"
#include "MStringf.h"
#include "NLRError.h"
#include "Options.h"
#include "TimeUtils.h"

#include <boost/thread.hpp>

namespace NLR {

void ParallelSolver::clearSolverQueue( SolverQueue &freeSolvers )
{
    // Remove the solvers
    GurobiWrapper *freeSolver;
    while ( freeSolvers.pop( freeSolver ) )
        delete freeSolver;
}

void ParallelSolver::enqueueSolver( SolverQueue &solvers, GurobiWrapper *solver )
{
    if ( !solvers.push( solver ) )
    {
        ASSERT( false );
    }
}

double ParallelSolver::optimizeWithGurobi( GurobiWrapper &gurobi, MinOrMax
                                           minOrMax, String variableName,
                                           double cutoffValue,
                                           std::atomic_bool *infeasible )
{
    List<GurobiWrapper::Term> terms;
    terms.append( GurobiWrapper::Term( 1, variableName ) );

    if ( minOrMax == MAX )
        gurobi.setObjective( terms );
    else
        gurobi.setCost( terms );

    gurobi.solve();

    if ( gurobi.infeasbile() )
    {
        if ( infeasible )
        {
            *infeasible = true;
            return FloatUtils::infinity();
        }
        else
            throw InfeasibleQueryException();
    }

    if ( gurobi.cutoffOccurred() )
        return cutoffValue;

    if ( gurobi.optimal() )
    {
        Map<String, double> dontCare;
        double result = 0;
        gurobi.extractSolution( dontCare, result );
        return result;
    }
    else if ( gurobi.timeout() )
    {
        return gurobi.getObjectiveBound();
    }

    throw NLRError( NLRError::UNEXPECTED_RETURN_STATUS_FROM_GUROBI );
}

void ParallelSolver::tightenSingleVariableBounds( ThreadArgument &argument )
{
    try
    {
        GurobiWrapper *gurobi = argument._gurobi;
        Layer *layer = argument._layer;
        unsigned index = argument._index;
        double currentLb = argument._currentLb;
        double currentUb = argument._currentUb;
        bool cutoffInUse = argument._cutoffInUse;
        double cutoffValue = argument._cutoffValue;
        LayerOwner *layerOwner = argument._layerOwner;
        SolverQueue &freeSolvers = argument._freeSolvers;
        std::mutex &mtx = argument._mtx;
        std::atomic_bool &infeasible = argument._infeasible;
        std::atomic_uint &tighterBoundCounter = argument._tighterBoundCounter;
        std::atomic_uint &signChanges = argument._signChanges;
        std::atomic_uint &cutoffs = argument._cutoffs;

        ParallelSolver_LOG( Stringf( "Tightening bounds for layer %u index %u",
                                   layer->getLayerIndex(), index ).ascii() );

        unsigned variable = layer->neuronToVariable( index );
        Stringf variableName( "x%u", variable );

        ParallelSolver_LOG( Stringf( "Computing upperbound..." ).ascii() );
        double ub = optimizeWithGurobi( *gurobi, MinOrMax::MAX, variableName,
                                        cutoffValue, &infeasible );
        ParallelSolver_LOG( Stringf( "Upperbound computed %f", ub ).ascii() );

        // Store the new bound if it is tighter
        if ( ub < currentUb )
        {
            if ( FloatUtils::isPositive( currentUb ) &&
                 !FloatUtils::isPositive( ub ) )
                ++signChanges;

            mtx.lock();
            layer->setUb( index, ub );
            layerOwner->receiveTighterBound( Tightening( variable,
                                                         ub,
                                                         Tightening::UB ) );
            mtx.unlock();

            ++tighterBoundCounter;

            if ( cutoffInUse && ub < cutoffValue )
            {
                ++cutoffs;
                enqueueSolver( freeSolvers, gurobi );
                return;
            }
        }

        ParallelSolver_LOG( Stringf( "Computing lowerbound..." ).ascii() );
        gurobi->reset();
        double lb = optimizeWithGurobi( *gurobi, MinOrMax::MIN, variableName,
                                        cutoffValue, &infeasible );
        ParallelSolver_LOG( Stringf( "Lowerbound computed: %f", lb ).ascii() );

        // Store the new bound if it is tighter
        if ( lb > currentLb )
        {
            if ( FloatUtils::isNegative( currentLb ) &&
                 !FloatUtils::isNegative( lb ) )
                ++signChanges;

            mtx.lock();
            layer->setLb( index, lb );
            layerOwner->receiveTighterBound( Tightening( variable,
                                                         lb,
                                                         Tightening::LB ) );
            mtx.unlock();
            ++tighterBoundCounter;

            if ( cutoffInUse && lb > cutoffValue )
            {
                ++cutoffs;
            }
        }
        enqueueSolver( freeSolvers, gurobi );
    }
    catch ( boost::thread_interrupted& )
    {
        enqueueSolver( argument._freeSolvers, argument._gurobi );
    }
}

} // namespace NLR
