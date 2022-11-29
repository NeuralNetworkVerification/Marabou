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
                                            const SparseMatrix *initialTableau,
                                            const double *groundUpperBounds,
                                            const double *groundLowerBounds,
                                            unsigned numberOfRows,
                                            unsigned numberOfVariables )
{
    ASSERT( var < numberOfVariables );

    double derivedBound = 0;
    double temp;

    if ( !explanation )
        return isUpper ? groundUpperBounds[var]  : groundLowerBounds[var];

    Vector<double> explanationRowCombination( numberOfVariables, 0 );
    // Create linear combination of original rows implied from explanation
    UNSATCertificateUtils::getExplanationRowCombination( var, explanation, explanationRowCombination, initialTableau, numberOfRows, numberOfVariables );

    // Set the bound derived from the linear combination, using original bounds.
    for ( unsigned i = 0; i < numberOfVariables; ++i )
    {
        temp = explanationRowCombination[i];
        if ( !FloatUtils::isZero( temp ) )
        {
            if ( isUpper )
                temp *= FloatUtils::isPositive( explanationRowCombination[i] ) ? groundUpperBounds[i] : groundLowerBounds[i];
            else
                temp *= FloatUtils::isPositive( explanationRowCombination[i] ) ? groundLowerBounds[i] : groundUpperBounds[i];

            if ( !FloatUtils::isZero( temp ) )
                derivedBound += temp;
        }
    }

    return derivedBound;
}

void UNSATCertificateUtils::getExplanationRowCombination( unsigned var,
                                                          const double *explanation,
                                                          Vector<double> &explanationRowCombination,
                                                          const SparseMatrix *initialTableau,
                                                          unsigned numberOfRows,
                                                          unsigned numberOfVariables )
{
    ASSERT( explanation != NULL );

    SparseUnsortedList tableauRow( numberOfVariables );
    explanationRowCombination = Vector<double> ( numberOfVariables, 0 );

    for ( unsigned i = 0; i < numberOfRows; ++i )
    {
         if ( FloatUtils::isZero( explanation[i] ) )
             continue;

        initialTableau->getRow( i, &tableauRow );
        for ( auto entry : tableauRow )
        {
            if ( !FloatUtils::isZero( entry._value ) )
                explanationRowCombination[entry._index] += entry._value * explanation[i];
        }
    }

    for ( unsigned i = 0; i < numberOfVariables; ++i )
    {
        if ( FloatUtils::isZero( explanationRowCombination[i] ) )
            explanationRowCombination[i] = 0;
    }

    // Since: 0 = Sum (ci * xi) + c * var = Sum (ci * xi) + (c + 1) * var - var
    // We have: var = Sum (ci * xi) + (c + 1) * var
    ++explanationRowCombination[var];
}

double UNSATCertificateUtils::computeCombinationUpperBound( const double *explanation,
                                                            const SparseMatrix *initialTableau,
                                                            const double *groundUpperBounds,
                                                            const double *groundLowerBounds,
                                                            unsigned numberOfRows,
                                                            unsigned numberOfVariables )
{
    ASSERT( explanation != NULL );

    SparseUnsortedList tableauRow( numberOfVariables );
    Vector<double> explanationRowCombination( numberOfVariables, 0 );

    for ( unsigned row = 0; row < numberOfRows; ++row )
    {
        if ( FloatUtils::isZero( explanation[row] ) )
            continue;

        initialTableau->getRow( row, &tableauRow );
        for ( auto entry : tableauRow )
        {
            if ( !FloatUtils::isZero( entry._value ) )
                explanationRowCombination[entry._index] += entry._value * explanation[row];
        }
    }

    double derivedBound = 0;
    double temp;

    // Set the bound derived from the linear combination, using original bounds.
    for ( unsigned i = 0; i < numberOfVariables; ++i )
    {
        temp = explanationRowCombination[i];
        if ( !FloatUtils::isZero( temp ) )
        {
            temp *= FloatUtils::isPositive( explanationRowCombination[i] ) ? groundUpperBounds[i] : groundLowerBounds[i];

            if ( !FloatUtils::isZero( temp ) )
                derivedBound += temp;
        }
    }

    return derivedBound;
}
