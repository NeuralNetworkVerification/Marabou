#include <cstdio>


#include "Engine.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "InputQuery.h"
#include "Preprocessor.h"
#include "ReluplexError.h"

#include "QueryLoader.h"

int main(){

	InputQuery inputQuery = load_query("test_TF");
	
	try{

	// Feed the query to the engine
	    Engine engine;
	    bool preprocess = engine.processInputQuery( inputQuery );

	    if ( !preprocess || !engine.solve() )
	    {
	        printf( "\n\nQuery is unsat\n" );
	        return 0;
	    }

	    printf( "\n\nQuery is sat! Extracting solution...\n" );
	    engine.extractSolution( inputQuery );

	}

	catch ( const ReluplexError &e )
	{
	    printf( "Caught a ReluplexError. Code: %u. Message: %s\n", e.getCode(), e.getUserMessage() );
	    return 0;
	}
	//InputQuery inputQuery;

}