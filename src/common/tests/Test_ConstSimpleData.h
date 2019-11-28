/*********************                                                        */
/*! \file Test_ConstSimpleData.h
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

#include "ConstSimpleData.h"
#include "HeapData.h"
#include "RealMalloc.h"

class ConstSimpleDataTestSuite : public CxxTest::TestSuite
{
public:
    void test_constructor()
	{
		char data[] = { 'a','b','c' };

		ConstSimpleData *constSimpleData = NULL;

		TS_ASSERT( constSimpleData = new ConstSimpleData( data, sizeof(data) ) );

		TS_ASSERT_EQUALS( constSimpleData->size(), 3U );
		TS_ASSERT_SAME_DATA( constSimpleData->data(), data, sizeof(data) );

		data[1] = 'd';

		TS_ASSERT_EQUALS( constSimpleData->size(), 3U );
		TS_ASSERT_SAME_DATA( constSimpleData->data(), data, sizeof(data) );

		TS_ASSERT_THROWS_NOTHING( delete constSimpleData );
	}

	void test_constructor__heap_data()
	{
        RealMalloc realMalloc;

		char data[] = { 'a','b','c' };

        HeapData heapData( data, sizeof(data) );

		ConstSimpleData *constSimpleData = NULL;

		TS_ASSERT( constSimpleData = new ConstSimpleData( heapData ) );

		TS_ASSERT_EQUALS( constSimpleData->size(), 3U );
		TS_ASSERT_SAME_DATA( constSimpleData->data(), data, sizeof(data) );

        TS_ASSERT_THROWS_NOTHING( delete constSimpleData );
	}

    void test_as_char()
    {
        char data[] = { 'a', 'b', 'c' };

        ConstSimpleData constSimpleData( data, sizeof(data) );

        TS_ASSERT_EQUALS( constSimpleData.asChar(), data );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
