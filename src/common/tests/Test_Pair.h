/*********************                                                        */
/*! \file Test_Pair.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief [[ Add one-line brief description here ]]
 **
 ** [[ Add lengthier description here ]]
 **/

#include <cxxtest/TestSuite.h>

#include "MString.h"
#include "Pair.h"

class PairTestSuite : public CxxTest::TestSuite
{
public:

    void test_pair()
    {
        Pair<int, String> p1;
        Pair<int, String> p2( 5, "dog" );

        TS_ASSERT( p1 != p2 );
        TS_ASSERT( !( p1 == p2 ) );

        p1.first() = 5;

        TS_ASSERT( p1 != p2 );
        TS_ASSERT( !( p1 == p2 ) );

        p1.second() = "dog";

        TS_ASSERT( p1 == p2 );
        TS_ASSERT( !( p1 != p2 ) );

        p2.second() = "bird";

        TS_ASSERT( p1 != p2 );
        TS_ASSERT( !( p1 == p2 ) );

        p1 = p2;

        TS_ASSERT( p1 == p2 );
        TS_ASSERT( !( p1 != p2 ) );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
