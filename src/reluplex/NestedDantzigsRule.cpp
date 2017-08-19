/*********************                                                        */
/*! \file NestedDantzigsRule.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Duligur Ibeling
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

/*
 * Based on "Efficient nested pricing in the simplex method," PQ Pan
 */

#include "FloatUtils.h"
#include "ITableau.h"
#include "NestedDantzigsRule.h"
#include "ReluplexError.h"

bool NestedDantzigsRule::select( ITableau &tableau, const Set<unsigned> &/* excluded */ )
{
    const double *costFunction = tableau.getCostFunction();

    Set<unsigned> Jhat;
    for ( const auto &i : _J )
    {
        if ( tableau.eligibleForEntry( i ) )
            Jhat.insert( i );
    }

    if ( Jhat.empty() )
    {
        unsigned numNonBasic = tableau.getN() - tableau.getM();
        for ( unsigned i = 0; i < numNonBasic; ++i )
        {
            if ( !_J.exists( i ) )
            {
                if ( tableau.eligibleForEntry( i ) )
                    Jhat.insert( i );
            }
        }

        if ( Jhat.empty() )
            return false;
    }

    Set<unsigned>::const_iterator candidate = Jhat.begin();
    Set<unsigned>::const_iterator maxIt = Jhat.begin();
    double maxValue = FloatUtils::abs( costFunction[*maxIt] );
    ++candidate;
    while ( candidate != Jhat.end() )
    {
        double contenderValue = FloatUtils::abs( costFunction[*candidate] );
        if ( FloatUtils::gt( contenderValue, maxValue ) )
        {
    	    maxIt = candidate;
            maxValue = contenderValue;
        }
        ++candidate;
    }

    tableau.setEnteringVariable( *maxIt );
    _J = Jhat;
    _J.erase( *maxIt );
    return true;
}

void NestedDantzigsRule::initialize( const ITableau &tableau )
{
    _J.clear();
    unsigned numNonBasic = tableau.getN() - tableau.getM();
    for ( unsigned i = 0; i < numNonBasic; ++i )
        _J.insert( i );
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
