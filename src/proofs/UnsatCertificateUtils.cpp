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
                                            const SparseUnsortedList &explanation,
                                            const SparseMatrix *initialTableau,
                                            const double *groundUpperBounds,
                                            const double *groundLowerBounds,
                                            unsigned numberOfVariables )
{
    ASSERT( var < numberOfVariables );

    double derivedBound = 0;
    double temp;

    if ( explanation.empty() )
        return isUpper ? groundUpperBounds[var] : groundLowerBounds[var];

    bool allZeros = true;

    for ( const auto &entry : explanation )
        if ( !FloatUtils::isZero( entry._value ) )
        {
            allZeros = false;
            break;
        }

    if ( allZeros )
        return isUpper ? groundUpperBounds[var] : groundLowerBounds[var];

    Vector<double> explanationRowCombination( numberOfVariables, 0 );
    // Create linear combination of original rows implied from explanation
    UNSATCertificateUtils::getExplanationRowCombination(
        var, explanation, explanationRowCombination, initialTableau, numberOfVariables );

    // Set the bound derived from the linear combination, using original bounds.
    for ( unsigned i = 0; i < numberOfVariables; ++i )
    {
        temp = explanationRowCombination[i];
        if ( !FloatUtils::isZero( temp ) )
        {
            if ( isUpper )
                temp *= FloatUtils::isPositive( explanationRowCombination[i] )
                          ? groundUpperBounds[i]
                          : groundLowerBounds[i];
            else
                temp *= FloatUtils::isPositive( explanationRowCombination[i] )
                          ? groundLowerBounds[i]
                          : groundUpperBounds[i];

            if ( !FloatUtils::isZero( temp ) )
                derivedBound += temp;
        }
    }

    return derivedBound;
}

void UNSATCertificateUtils::getExplanationRowCombination( unsigned var,
                                                          const SparseUnsortedList &explanation,
                                                          Vector<double> &explanationRowCombination,
                                                          const SparseMatrix *initialTableau,
                                                          unsigned numberOfVariables )
{
    ASSERT( !explanation.empty() );

    SparseUnsortedList tableauRow( numberOfVariables );
    explanationRowCombination = Vector<double>( numberOfVariables, 0 );

    for ( const auto &entry : explanation )
    {
        if ( FloatUtils::isZero( entry._value ) )
            continue;

        initialTableau->getRow( entry._index, &tableauRow );
        for ( const auto &tableauEntry : tableauRow )
        {
            if ( !FloatUtils::isZero( tableauEntry._value ) )
                explanationRowCombination[tableauEntry._index] +=
                    entry._value * tableauEntry._value;
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

double UNSATCertificateUtils::computeCombinationUpperBound( const SparseUnsortedList &explanation,
                                                            const SparseMatrix *initialTableau,
                                                            const double *groundUpperBounds,
                                                            const double *groundLowerBounds,
                                                            unsigned numberOfVariables )
{
    ASSERT( !explanation.empty() );

    SparseUnsortedList tableauRow( numberOfVariables );
    Vector<double> explanationRowCombination( numberOfVariables, 0 );

    for ( const auto &entry : explanation )
    {
        if ( FloatUtils::isZero( entry._value ) )
            continue;

        initialTableau->getRow( entry._index, &tableauRow );
        for ( const auto &tableauEntry : tableauRow )
        {
            if ( !FloatUtils::isZero( tableauEntry._value ) )
                explanationRowCombination[tableauEntry._index] +=
                    tableauEntry._value * entry._value;
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
            temp *= FloatUtils::isPositive( explanationRowCombination[i] ) ? groundUpperBounds[i]
                                                                           : groundLowerBounds[i];

            if ( !FloatUtils::isZero( temp ) )
                derivedBound += temp;
        }
    }

    return derivedBound;
}

const Set<PiecewiseLinearFunctionType> UNSATCertificateUtils::getSupportedActivations()
{
    return { RELU, SIGN, ABSOLUTE_VALUE, MAX, DISJUNCTION, LEAKY_RELU };
}
