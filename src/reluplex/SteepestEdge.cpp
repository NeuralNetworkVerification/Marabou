/*********************                                                        */
/*! \file SteepestEdge.cpp
** \verbatim
** Top contributors (to current version):
**   Rachel Lim
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#include "SteepestEdge.h"
#include "FloatUtils.h"
#include "ITableau.h"
#include "ReluplexError.h"

bool SteepestEdge::select( ITableau &tableau )
{
    /***************************************************************
     * Chooses most eligible nonbasic variable xN[q] according 
     * to steepest edge pivot selection rules.
     *                c[j]**2
     *   q = arg max -----------
     *       j in J   gamma[j]
     *
     * where
     *         J = set of indices of eligible non-basic variables
     *      c[j] = reduced cost of nonbasic variable j
     *  gamma[j] = steepest edge weight
     *
     * Sets entering variable of the tableau to q.
     ***************************************************************/

    // Calculate entire cost function
    // TODO: integrate with Duligur's partial pricing
    tableau.computeCostFunction();
    
    List<unsigned> candidates;
    // TODO: this candidate selection for Dantzig's may not apply here. 
    tableau.getCandidates(candidates);

    if ( candidates.empty() )
	return false;

    // TODO: For each candidate, compute the gradient wrt step direction
    // and pick entering variable 

    List<unsigned>::const_iterator candidate = candidates.begin();
    unsigned maxIndex = *candidate;
    double maxValue = computeGradient(*candidate);
    ++candidate;

    while ( candidate != candidates.end() )
    {
	double contenderValue = computeGradient(*candidate);
	// TODO: use FloatUtils::gt( contenderValue, maxValue )
	if ( contenderValue > contenderValue )
	{
	    maxIndex = *candidate;
	    maxValue = contenderValue;
	}
	++candidate;
    }

    tableau.setEnteringVariable(maxIndex);
    return true;
}

double SteepestEdge::computeGradient( unsigned candidate )
{
    return 0;
}

void SteepestEdge::initialize( const ITableau & /* tableau */ )
{}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
