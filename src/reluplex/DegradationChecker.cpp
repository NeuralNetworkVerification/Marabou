/*********************                                                        */
/*! \file DegradationChecker.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "DegradationChecker.h"
#include "FloatUtils.h"
#include "InputQuery.h"

void DegradationChecker::storeEquations( const InputQuery &query )
{
    _equations = query.getEquations();
}

double DegradationChecker::computeDegradation( ITableau &tableau ) const
{
    double degradation = 0.0;

    for ( const auto &equation : _equations )
        degradation += computeDegradation( equation, tableau );

    return degradation;
}

double DegradationChecker::computeDegradation( const Equation &equation, ITableau &tableau ) const
{
    double sum = 0.0;
    for ( const auto &addend : equation._addends )
        sum += addend._coefficient * tableau.getValue( addend._variable );

    return FloatUtils::abs( sum - equation._scalar );
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
