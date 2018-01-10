/*********************                                                        */
/*! \file ForrestTomlinFactorization.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "ForrestTomlinFactorization.h"
#include <cstdlib>

ForrestTomlinFactorization::ForrestTomlinFactorization( unsigned m )
    : _Q( m )
    , _R( m )
{
}

ForrestTomlinFactorization::~ForrestTomlinFactorization()
{
}

void ForrestTomlinFactorization::pushEtaMatrix( unsigned /* columnIndex */, double */* column */ )
{
}

void ForrestTomlinFactorization::forwardTransformation( const double */* y */, double */* x */ ) const
{
}

void ForrestTomlinFactorization::backwardTransformation( const double */* y */, double */* x */ ) const
{
}

void ForrestTomlinFactorization::storeFactorization( IBasisFactorization */* other */ )
{
}

void ForrestTomlinFactorization::restoreFactorization( const IBasisFactorization */* other */ )
{
}

void ForrestTomlinFactorization::setBasis( const double */* B */ )
{
}

bool ForrestTomlinFactorization::factorizationEnabled() const
{
    return true;
}

void ForrestTomlinFactorization::toggleFactorization( bool /* value */ )
{
}

bool ForrestTomlinFactorization::explicitBasisAvailable() const
{
    return true;
}

void ForrestTomlinFactorization::makeExplicitBasisAvailable()
{
}

const double *ForrestTomlinFactorization::getBasis() const
{
    return NULL;
}

void ForrestTomlinFactorization::invertBasis( double */* result */ )
{
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
