/*********************                                                        */
/*! \file ProjectedSteepestEdge.cpp
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#include "Debug.h"
#include "FloatUtils.h"
#include "ITableau.h"
#include "MString.h"
#include "ProjectedSteepestEdge.h"
#include "ReluplexError.h"
#include "Statistics.h"

ProjectedSteepestEdgeRule::ProjectedSteepestEdgeRule()
    : _referenceSpace( NULL )
    , _gamma( NULL )
    , _work( NULL )
    , _iterationsUntilReset( GlobalConfiguration::PSE_ITERATIONS_BEFORE_RESET )
    , _errorInGamma( 0.0 )
{
}

ProjectedSteepestEdgeRule::~ProjectedSteepestEdgeRule()
{
    freeIfNeeded();
}

void ProjectedSteepestEdgeRule::freeIfNeeded()
{
    if ( _referenceSpace )
    {
        delete[] _referenceSpace;
        _referenceSpace = NULL;
    }

    if ( _gamma )
    {
        delete[] _gamma;
        _gamma = NULL;
    }

    if ( _work )
    {
        delete[] _work;
        _work = NULL;
    }
}

void ProjectedSteepestEdgeRule::initialize( const ITableau &tableau )
{
    freeIfNeeded();

    _n = tableau.getN();
    _m = tableau.getM();

    _referenceSpace = new char[_n];
    if ( !_referenceSpace )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "ProjectedSteepestEdgeRule::referenceSpace" );

    _gamma = new double[_n - _m];
    if ( !_gamma )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "ProjectedSteepestEdgeRule::gamma" );

    _work = new double[_m];
    if ( !_work )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "ProjectedSteepestEdgeRule::work" );

    resetReferenceSpace( tableau );
}

void ProjectedSteepestEdgeRule::resetReferenceSpace( const ITableau &tableau )
{
    memset( _referenceSpace, 0, _n * sizeof(char) );

    for ( unsigned i = 0; i < _n - _m; ++i )
    {
        _gamma[i] = 1.0;
        _referenceSpace[tableau.nonBasicIndexToVariable( i )] = 1;
    }

    _iterationsUntilReset = GlobalConfiguration::PSE_ITERATIONS_BEFORE_RESET;

    if ( _statistics )
        _statistics->pseIncNumResetReferenceSpace();
}

bool ProjectedSteepestEdgeRule::select( ITableau &tableau )
{
    tableau.computeCostFunction();

    // Obtain the list of eligible non-basic variables to consider
    List<unsigned> candidates;
    tableau.getEntryCandidates( candidates );

    if ( candidates.empty() )
    {
        log( "No candidates, select returning false" );
        return false;
    }

    // Obtain the cost function
    const double *costFunction = tableau.getCostFunction();

    /*
      Apply the steepest edge rule: iterate over the candidates and
      pick xi for which the value of

                  costFunction[i]^2
                  -----------------
                       gamma[i]

      is maximal.
    */

    auto it = candidates.begin();
    unsigned bestCandidate = *it;
    double gammaValue = _gamma[*it];
    double bestValue =
        FloatUtils::isZero( gammaValue ) ? 0 : ( costFunction[*it] * costFunction[*it] ) / _gamma[*it];
    ++it;

    while ( it != candidates.end() )
    {
        unsigned contender = *it;
        gammaValue = _gamma[*it];
        double contenderValue =
            FloatUtils::isZero( gammaValue ) ? 0 : ( costFunction[*it] * costFunction[*it] ) / _gamma[*it];

        if ( FloatUtils::gt( contenderValue, bestValue ) )
        {
            bestCandidate = contender;
            bestValue = contenderValue;
        }

        ++it;
    }

    tableau.setEnteringVariable( bestCandidate );

    if ( _statistics )
        _statistics->pseIncNumIterations();

    return true;
}

void ProjectedSteepestEdgeRule::prePivotHook( const ITableau &tableau, bool fakePivot )
{
    log( "PrePivotHook called" );
    // If the pivot is fake, gamma does not need to be updated
    if ( fakePivot )
    {
        log( "PrePivotHook done - fake pivot" );
        return;
    }

    // When this hook is called, the entering and leaving variables have
    // already been determined. This are the actual varaibles, not the indices.
    unsigned entering = tableau.getEnteringVariable();
    unsigned enteringIndex = tableau.variableToIndex( entering );
    unsigned leaving = tableau.getLeavingVariable();
    unsigned leavingIndex = tableau.variableToIndex( leaving );

    const double *changeColumn = tableau.getChangeColumn();
    const double *pivotRow = tableau.getPivotRow();

    // Update gamma[entering] to the accurate value, taking the pivot into account
    double accurateGamma;
    _errorInGamma = computeAccurateGamma( accurateGamma, tableau );
    _gamma[enteringIndex] = accurateGamma / ( changeColumn[leavingIndex] * changeColumn[leavingIndex] );

    unsigned m = tableau.getM();
    unsigned n = tableau.getN();

    // TODO: make sure that there's no hidden minus in glpk's row computation

    // Auxiliary variables
    double r, s, t1, t2;
    const double *AColumn;

    // Compute GLPK's u vector
    // for ( unsigned i = 0; i < m; ++i )
    // {
    //     unsigned basicVariable = tableau.basicIndexToVariable( i );
    //     if ( _referenceSpace[basicVariable] )
    //         _work[i] = changeColumn[i];
    //     else
    //         _work[i] = 0.0;
    // }

    tableau.backwardTransformation( tableau.getRightHandSide(), _work );

    // Update gamma[i] for all i != enteringIndex
    for ( unsigned i = 0; i < n - m; ++i )
    {
        if ( i == enteringIndex )
            continue;

        if ( FloatUtils::isZero( pivotRow[i] ) )
            continue;

        r = pivotRow[i] / changeColumn[leavingIndex];

        /* compute inner product s[j] = N'[j] * u, where N[j] = A[k]
         * is constraint matrix column corresponding to xN[j] */
        unsigned nonBasic = tableau.nonBasicIndexToVariable( i );
        AColumn = tableau.getAColumn( nonBasic );
        s = 0.0;
        for ( unsigned j = 0; j < m; ++j )
            s += AColumn[j] * _work[j];

        /* compute new gamma[j] */
        t1 = _gamma[i] + r * ( r * accurateGamma + s + s );
        t2 = ( ( _referenceSpace[nonBasic] ? 1.0 : 0.0 ) +
               ( _referenceSpace[entering] ? 1.0 : 0.0 ) * r * r );
        _gamma[i] = ( t1 >= t2 ? t1 : t2 );
    }

    log( "PrePivotHook done" );
}

double ProjectedSteepestEdgeRule::computeAccurateGamma( double &accurateGamma, const ITableau &tableau )
{
    unsigned entering = tableau.getEnteringVariable();
    unsigned m = tableau.getM();
    const double *changeColumn = tableau.getChangeColumn();

    // Is the entering variable in the reference space?
    accurateGamma = _referenceSpace[entering] ? 1.0 : 0.0;

    for ( unsigned i = 0; i < m; ++i )
    {
        unsigned basic = tableau.basicIndexToVariable( i );
        if ( _referenceSpace[basic] )
            accurateGamma += ( changeColumn[i] * changeColumn[i] );
    }

    return FloatUtils::abs( ( accurateGamma - _gamma[entering] ) / ( 1.0 + accurateGamma ) );
}

void ProjectedSteepestEdgeRule::postPivotHook( const ITableau &tableau, bool fakePivot )
{
    log( "PostPivotHook called" );

    // If the pivot is fake, no need to reset the reference space.
    if ( fakePivot )
    {
        log( "PostPivotHook done - fake pivot" );
        return;
    }

    // If the iteration limit has been exhausted, reset the reference space
    --_iterationsUntilReset;
    if ( _iterationsUntilReset == 0 )
    {
        log( "PostPivotHook reseting ref space (iterations)" );
        resetReferenceSpace( tableau );
        return;
    }

    // If the error is too great, reset the reference space.
    if ( FloatUtils::gt( _errorInGamma, GlobalConfiguration::PSE_GAMMA_ERROR_THRESHOLD ) )
    {
        log( "PostPivotHook reseting ref space (degradation)" );
        resetReferenceSpace( tableau );
        return;
    }

    log( "PostPivotHook done (ref space not reset)" );
}

void ProjectedSteepestEdgeRule::resizeHook( const ITableau &tableau )
{
    initialize( tableau );
}

void ProjectedSteepestEdgeRule::log( const String &message )
{
    if ( GlobalConfiguration::PROJECTED_STEEPEST_EDGE_LOGGING )
        printf( "Projected SE: %s\n", message.ascii() );
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
