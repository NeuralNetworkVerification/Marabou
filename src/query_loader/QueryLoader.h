#ifndef __QueryLoader_h__
#define __QueryLoader_h__

#include "InputQuery.h"

class QueryLoader
{
public:
    unsigned _numVars;
    unsigned _numEquations;
    unsigned _numConstraunsigneds;

    /*
      Parse a serialized query and return it in InputQuery form
    */
    static InputQuery loadQuery( const String &fileName );

    static void log( const String &message );
};

#endif // __QueryLoader_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
