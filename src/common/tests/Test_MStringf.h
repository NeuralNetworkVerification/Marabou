/*********************                                                        */
/*! \file Test_MStringf.h
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

#include "MStringf.h"

class StringfTestSuite : public CxxTest::TestSuite
{
public:
	void test_constructor()
	{
        Stringf stringf( "Hello %s %u world", "bla", 5 );
        String expected( "Hello bla 5 world" );

        TS_ASSERT_EQUALS( stringf, expected );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
