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

bool SteepestEdgeRule::select( ITableau &tableau )
{
    /***************************************************************
     * Chooses most eligible nonbasic variable xN[q] according 
     * to steepest edge pivot selection rules.
     *
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
    // TODO: integrate with Duligur's partial pricing?
    tableau.computeCostFunction();
    
    List<unsigned> candidates;
    tableau.getCandidates(candidates);

    if ( candidates.empty() )
	return false;

    const double *costFunction = tableau.getCostFunction();
    const double *gamma = tableau.getSteepestEdgeGamma();

    List<unsigned>::const_iterator candidate = candidates.begin();
    unsigned maxIndex = *candidate;
    double maxValue = computeGradient( *candidate, costFunction, gamma );
    ++candidate;

    while ( candidate != candidates.end() )
    {
	double contenderValue = computeGradient( *candidate, costFunction, gamma );
	// TODO: use FloatUtils::gt( contenderValue, maxValue )
	if ( contenderValue > maxValue )
	{
	    maxIndex = *candidate;
	    maxValue = contenderValue;
	}
	++candidate;
    }

    tableau.setEnteringVariable(maxIndex);
    return true;
}

double SteepestEdgeRule::computeGradient( const unsigned j, const double *c, const double *gamma )
{
    /* Computes the (square of the) gradient in the step direction of the
     * j-th nonbasic var.
     *
     * Step direction for candidate j,
     *    p[j] = [ -inv(B)*AN*e[j]; e[j] ]
     *    (where e[j] is the j-th standard unit vector)
     *
     * Let gamma[j] = || p[j] || ** 2
     *                                      c'*p[j]     c[j]
     * Gradient of cost function wrt p[j] = -------- = --------
     *                                      ||p[j]||   ||p[j]||
     *               
     * This function returns c[j]**2 / gamma[j]
     *
     * Note: gamma[j] is costly to compute from scratch, but after each pivot operation, we 
     * can update gamma more cheaply using a recurrence relation. See Goldfarb and Reid (1977),
     * Forrest and Goldfarb (1992).
     */
    return (c[j] * c[j]) / gamma[j];
}

void SteepestEdgeRule::initialize( const ITableau & /* tableau */ )
{}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
