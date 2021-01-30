/*********************                                                        */
/*! \file Test_BoundManager.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze (Andrew) Wu, Aleksandar Zeljic
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include <cxxtest/TestSuite.h>

#include "BoundManager.h"
#include "context/cdlist.h"
#include "context/context.h"
#include "FloatUtils.h"
#include "InfeasibleQueryException.h"
#include "Tightening.h"

using CVC4::context::Context;

class BoundManagerTestSuite : public CxxTest::TestSuite
{
public:

    Context * context;

    void setUp()
    {
        TS_ASSERT_THROWS_NOTHING( context = new Context );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete context; );
    }

    /*
     * Initialize creates bounds for every variable up to numberOfVariables
     * and sets their lower/upper bounds to -/+infinity respectively
     */
    void test_bound_manager_initialize()
    {
        BoundManager boundManager( *context );

        unsigned numberOfVariables = 5u;
        TS_ASSERT_THROWS_NOTHING( boundManager.initialize( numberOfVariables) );

        for ( unsigned i = 0; i < numberOfVariables; ++i)
        {
            TS_ASSERT( FloatUtils::areEqual( boundManager.getLowerBound( i ),
                                             FloatUtils::negativeInfinity() ) );
            TS_ASSERT( FloatUtils::areEqual( boundManager.getUpperBound( i ),
                                             FloatUtils::infinity() ) );
        }

    }

    /*
     * BoundManager correctly updates the number of variables with advancement
     * and backtracking of context
     *
     */
    void test_register_variable()
    {
        BoundManager boundManager( *context );

        unsigned numberOfVariables = 5u;

        TS_ASSERT_THROWS_NOTHING( boundManager.initialize( numberOfVariables ) );

        TS_ASSERT_EQUALS( boundManager.getNumberOfVariables(), 5u );
        TS_ASSERT( FloatUtils::areEqual( boundManager.getLowerBound( 4 ),
                                         FloatUtils::negativeInfinity() ) );
        TS_ASSERT( FloatUtils::areEqual( boundManager.getUpperBound( 4 ),
                                         FloatUtils::infinity() ) );

        TS_ASSERT_THROWS_NOTHING( boundManager.registerNewVariable() );
        TS_ASSERT_THROWS_NOTHING( boundManager.registerNewVariable() );
        TS_ASSERT_EQUALS( boundManager.getNumberOfVariables(), 7u );
        TS_ASSERT( FloatUtils::areEqual( boundManager.getLowerBound( 6 ),
                                         FloatUtils::negativeInfinity() ) );
        TS_ASSERT( FloatUtils::areEqual( boundManager.getUpperBound( 6 ),
                                         FloatUtils::infinity() ) );
    }

    /*
     * BoundManager throws infeasible query exception when some variable bounds
     * become invalid
     *
     */
    void test_bound_valid()
    {
        BoundManager boundManager( *context );

        unsigned numberOfVariables = 1u;

        TS_ASSERT_THROWS_NOTHING( boundManager.initialize( numberOfVariables ) );

        TS_ASSERT_THROWS_NOTHING( boundManager.setLowerBound( 0, 1 ) );
        TS_ASSERT_THROWS_NOTHING( boundManager.setUpperBound( 0, 2 ) );
        TS_ASSERT( boundManager.consistentBounds( 0 ) );

        TS_ASSERT_THROWS_NOTHING( boundManager.setUpperBound( 0, 1 ) );
        TS_ASSERT_THROWS( boundManager.setUpperBound( 0, 0 ), InfeasibleQueryException );
    }

    /*
     * BoundManager correctly updates bounds with advancement and backtracking of context
     * 
     */
    void test_bound_manager_context_interaction()
    {
      BoundManager boundManager( *context );

      unsigned numberOfVariables = 5u;

      TS_ASSERT_THROWS_NOTHING( boundManager.initialize( numberOfVariables ) );

      double level0Lower[] = { -12.357682,  0.230001234, -333.78091231, 100.00,    -9.000002354 };
      double level0Upper[] = {  15.387692, 20.301878234,   45.79159213, 120.03559, 89.53402 };
      double level1Lower[] = { -2.357682,   5.230001234, -222.87012913, 103.5682,  -5.002300054 };
      double level1Upper[] = {  5.387692,  15.308798432,   26.79159213, 119.5559,  77.500002 };
      double level2Lower[] = {  2.523786,   8.231234000, -122.01291387, 111.5392,  10.002300054 };
      double level2Upper[] = {  3.738962,   8.308432000,   16.79211593, 115.9003,  57.5459822 };


      TS_ASSERT_THROWS_NOTHING( context->push() )

      for ( unsigned v = 0; v < numberOfVariables; ++v )
      {
        TS_ASSERT_THROWS_NOTHING( boundManager.setLowerBound( v, level0Lower[v] ) );
        TS_ASSERT_THROWS_NOTHING( boundManager.setUpperBound( v, level0Upper[v] ) );

        TS_ASSERT_EQUALS( boundManager.getLowerBound( v ), level0Lower[v] );
        TS_ASSERT_EQUALS( boundManager.getUpperBound( v ), level0Upper[v] );
      }

      TS_ASSERT_THROWS_NOTHING( context->push() );

      for ( unsigned v = 0; v < numberOfVariables; ++v )
      {
          TS_ASSERT_THROWS_NOTHING( boundManager.setLowerBound( v, level1Lower[v] ) );
          TS_ASSERT_THROWS_NOTHING( boundManager.setUpperBound( v, level1Upper[v] ) );

          TS_ASSERT_EQUALS( boundManager.getLowerBound( v ), level1Lower[v] );
          TS_ASSERT_EQUALS( boundManager.getUpperBound( v ), level1Upper[v] );
      }


      TS_ASSERT_THROWS_NOTHING( context->push() );

      for ( unsigned v = 0; v < numberOfVariables; ++v )
      {
          TS_ASSERT_THROWS_NOTHING( boundManager.setLowerBound( v, level2Lower[v] ) );
          TS_ASSERT_THROWS_NOTHING( boundManager.setUpperBound( v, level2Upper[v] ) );

          TS_ASSERT_EQUALS( boundManager.getLowerBound( v ), level2Lower[v] );
          TS_ASSERT_EQUALS( boundManager.getUpperBound( v ), level2Upper[v] );
       }


      TS_ASSERT_THROWS_NOTHING( context->pop() );

      for ( unsigned v = 0; v < numberOfVariables; ++v )
      {
          TS_ASSERT_EQUALS( boundManager.getLowerBound( v ), level1Lower[v] );
          TS_ASSERT_EQUALS( boundManager.getUpperBound( v ), level1Upper[v] );
      }


      TS_ASSERT_THROWS_NOTHING( context->pop() );

      for ( unsigned v = 0; v < numberOfVariables; ++v )
      {
          TS_ASSERT_EQUALS( boundManager.getLowerBound( v ), level0Lower[v] );
          TS_ASSERT_EQUALS( boundManager.getUpperBound( v ), level0Upper[v] );
      }

      for ( unsigned v = 0; v < numberOfVariables; ++v )
      {
          TS_ASSERT_THROWS_NOTHING( boundManager.setLowerBound( v, level1Lower[v] ) );
          TS_ASSERT_THROWS_NOTHING( boundManager.setUpperBound( v, level1Upper[v] ) );

          TS_ASSERT_EQUALS( boundManager.getLowerBound( v ), level1Lower[v] );
          TS_ASSERT_EQUALS( boundManager.getUpperBound( v ), level1Upper[v] );
      }

      for ( unsigned v = 0; v < numberOfVariables; ++v )
        {
          TS_ASSERT_THROWS_NOTHING( boundManager.setLowerBound( v, level2Lower[v] ) );
          TS_ASSERT_THROWS_NOTHING( boundManager.setUpperBound( v, level2Upper[v] ) );

          TS_ASSERT_EQUALS( boundManager.getLowerBound( v ), level2Lower[v] );
          TS_ASSERT_EQUALS( boundManager.getUpperBound( v ), level2Upper[v] );
        }

      TS_ASSERT_THROWS_NOTHING( context->pop() );

      for ( unsigned v = 0; v < numberOfVariables; ++v )
        {
          TS_ASSERT_EQUALS( boundManager.getLowerBound( v ), FloatUtils::negativeInfinity() );
          TS_ASSERT_EQUALS( boundManager.getUpperBound( v ), FloatUtils::infinity() );
        }
    }

};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:

