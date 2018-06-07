/*********************                                                        */
/*! \file Test_LUFactors.h
** \verbatim
** Top contributors (to current version):
**   Derek Huang
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#include <cxxtest/TestSuite.h>

#include "FloatUtils.h"
#include "LUFactors.h"
#include "MString.h"

class MockForLUFactors
{
public:
};

class LUFactorsTestSuite : public CxxTest::TestSuite
{
public:
    MockForLUFactors *mock;
    LUFactors *lu;

    void setUp()
    {
        TS_ASSERT( mock = new MockForLUFactors );
        TS_ASSERT( lu = new LUFactors( 4 ) );

        /*
          Set
              | 0 1 0 0 |       | 1 0 0 0 |
          P = | 0 0 0 1 |   Q = | 0 0 0 1 |
              | 1 0 0 0 |       | 0 0 1 0 |
              | 0 0 1 0 |       | 0 1 0 0 |
        */

        lu->_P.swapRows( 0, 1 );
        lu->_P.swapRows( 1, 3 );
        lu->_P.swapRows( 2, 3 );

        lu->_Q.swapRows( 1, 3 );

        /*
              | 1 0  0 0 |
          L = | 2 1  0 0 |
              | 3 0  1 0 |
              | 4 -2 5 1 |

                      |  1 0 2 0 |
              --> F = | -2 1 4 5 |
                      |  0 0 1 0 |
                      |  0 0 3 1 |
        */

        double F[16] =
            {
                1, 0, 2, 0,
                -2, 1, 4, 5,
                0, 0, 1, 0,
                0, 0, 3, 1,
            };

        memcpy( lu->_F, F, sizeof(F) );


        /*
              | 1 3 -2 -3 |
          U = | 0 2 5   1 |
              | 0 0 -2  2 |
              | 0 0 0   7 |

                      |  0  1  5 2 |
              --> V = |  0  7  0 0 |
                      |  1 -3 -2 3 |
                      |  0  2 -2 0 |

        */

        double V[16] =
            {
                0, 1, 5, 2,
                0, 7, 0, 0,
                1, -3, -2, 3,
                0, 2, -2, 0,
            };

        memcpy( lu->_V, V, sizeof(V) );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete lu );
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void test_f_forward_transformation()
    {
        /*
              |  1 0 2 0 |
          F = | -2 1 4 5 |
              |  0 0 1 0 |
              |  0 0 3 1 |

                        | 1	0 -2  0 |
               inv(F) = | 2	1  7 -5 |
                        | 0	0  1  0 |
                        | 0	0 -3  1 |

                   Fx = y
                   x = inv(F)y
        */

        double x1[] = { 1, 2, 3, 4 };
        double expected1[] = { -5, 5, 3, -5 };

        TS_ASSERT_THROWS_NOTHING( lu->fForwardTransformation( x1 ) );
        for ( unsigned i = 0; i < 4; ++i )
            TS_ASSERT( FloatUtils::areEqual( x1[i], expected1[i] ) );

        double x2[] = { 2, 0, -3, 1 };
        double expected2[] = { 8, -22, -3, 10 };

        TS_ASSERT_THROWS_NOTHING( lu->fForwardTransformation( x2 ) );
        for ( unsigned i = 0; i < 4; ++i )
            TS_ASSERT( FloatUtils::areEqual( x2[i], expected2[i] ) );
    }

    void test_f_backward_transformation()
    {
        /*
              |  1 0 2 0 |
          F = | -2 1 4 5 |
              |  0 0 1 0 |
              |  0 0 3 1 |

                        | 1	0 -2  0 |
               inv(F) = | 2	1  7 -5 |
                        | 0	0  1  0 |
                        | 0	0 -3  1 |

                   xF = y
                   x = y inv(F)
        */

        double x1[] = { 1, 2, 3, 4 };
        double expected1[] = { 5, 2, 3, -6 };

        TS_ASSERT_THROWS_NOTHING( lu->fBackwardTransformation( x1 ) );
        for ( unsigned i = 0; i < 4; ++i )
            TS_ASSERT( FloatUtils::areEqual( x1[i], expected1[i] ) );

        double x2[] = { 2, 0, -3, 1 };
        double expected2[] = { 2, 0, -10, 1 };

        TS_ASSERT_THROWS_NOTHING( lu->fBackwardTransformation( x2 ) );
        for ( unsigned i = 0; i < 4; ++i )
            TS_ASSERT( FloatUtils::areEqual( x2[i], expected2[i] ) );
    }

    void test_v_forward_transformation()
    {
        /*
              |  0  1  5 2 |
          V = |  0  7  0 0 |
              |  1 -3 -2 3 |
              |  0  2 -2 0 |

                        | -3/2    2 1 -19/4 |
               inv(V) = |    0  1/7 0     0 |
                        |    0  1/7 0  -1/2 |
                        |  1/2 -3/7 0   5/4 |

                   Vx = y
                   x = inv(V)y
        */

        double y1[] = { 1, 2, 3, 4 };
        double x1[] = { 0, 0, 0, 0 };
        double expected1[] = { -27.0/2, 2.0/7, -12.0/7, 65.0/14 };

        TS_ASSERT_THROWS_NOTHING( lu->vForwardTransformation( y1, x1 ) );
        for ( unsigned i = 0; i < 4; ++i )
            TS_ASSERT( FloatUtils::areEqual( x1[i], expected1[i] ) );

        double y2[] = { 2, 0, -3, 1 };
        double x2[] = { 0, 0, 0, 0 };
        double expected2[] = { -43.0/4, 0, -1.0/2, 9.0/4 };

        TS_ASSERT_THROWS_NOTHING( lu->vForwardTransformation( y2, x2 ) );
        for ( unsigned i = 0; i < 4; ++i )
            TS_ASSERT( FloatUtils::areEqual( x2[i], expected2[i] ) );
    }

    void test_v_backward_transformation()
    {
        /*
              |  0  1  5 2 |
          V = |  0  7  0 0 |
              |  1 -3 -2 3 |
              |  0  2 -2 0 |

                        | -3/2    2 1 -19/4 |
               inv(V) = |    0  1/7 0     0 |
                        |    0  1/7 0  -1/2 |
                        |  1/2 -3/7 0   5/4 |

                   xV = y
                   x = y * inv(V)
        */

        double y1[] = { 1, 2, 3, 4 };
        double x1[] = { 0, 0, 0, 0 };
        double expected1[] = { 1.0/2, 1, 1, -5.0/4 };

        TS_ASSERT_THROWS_NOTHING( lu->vBackwardTransformation( y1, x1 ) );
        for ( unsigned i = 0; i < 4; ++i )
            TS_ASSERT( FloatUtils::areEqual( x1[i], expected1[i] ) );

        double y2[] = { 2, 0, -3, 1 };
        double x2[] = { 0, 0, 0, 0 };
        double expected2[] = { -5.0/2, 22.0/7, 2, -27.0/4 };

        TS_ASSERT_THROWS_NOTHING( lu->vBackwardTransformation( y2, x2 ) );
        for ( unsigned i = 0; i < 4; ++i )
            TS_ASSERT( FloatUtils::areEqual( x2[i], expected2[i] ) );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
