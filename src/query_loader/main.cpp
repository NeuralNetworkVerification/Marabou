#include <cstdio>

#include "Engine.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "InputQuery.h"
#include "Preprocessor.h"
#include "QueryLoader.h"
#include "ReluplexError.h"

int main()
{
    InputQuery inputQuery = QueryLoader::loadQuery( "test_NNETc" );
    inputQuery.saveQuery( "test_NNETc_after" );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
