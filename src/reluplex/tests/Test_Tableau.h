#include <cxxtest/TestSuite.h>

#include "MockErrno.h"
#include "Tableau.h"
#include <string.h>

class MockForTableau
{
public:
};

class TableauTestSuite : public CxxTest::TestSuite
{
public:
    MockForTableau *mock;

    void tableauUp()
    {
        TS_ASSERT( mock = new MockForTableau );
    }

    void tearDown()
    {
        TS_ASSERT_THROWS_NOTHING( delete mock );
    }

    void initializeTableauValues( Tableau &tableau )
    {
        /*
              | 3 2 1 2 1 0 0 |
          A = | 1 1 1 1 0 1 0 |
              | 4 3 3 4 0 0 1 |
        */

        tableau.setEntryValue( 0, 0, 3 );
        tableau.setEntryValue( 0, 1, 2 );
        tableau.setEntryValue( 0, 2, 1 );
        tableau.setEntryValue( 0, 3, 2 );
        tableau.setEntryValue( 0, 4, 1 );
        tableau.setEntryValue( 0, 5, 0 );
        tableau.setEntryValue( 0, 6, 0 );

        tableau.setEntryValue( 1, 0, 1 );
        tableau.setEntryValue( 1, 1, 1 );
        tableau.setEntryValue( 1, 2, 1 );
        tableau.setEntryValue( 1, 3, 1 );
        tableau.setEntryValue( 1, 4, 0 );
        tableau.setEntryValue( 1, 5, 1 );
        tableau.setEntryValue( 1, 6, 0 );

        tableau.setEntryValue( 2, 0, 4 );
        tableau.setEntryValue( 2, 1, 3 );
        tableau.setEntryValue( 2, 2, 3 );
        tableau.setEntryValue( 2, 3, 4 );
        tableau.setEntryValue( 2, 4, 0 );
        tableau.setEntryValue( 2, 5, 0 );
        tableau.setEntryValue( 2, 6, 1 );
    }

    void test_backward_transformation_no_eta()
    {
        Tableau tableau;

        TS_ASSERT_THROWS_NOTHING( tableau.setDimensions( 3, 7 ) );
        initializeTableauValues( tableau );

        Set<unsigned> basicVariables = { 4, 5, 6 };
        TS_ASSERT_THROWS_NOTHING( tableau.initializeBasisMatrix( basicVariables ) );

        double result[3];
        std::fill( result, result + 3, 0.0 );

        tableau.dump();

        TS_ASSERT_THROWS_NOTHING( tableau.backwardTransformation( 1, result ) );

        // In this case, B = I, and so we expect the result d to be the column a that
        // belongs to the entering variable, 1.
        TS_ASSERT_EQUALS( result[2], 3.0 );
        TS_ASSERT_EQUALS( result[1], 1.0 );
        TS_ASSERT_EQUALS( result[0], 2.0 );
    }
};

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
