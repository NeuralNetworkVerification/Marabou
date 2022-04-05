/*********************                                                        */
/*! \file IncrementalLinearizatoin.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Teruhiro Tagomori
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief [[ Add one-line brief description here ]]
 **
 ** [[ Add lengthier description here ]]
 **/

#include "FloatUtils.h"
#include "GurobiWrapper.h"
#include "IncrementalLinearization.h"
#include "Options.h"
#include "TimeUtils.h"

IncrementalLinearization::IncrementalLinearization( MILPEncoder &milpEncoder )
    : _milpEncoder( milpEncoder)
{
}

IEngine::ExitCode IncrementalLinearization::solveWithIncrementalLinearization( GurobiWrapper &gurobi, List<TranscendentalConstraint *> tsConstraints, double timeoutInSeconds )
{
    unsigned incrementalCount = 0;
    const unsigned numOfIncrementalLinearizations = Options::get()->getInt( Options::NUMBER_OF_INCREMENTAL_LINEARIZATIONS );
    
    double restTimeoutInSeconds = timeoutInSeconds;

    while ( incrementalCount < numOfIncrementalLinearizations && restTimeoutInSeconds > 0 )
    {
        gurobi.setTimeLimit( restTimeoutInSeconds );

        INCREMENTAL_LINEARIZATION_LOG( Stringf( "Start incremental linearization: %u",
                                incrementalCount ).ascii() );
        
        // Extract the last solution
        Map<String, double> assignment;
        double costOrObjective;
        gurobi.extractSolution( assignment, costOrObjective );

        // Number of new split points
        unsigned numOfNewSplitPoints = 0;

        for ( const auto &tsConstraint : tsConstraints )
        {
            switch ( tsConstraint->getType() )
            {
                case TranscendentalFunctionType::SIGMOID:
                {
                    bool ret = incrementLinearConstraint( gurobi, tsConstraint, assignment );
                    if ( ret )
                        numOfNewSplitPoints++;
                    break;
                }
                default:
                {
                    throw MarabouError( MarabouError::UNSUPPORTED_TRANSCENDENTAL_CONSTRAINT,
                                        "IncrementalLinearization::solveWithIncrementalLinearization: "
                                        "Only Sigmoid is supported\n" );
                }
            }
        }

        if ( numOfNewSplitPoints > 0 )
        {
            INCREMENTAL_LINEARIZATION_LOG( Stringf( "%u split points were added.",
                                            numOfNewSplitPoints ).ascii() );
            struct timespec start = TimeUtils::sampleMicro();
            gurobi.solve();
            struct timespec end = TimeUtils::sampleMicro();
            unsigned long long passedTime = TimeUtils::timePassed( start, end );
            restTimeoutInSeconds -= passedTime / 1000000;
            
            // for debug
            // gurobi.dumpModel( Stringf("gurobi_%u.lp", incrementalCount).ascii() );
            // if ( gurobi.haveFeasibleSolution () )
            //     gurobi.dumpModel( Stringf("gurobi_%u.sol", incrementalCount).ascii() );
        }
        else
        {
            INCREMENTAL_LINEARIZATION_LOG( String( "No longer solve with linearlizations because no new constraint was added." ).ascii() );
            return IEngine::UNKNOWN;
        }

        if ( gurobi.haveFeasibleSolution() )
        {
            incrementalCount++;
            continue;
        }
        else if ( gurobi.infeasible() )
            return IEngine::UNSAT;
        else if ( gurobi.timeout() || restTimeoutInSeconds <= 0 )
            return IEngine::TIMEOUT;
        else
            throw NLRError( NLRError::UNEXPECTED_RETURN_STATUS_FROM_GUROBI );
    }
    return IEngine::UNKNOWN;
}

bool IncrementalLinearization::incrementLinearConstraint( GurobiWrapper &gurobi, TranscendentalConstraint *constraint, const Map<String, double> &assignment)
{
    SigmoidConstraint *sigmoid = ( SigmoidConstraint * )constraint;
    unsigned sourceVariable = sigmoid->getB();  // x_b
    unsigned targetVariable = sigmoid->getF();  // x_f
    
    // get x of the found solution and calculate y of the x
    // This x is going to become a new split point
    double xpt = assignment[_milpEncoder.getVariableNameFromVariable( sourceVariable )];
    double ypt = sigmoid->sigmoid( xpt );

    const double clipPoint = GlobalConfiguration::SIGMOID_CLIP_POINT_OF_LINEARIZATION;
    if ( xpt <= -clipPoint || xpt >= clipPoint )
        return false;


    double yptOfSol = assignment[_milpEncoder.getVariableNameFromVariable( targetVariable )];
    
    // If true, secant lines are added, otherwise a tangent line is added.
    bool isSolInsideOfConvex = ( xpt >= 0 && ypt > yptOfSol ) || ( xpt < 0 && ypt < yptOfSol );

    // get current secant points
    const TranscendentalConstraint::SecantPoints &secantPts = sigmoid->getSecantPoints();

    // get lower bound and upper bound
    ASSERT( secantPts.size() > 1 );
    auto it = secantPts.begin();
    double sourceLb = it->_x;
    it = secantPts.end();
    --it;
    double sourceUb = it->_x;

    if( !isSolInsideOfConvex )
    {
        // get current tangent points
        const TranscendentalConstraint::TangentPoints &tangentPts = sigmoid->getTangentPoints();

        for ( const auto &pt : tangentPts )
        {
            if ( FloatUtils::areEqual( pt._x, xpt ))
            {
                // if xpt is same as one of current tangent points, no longer continues.
                return false;            
            }
        }

        // add a tangent line
        _milpEncoder.addTangentLineOnSigmoid( gurobi, sigmoid, xpt, ypt, sourceLb, sourceUb );
        INCREMENTAL_LINEARIZATION_LOG( Stringf( "new xpt: %f, new ypt: %f for a tangent line",
                                        xpt, ypt ).ascii() );
        sigmoid->addTangentPoint( xpt, ypt );
    }
    else
    {
        // generate xpts and ypts for secant lines.
        double xpts[secantPts.size() + 1];
        double ypts[secantPts.size() + 1];
        unsigned i = 0;
        double ptSet = false;
        for ( const auto &pt : secantPts )
        {
            if ( FloatUtils::areEqual( pt._x, xpt ))
            {
                // if xpt is same as one of current secant points, no longer continues.
                return false;
            }
            else if ( FloatUtils::gt( pt._x, xpt ) && !ptSet )
            {
                xpts[i] = xpt;
                ypts[i] = ypt;
                i++;
                ptSet = true;
            }

            xpts[i] = pt._x;
            ypts[i] = pt._y;
            i++;
        }

        // add secant lines
        _milpEncoder.addSecantLinesOnSigmoid( gurobi, sigmoid, secantPts.size() + 1, xpts, ypts, sourceLb, sourceUb );
        INCREMENTAL_LINEARIZATION_LOG( Stringf( "new xpt: %f, new ypt: %f for secant lines",
                                        xpt, ypt ).ascii() );
        sigmoid->addSecantPoint( xpt, ypt );
    }

    return true;
}