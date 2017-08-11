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

#include "FloatUtils.h"
#include "ITableau.h"
#include "MString.h"
#include "ProjectedSteepestEdge.h"
#include "ReluplexError.h"

ProjectedSteepestEdgeRule::ProjectedSteepestEdgeRule()
    : _referenceSpace( NULL )
    , _gamma( NULL )
    , _work( NULL )
{
}

ProjectedSteepestEdgeRule::~ProjectedSteepestEdgeRule()
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
    _n = tableau.getN();
    _m = tableau.getM();

    _referenceSpace = new bool[_n];
    if ( !_referenceSpace )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "ProjectedSteepestEdgeRule::referenceSpace" );

    _gamma = new double[_n - _m];
    if ( !_gamma )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "ProjectedSteepestEdgeRule::gamma" );

    _work = new double[_m];
    if ( !_work )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "ProjectedSteepestEdgeRule::work" );
}

void ProjectedSteepestEdgeRule::resetReferenceSpace( const ITableau &tableau )
{
    for ( unsigned i = 0; i < _n; ++i )
    {
        if ( tableau.isBasic( i ) )
            _referenceSpace[i] = false;
        else
        {
            _gamma[i] = 1.0;
            _referenceSpace[i] = true;
        }
    }
}

bool ProjectedSteepestEdgeRule::select( ITableau &tableau )
{
    // Obtain the list of eligible non-basic variables to consider
    List<unsigned> candidates;
    tableau.getEntryCandidates( candidates );

    if ( candidates.empty() )
        return false;

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
    double bestValue = ( costFunction[*it] * costFunction[*it] ) / _gamma[*it];
    ++it;

    while ( it != candidates.end() )
    {
        unsigned contender = *it;
        double contenderValue = ( costFunction[*it] * costFunction[*it] ) / _gamma[*it];

        if ( FloatUtils::gt( contenderValue, bestValue ) )
        {
            bestCandidate = contender;
            bestValue = contenderValue;
        }

        ++it;
    }

    tableau.setEnteringVariable( bestCandidate );

    return true;
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
