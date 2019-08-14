/*********************                                                        */
/*! \file Test_SparseLUFactors.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include <cxxtest/TestSuite.h>

#include "FloatUtils.h"
#include "SparseLUFactors.h"
#include "MString.h"

class MockForSparseLUFactors
{
public:
};

class SparseLUFactorsTestSuite : public CxxTest::TestSuite
{
public:
    MockForSparseLUFactors *mock;
    SparseLUFactors *lu;

    void transpose( const double *A, double *At, unsigned dim )
    {
        for ( unsigned i = 0; i < dim; ++i )
        {
            for ( unsigned j = 0; j < dim; ++j )
            {
                At[i*dim + j] = A[j*dim + i];
            }
        }
    }

    void setUp()
    {
        TS_ASSERT( mock = new MockForSparseLUFactors );
        TS_ASSERT( lu = new SparseLUFactors( 4 ) );

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

           Recall that F's 1-diagonal is IMPLICIT
        */

        double F[16] =
            {
                0, 0, 2, 0,
                -2, 0, 4, 5,
                0, 0, 0, 0,
                0, 0, 3, 0,
            };

        lu->_F->initialize( F, 4, 4 );

        double Ft[16];
        transpose( F, Ft, 4 );
        lu->_Ft->initialize( Ft, 4, 4 );

        /*
              | 1 3 -2 -3 |
          U = | 0 2 5   1 |
              | 0 0 -2  2 |
              | 0 0 0   7 |

                      |  0  1  5 2 |
              --> V = |  0  7  0 0 |
                      |  1 -3 -2 3 |
                      |  0  2 -2 0 |

                        | -3/2    2 1 -19/4 |
               inv(V) = |    0  1/7 0     0 |
                        |    0  1/7 0  -1/2 |
                        |  1/2 -3/7 0   5/4 |

         inv(V) = Q' * inv(U) * P'

              | 0 1 0 0 |       | 1 0 0 0 |
          P = | 0 0 0 1 |   Q = | 0 0 0 1 |
              | 1 0 0 0 |       | 0 0 1 0 |
              | 0 0 1 0 |       | 0 1 0 0 |
        */


        double V[16] =
            {
                0, 1, 5, 2,
                0, 7, 0, 0,
                1, -3, -2, 3,
                0, 2, -2, 0,
            };

        lu->_V->initialize( V, 4, 4 );

        double Vt[16];
        transpose( V, Vt, 4 );
        lu->_Vt->initialize( Vt, 4, 4 );

        /*
          If row i of V corresponds to row j of U, the i'th diaonal element will be U[j,j].
        */
        lu->_vDiagonalElements[0] = 2;
        lu->_vDiagonalElements[1] = 7;
        lu->_vDiagonalElements[2] = 1;
        lu->_vDiagonalElements[3] = -2;

        /*
          Implies A = FV = | 2 -5   1 8 |
                           | 4  3 -28 8 |
                           | 1 -3  -2 3 |
                           | 3 -7  -8 9 |
        */
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

        double y1[] = { 1, 2, 3, 4 };
        double x1[] = { 0, 0, 0, 0 };
        double expected1[] = { -5, 5, 3, -5 };

        TS_ASSERT_THROWS_NOTHING( lu->fForwardTransformation( y1, x1 ) );

        for ( unsigned i = 0; i < 4; ++i )
            TS_ASSERT( FloatUtils::areEqual( x1[i], expected1[i] ) );

        double y2[] = { 2, 0, -3, 1 };
        double x2[] = { 0, 0, 0, 0 };
        double expected2[] = { 8, -22, -3, 10 };

        TS_ASSERT_THROWS_NOTHING( lu->fForwardTransformation( y2, x2 ) );
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

        double y1[] = { 1, 2, 3, 4 };
        double x1[] = { 0, 0, 0, 0 };
        double expected1[] = { 5, 2, 3, -6 };

        TS_ASSERT_THROWS_NOTHING( lu->fBackwardTransformation( y1, x1 ) );
        for ( unsigned i = 0; i < 4; ++i )
            TS_ASSERT( FloatUtils::areEqual( x1[i], expected1[i] ) );

        double y2[] = { 2, 0, -3, 1 };
        double x2[] = { 0, 0, 0, 0 };
        double expected2[] = { 2, 0, -10, 1 };

        TS_ASSERT_THROWS_NOTHING( lu->fBackwardTransformation( y2, x2 ) );
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

    void test_forward_transformation()
    {
        /*
           A = | 2 -5   1 8 |
               | 4  3 -28 8 |
               | 1 -3  -2 3 |
               | 3 -7  -8 9 |

                        | 5/2      2 129/4  -59/4 |
               inv(A) = | 2/7    1/7     1   -5/7 |
                        | 2/7    1/7   5/2 -17/14 |
                        | -5/14 -3/7 -31/4  95/28 |

                   Ax = y
                   x = inv(A)y
        */

        double y1[] = { 1, 2, 3, 4 };
        double x1[] = { 0, 0, 0, 0 };
        double expected1[] = { 177.0/4, 5.0/7, 45.0/14, -305.0/28 };

        TS_ASSERT_THROWS_NOTHING( lu->forwardTransformation( y1, x1 ) );
        for ( unsigned i = 0; i < 4; ++i )
            TS_ASSERT( FloatUtils::areEqual( x1[i], expected1[i] ) );

        double y2[] = { 2, 0, -3, 1 };
        double x2[] = { 0, 0, 0, 0 };
        double expected2[] = { -213.0/2, -22.0/7, -57.0/7, 363.0/14 };

        TS_ASSERT_THROWS_NOTHING( lu->forwardTransformation( y2, x2 ) );
        for ( unsigned i = 0; i < 4; ++i )
            TS_ASSERT( FloatUtils::areEqual( x2[i], expected2[i] ) );
    }

    void test_backward_transformation()
    {
        /*
           A = | 2 -5   1 8 |
               | 4  3 -28 8 |
               | 1 -3  -2 3 |
               | 3 -7  -8 9 |

                        | 5/2      2 129/4  -59/4 |
               inv(A) = | 2/7    1/7     1   -5/7 |
                        | 2/7    1/7   5/2 -17/14 |
                        | -5/14 -3/7 -31/4  95/28 |

                   xA = y
                   x = y inv(A)
        */

        double y1[] = { 1, 2, 3, 4 };
        double x1[] = { 0, 0, 0, 0 };
        double expected1[] = { 5.0/2, 1, 43.0/4, -25.0/4 };

        TS_ASSERT_THROWS_NOTHING( lu->backwardTransformation( y1, x1 ) );
        for ( unsigned i = 0; i < 4; ++i )
            TS_ASSERT( FloatUtils::areEqual( x1[i], expected1[i] ) );

        double y2[] = { 2, 0, -3, 1 };
        double x2[] = { 0, 0, 0, 0 };
        double expected2[] = { 53.0/14, 22.0/7, 197.0/4, -629.0/28 };

        TS_ASSERT_THROWS_NOTHING( lu->backwardTransformation( y2, x2 ) );
        for ( unsigned i = 0; i < 4; ++i )
            TS_ASSERT( FloatUtils::areEqual( x2[i], expected2[i] ) );
    }

    void test_invert_basis()
    {
        /*
           A = | 2 -5   1 8 |
               | 4  3 -28 8 |
               | 1 -3  -2 3 |
               | 3 -7  -8 9 |

                        | 5/2      2 129/4  -59/4 |
               inv(A) = | 2/7    1/7     1   -5/7 |
                        | 2/7    1/7   5/2 -17/14 |
                        | -5/14 -3/7 -31/4  95/28 |
        */
        double expectedInverse[] = {
              5.0/2,      2, 129.0/4,  -59.0/4,
              2.0/7,  1.0/7,       1,   -5.0/7,
              2.0/7,  1.0/7,   5.0/2, -17.0/14,
            -5.0/14, -3.0/7, -31.0/4,  95.0/28,
        };

        double result[16];

        TS_ASSERT_THROWS_NOTHING( lu->invertBasis( result ) );

        for ( unsigned i = 0; i < 16; ++i )
            TS_ASSERT( FloatUtils::areEqual( result[i], expectedInverse[i] ) );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
