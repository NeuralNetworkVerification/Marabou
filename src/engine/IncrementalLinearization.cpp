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
    SigmoidConstraint *sigmoid = (SigmoidConstraint *)constraint;
    unsigned sourceVariable = sigmoid->getB();  // x_b
    
    // get x of the found solution and calculate y of the x
    // This x is going to become a new split point
    double xpt = assignment[_milpEncoder.getVariableNameFromVariable( sourceVariable )];
    double ypt = sigmoid->sigmoid( xpt );

    INCREMENTAL_LINEARIZATION_LOG( Stringf( "new xpt: %f, new ypt: %f",
                                    xpt, ypt ).ascii() );

    // get current split points
    const TranscendentalConstraint::SplitPoints &pts = sigmoid->getSplitPoints();

    // get lower bound and upper bound
    ASSERT( pts.size() > 1 );
    auto it = pts.begin();
    double sourceLb = it->_x;
    it = pts.end();
    --it;
    double sourceUb = it->_x;

    // add a tangent line
    _milpEncoder.addTangentLineOnSigmoid( gurobi, sigmoid, xpt, ypt, sourceLb, sourceUb );

    // generate xpts and ypts for new secant lines.
    Vector<double> xpts;
    Vector<double> ypts;
    if ( FloatUtils::lt( sourceLb, 0 ) && FloatUtils::gt( sourceUb, 0 ) )
    {
        // if xpt is positive, every element in xpts should be greater than and equal to 0.
        if ( FloatUtils::gte( xpt, 0 ) )
        {
            bool ptSet = false;
            for ( const auto &pt : pts )
            {
                if ( FloatUtils::areEqual( xpt, pt._x ) )
                    // if xpt is same as one of current split points, no longer continue.
                    return false;
                else if ( FloatUtils::gte( pt._x, 0 ) )
                {
                    if ( FloatUtils::lt( pt._x, xpt ) )
                    {
                        xpts.append( pt._x );
                        ypts.append( pt._y );
                    }    
                    else if ( FloatUtils::gt( pt._x, xpt ) )
                    {
                        if ( !ptSet )
                        {
                            // insert xpt and ypt
                            xpts.append( xpt );
                            ypts.append( ypt );
                            ptSet = true;
                        }
                        xpts.append( pt._x );
                        ypts.append( pt._y );
                    }
                }
            }
        }
        // if xpt is negative, every element in xpts should be less than and equal to 0.
        else
        {
            bool ptSet = false;
            for ( const auto &pt : pts )
            {
                if ( FloatUtils::areEqual( xpt, pt._x ) )
                    // if xpt is positive, every element in xpts should be greater than and equal to 0.
                    return false;
                else if ( FloatUtils::lte( pt._x, 0 ) )
                {
                    if ( FloatUtils::lt( pt._x, xpt ) )
                    {
                        xpts.append( pt._x );
                        ypts.append( pt._y );
                    }    
                    else
                    {
                        if ( !ptSet )
                        {
                            xpts.append( xpt );
                            ypts.append( ypt );
                            ptSet = true;
                        }
                        xpts.append( pt._x );
                        ypts.append( pt._y );
                    }
                }
                else
                {
                    break;
                }
            }
        }
    }
    else
    {
        bool ptSet = false;
        for ( const auto &pt : pts )
        {
            if ( FloatUtils::areEqual( xpt, pt._x ) )
                // if xpt is positive, every element in xpts should be greater than and equal to 0.
                return false;
            else
            {
                if ( FloatUtils::lt( pt._x, xpt ) )
                {
                    xpts.append( pt._x );
                    ypts.append( pt._y );
                }    
                else if ( FloatUtils::gt( pt._x, xpt ) )
                {
                    if ( !ptSet )
                    {
                        xpts.append( xpt );
                        ypts.append( ypt );
                        ptSet = true;
                    }
                    xpts.append( pt._x );
                    ypts.append( pt._y );
                }
            }
        }
    }

    // add secant lines
    _milpEncoder.addSecantLinesOnSigmoid( gurobi, sigmoid, xpt, xpts.size(), xpts.data(), ypts.data(), sourceLb, sourceUb );

    // add split points for next linearization
    sigmoid->addSplitPoint( xpt, ypt );
    
    return true;
}