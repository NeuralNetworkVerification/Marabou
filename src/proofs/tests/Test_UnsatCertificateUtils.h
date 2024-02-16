/*********************                                                        */
/*! \file Test_UnsatCertificateUtils.h
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

#include "CSRMatrix.h"
#include "UnsatCertificateUtils.h"
#include "cxxtest/TestSuite.h"

class UnsatCertificateUtilsTestSuite : public CxxTest::TestSuite
{
public:
    void test_bound_computation()
    {
        unsigned m = 3, n = 6;
        double A[] = { 1, 0, -1, 1, 0, 0, 0, -1, 2, 0, 1, 0, 0.5, 0, -1, 0, 0, 1 };
        auto initialTableau = CSRMatrix( A, m, n );

        Vector<double> groundUpperBounds = { 1, 1, 1, 1, 1, 1 };
        Vector<double> groundLowerBounds = { 0, 0, 0, 0, 0, 0 };
        Vector<double> rowCombination;
        const double expl[3] = { 1, 1, 0 };
        SparseUnsortedList explanation = SparseUnsortedList();
        explanation.initialize( expl, 3 );
        // Linear combination is x0 = 2x0 - x1 + x2 + x3 + x4, thus explanation combination is only
        // lhs Checks computation method only, since no actual bound will be explained this way

        unsigned var = 0;

        UNSATCertificateUtils::getExplanationRowCombination(
            var, explanation, rowCombination, &initialTableau, n );

        auto it = rowCombination.begin();
        TS_ASSERT_EQUALS( *it, 2 );
        ++it;
        TS_ASSERT_EQUALS( *it, -1 );
        ++it;
        TS_ASSERT_EQUALS( *it, 1 );
        ++it;
        TS_ASSERT_EQUALS( *it, 1 );
        ++it;
        TS_ASSERT_EQUALS( *it, 1 );
        ++it;
        TS_ASSERT_EQUALS( *it, 0 );
        ++it;
        TS_ASSERT_EQUALS( it, rowCombination.end() );

        double explainedBound = UNSATCertificateUtils::computeBound( var,
                                                                     true,
                                                                     explanation,
                                                                     &initialTableau,
                                                                     groundUpperBounds.data(),
                                                                     groundLowerBounds.data(),
                                                                     groundUpperBounds.size() );

        TS_ASSERT_EQUALS( explainedBound, 5 );
    }
};
