/*********************                                                        */
/*! \file Test_Preprocessor.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Derek Huang, Shantanu Thakoor
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

#include "DnCWorker.h"
#include "MockEngine.h"

class Test_DnCWorker : public CxxTest::TestSuite
{
public:

    std::unique_ptr<DnCWorker> worker;

	void setUp()
	{
        WorkerQueue *workload;
        auto engine = std::shared_ptr<Engine>( new MockEngine() );

		worker = std::unique_ptr<DnCWorker>
            ( new DnCWorker(  ) );
	}

	void tearDown()
	{
	}

};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
