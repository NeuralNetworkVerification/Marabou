/*********************                                                        */
/*! \file DantzigsRule.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "DantzigsRule.h"
#include "FloatUtils.h"
#include "ITableau.h"
#include "MStringf.h"
#include "ReluplexError.h"

bool DantzigsRule::select( ITableau &tableau,
                           const List<unsigned> &candidates,
                           const Set<unsigned> &excluded )
{
    List<unsigned> remainingCandidates = candidates;

    List<unsigned>::iterator it = remainingCandidates.begin();
    while ( it != remainingCandidates.end() )
    {
        if ( excluded.exists( *it ) )
            it = remainingCandidates.erase( it );
        else
            ++it;
    }

    if ( remainingCandidates.empty() )
        return false;

    // Dantzig's rule
    const double *costFunction = tableau.getCostFunction();

    unsigned n = tableau.getN();
    unsigned m = tableau.getM();

    String cost;
    for ( unsigned i = 0; i < n - m; ++i )
    {
        if ( FloatUtils::isZero( costFunction[i] ) )
            continue;

        if ( FloatUtils::isPositive( costFunction[i] ) )
             cost += "+";
        cost += Stringf( "%.3lf*nb[%u] ", costFunction[i], i );
    }
    log( Stringf( "Cost function: %s\n", cost.ascii() ) );

    List<unsigned>::const_iterator candidate = remainingCandidates.begin();
    unsigned maxIndex = *candidate;
    double maxValue = FloatUtils::abs( costFunction[maxIndex] );
    ++candidate;

    while ( candidate != remainingCandidates.end() )
    {
        double contenderValue = FloatUtils::abs( costFunction[*candidate] );
        if ( FloatUtils::gt( contenderValue, maxValue ) )
        {
            maxIndex = *candidate;
            maxValue = contenderValue;
        }
        ++candidate;
    }

    log( Stringf( "Largest coefficient: %.3lf. Corresponding variable: %u\n", maxValue, maxIndex ) );

    tableau.setEnteringVariableIndex( maxIndex );
    return true;
}

void DantzigsRule::log( const String &message )
{
    if ( GlobalConfiguration::DANTZIGS_RULE_LOGGING )
        printf( "DantzigsRule: %s\n", message.ascii() );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
