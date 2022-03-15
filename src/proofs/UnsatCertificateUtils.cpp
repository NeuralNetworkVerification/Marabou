/*********************                                                        */
/*! \file UnsatCertificateUtils.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Omri Isac, Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2022 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]
 **/

#include "UnsatCertificateUtils.h"

double UNSATCertificateUtils::computeBound( unsigned var,
                                            bool isUpper,
                                            const double *explanation,
                                            const Vector<Vector<double>> &initialTableau,
                                            const Vector<double> &groundUpperBounds,
                                            const Vector<double> &groundLowerBounds )
{
    ASSERT( groundLowerBounds.size() == groundUpperBounds.size() );
    ASSERT( groundLowerBounds.size() == initialTableau[0].size() );
    ASSERT( groundLowerBounds.size() == initialTableau[initialTableau.size() - 1 ].size() );
    ASSERT( var < groundUpperBounds.size() );

    double derivedBound = 0;
    double temp;
    unsigned n = groundUpperBounds.size();

    if ( !explanation )
        return isUpper ? groundUpperBounds[var]  : groundLowerBounds[var];

    // Create linear combination of original rows implied from explanation
    Vector<double> explanationRowsCombination;
    UNSATCertificateUtils::getExplanationRowCombination( var, explanationRowsCombination, explanation, initialTableau );

    // Set the bound derived from the linear combination, using original bounds.
    for ( unsigned i = 0; i < n; ++i )
    {
        temp = explanationRowsCombination[i];
        if ( !FloatUtils::isZero( temp ) )
        {
            if ( isUpper )
                temp *= FloatUtils::isPositive( explanationRowsCombination[i] ) ? groundUpperBounds[i] : groundLowerBounds[i];
            else
                temp *= FloatUtils::isPositive( explanationRowsCombination[i] ) ? groundLowerBounds[i] : groundUpperBounds[i];

            if ( !FloatUtils::isZero( temp ) )
                derivedBound += temp;
        }
    }

    return derivedBound;
}

void UNSATCertificateUtils::getExplanationRowCombination( unsigned var,
                                                          Vector<double> &explanationRowCombination,
                                                          const double *explanation,
                                                          const Vector<Vector<double>> &initialTableau )
{
    explanationRowCombination = Vector<double>( initialTableau[0].size(), 0 );
    unsigned n = initialTableau[0].size();
    unsigned m = initialTableau.size();
    for ( unsigned i = 0; i < m; ++i )
    {
        for ( unsigned j = 0; j < n; ++j )
        {
            if ( !FloatUtils::isZero( initialTableau[i][j] ) && !FloatUtils::isZero( explanation[i] ) )
                explanationRowCombination[j] += initialTableau[i][j] * explanation[i];
        }
    }

    for ( unsigned i = 0; i < n; ++i )
    {
        if ( FloatUtils::isZero( explanationRowCombination[i] ) )
            explanationRowCombination[i] = 0;
    }

    // Since: 0 = Sum (ci * xi) + c * var = Sum (ci * xi) + (c + 1) * var - var
    // We have: var = Sum (ci * xi) + (c + 1) * var
    ++explanationRowCombination[var];
}
