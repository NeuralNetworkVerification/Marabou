
#ifndef __QueryLoader_h__
#define __QueryLoader_h__

#include "InputQuery.h"

class QueryLoader {
public:
    //string filename;
    int numVars;
    int numEquations;
    int numConstraints;
};

//Functions Implemented
InputQuery load_query(const char *filename);

#endif // __AcasNnet_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
