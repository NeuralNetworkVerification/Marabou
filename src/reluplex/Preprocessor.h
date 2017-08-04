#ifndef __Preprocessor_h__
#define __Preprocessor_h__

#include "Equation.h"
#include "List.h"
#include "Map.h"
#include "PiecewiseLinearConstraint.h"
#include "InputQuery.h"

class Preprocessor
{
public:
	Preprocessor();

	void tightenBounds( InputQuery &input );

};

#endif //__Preprocessor_h__ 

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
