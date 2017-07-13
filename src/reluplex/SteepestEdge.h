#ifndef __SteepestEdge_h__
#define __SteepestEdge_h__

#include "EntrySelectionStrategy.h"

class SteepestEdge : public EntrySelectionStrategy
{
public:
    /*
      Apply steepest edge pivot selection rule: choose the candidate that maximizes the
      gradient of the cost function with respect to the step direction.
    */
    bool select( ITableau &tableau );
    void initialize( const ITableau &tableau );

};

#endif // __SteepestEdge_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
