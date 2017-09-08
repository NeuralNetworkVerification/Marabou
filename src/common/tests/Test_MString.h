#include <cxxtest/TestSuite.h>

#include "MString.h"

//template class List<String>;

class StringTestSuite : public CxxTest::TestSuite
{
public:
	void test_ascii()
	{
		char data[] = { "hello world" };

		String string( data );

		TS_ASSERT_EQUALS( strlen( data ), strlen( string.ascii() ) );
		TS_ASSERT_SAME_DATA( data, string.ascii(), strlen( data ) );
	}

	void test_bracket_operator()
	{
	  	String myString( "red apple" );

		TS_ASSERT_EQUALS( myString[0], 'r' );
		TS_ASSERT_EQUALS( myString[3], ' ' );
		TS_ASSERT_EQUALS( myString[8], 'e' );
	}

	void test_empty_constructor()
	{
		String string;

		TS_ASSERT_EQUALS( string.length(), 0U );
	}

	void test_assignemnt_from_c_str()
	{
		String myString;

		TS_ASSERT_EQUALS( myString.length(), 0U );

		myString = "apple";

		TS_ASSERT_EQUALS( myString.length(), strlen( "apple" ) );
		TS_ASSERT_SAME_DATA( myString.ascii(), "apple", myString.length() );
	}

	void test_equality_operator()
	{
		String string( "apple" );
		String otherString( "apple" );
		String different( "banana" );

		TS_ASSERT( string == otherString );
		TS_ASSERT( !( string == different ) );
		TS_ASSERT( string != different );

		TS_ASSERT_EQUALS( string, "apple" );
		TS_ASSERT_DIFFERS( string, "banana" );

        String otherApple( "apple" );

        TS_ASSERT_EQUALS( string, otherString );
        TS_ASSERT_EQUALS( string, otherString.ascii() );
    }

    void test_constructor_limited_length()
    {
        char data[] = "hello world";

        String string( data, 6 );

        String expected( "hello " );
        TS_ASSERT_EQUALS( string, expected );
    }

    void test_comparison_operator()
    {
        TS_ASSERT( String( "a" ) < String( "b" ) );
        TS_ASSERT( String( "aa" ) < String( "ab" ) );
    }

    void test_concatenation()
    {
        String line1 = "hello";
        String line2 = " world";
        String expected = "hello world";

        TS_ASSERT_EQUALS( line1 + line2, expected );

        line1 += line2;

        TS_ASSERT_EQUALS( line1, expected );

        line1 = "hello";

        String line3 = line1 + " world";

        TS_ASSERT_EQUALS( line3, expected );

        line1 += " world";

        TS_ASSERT_EQUALS( line1, expected );
    }

    void test_tokenize()
    {
        String string = "This is a string with several    space  separated words";

        char delimiters[] = " ";
        List<String> tokens = string.tokenize( delimiters );

        TS_ASSERT_EQUALS( tokens.size(), 9U );

        auto it = tokens.begin();

        TS_ASSERT_EQUALS( *it, "This" );
        ++it;
        TS_ASSERT_EQUALS( *it, "is" );
        ++it;
        TS_ASSERT_EQUALS( *it, "a" );
        ++it;
        TS_ASSERT_EQUALS( *it, "string" );
        ++it;
        TS_ASSERT_EQUALS( *it, "with" );
        ++it;
        TS_ASSERT_EQUALS( *it, "several" );
        ++it;
        TS_ASSERT_EQUALS( *it, "space" );
        ++it;
        TS_ASSERT_EQUALS( *it, "separated" );
        ++it;
        TS_ASSERT_EQUALS( *it, "words" );
        ++it;

        String empty;

        tokens = empty.tokenize( delimiters );
        TS_ASSERT_EQUALS( tokens.size(), 0U );
    }

    void test_tokenize__two_delimiters()
    {
        String string = "ThisZis a stringZwith several ZZZ   space  separated words";

        char delimiters[] = " Z";
        List<String> tokens = string.tokenize( delimiters );

        TS_ASSERT_EQUALS( tokens.size(), 9U );

        auto it = tokens.begin();

        TS_ASSERT_EQUALS( *it, "This" );
        ++it;
        TS_ASSERT_EQUALS( *it, "is" );
        ++it;
        TS_ASSERT_EQUALS( *it, "a" );
        ++it;
        TS_ASSERT_EQUALS( *it, "string" );
        ++it;
        TS_ASSERT_EQUALS( *it, "with" );
        ++it;
        TS_ASSERT_EQUALS( *it, "several" );
        ++it;
        TS_ASSERT_EQUALS( *it, "space" );
        ++it;
        TS_ASSERT_EQUALS( *it, "separated" );
        ++it;
        TS_ASSERT_EQUALS( *it, "words" );
        ++it;
    }

    void test_contains()
    {
        String string( "This is a simple string" );

        TS_ASSERT( string.contains( "This is" ) );
        TS_ASSERT( string.contains( "a simple" ) );
        TS_ASSERT( string.contains( " string" ) );
        TS_ASSERT( string.contains( "" ) );
        TS_ASSERT( string.contains( " " ) );

        TS_ASSERT( !string.contains( "Thisis" ) );
        TS_ASSERT( !string.contains( "crazy" ) );
    }

    void test_find()
    {
        String string( "This is a simple string" );

        TS_ASSERT_EQUALS( string.find( "is a" ), 5U );
        TS_ASSERT_EQUALS( string.find( " " ), 4U );
        TS_ASSERT_EQUALS( string.find( "simple string" ), 10U );

        TS_ASSERT_EQUALS( string.find( "crazy" ), String::Super::npos );
    }

    void test_substring()
    {
        String string( "This is a simple string" );

        TS_ASSERT_EQUALS( string.substring( 0, 4 ), "This" );
        TS_ASSERT_EQUALS( string.substring( 4, 3 ), " is" );
        TS_ASSERT_EQUALS( string.substring( 7, 3 ), " a " );
    }

    void test_replace()
    {
        String string( "As I was going to st Ives" );

        string.replace( "going", "running" );

        TS_ASSERT_EQUALS( string, String( "As I was running to st Ives" ) );

        string.replace( "I", "you" );

        TS_ASSERT_EQUALS( string, String( "As you was running to st Ives" ) );

        TS_ASSERT_THROWS_NOTHING( string.replace( "bla", "other bla" ) );

        TS_ASSERT_EQUALS( string, String( "As you was running to st Ives" ) );

        String string2( "here is a string" );

        TS_ASSERT_THROWS_NOTHING( string2.replace( " ", "_" ) );
        TS_ASSERT_EQUALS( string2, String( "here_is a string" ) );

        TS_ASSERT_THROWS_NOTHING( string2.replaceAll( " ", "_" ) );
        TS_ASSERT_EQUALS( string2, String( "here_is_a_string" ) );
    }

    void test_trim()
    {
        TS_ASSERT_EQUALS( String( "   hello      world     \n" ).trim() , "hello      world" );
        TS_ASSERT_EQUALS( String( "              \n" ).trim() , "" );
        TS_ASSERT_EQUALS( String( "   a" ).trim() , "a" );
        TS_ASSERT_EQUALS( String( "   ab\n" ).trim() , "ab" );
        TS_ASSERT_EQUALS( String( "ab\n   \n" ).trim() , "ab" );
        TS_ASSERT_EQUALS( String( "hey there   \n" ).trim() , "hey there" );
    }
};

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
