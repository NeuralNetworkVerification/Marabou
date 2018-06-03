#ifndef __QueryLoader_h__
#define __QueryLoader_h__

#include "InputQuery.h"

// Guy: TODO: please add comments.
// Why is the function load_query outside of the class? You can make it static if you like.
class QueryLoader
{
public:
    // String filename;
    int numVars;
    int numEquations;
    int numConstraints;

    static InputQuery loadQuery( const char *filename );
};

// Functions Implemented


#endif // __QueryLoader_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
