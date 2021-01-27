/*********************                                                        */
/*! \file BoundManager.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu, Aleksandar Zeljic
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** BoundManager class is a context-dependent implementation of a centralized set
 ** of bounds. It is intended to be a single centralized object used between multiple
 ** bound tightener classes, which enables those classes to care only about bounds
 ** and forget about book-keeping.
 **
 ** BoundManager serves as a central variable registry and provides a method to obtain
 ** a new variable id: registerNewVariable().
 **
 ** The bound values and tighten flags are stored using context-dependent objects,
 ** which backtrack automatically with a centralized _context object.
**/

#ifndef __BoundManager_h__
#define __BoundManager_h__

#include "context/cdo.h"
#include "context/context.h"
#include "List.h"
#include "Vector.h"

class Tableau;
class Tightening;
class BoundManager
{
public:
    BoundManager( CVC4::context::Context &ctx );
    ~BoundManager();

    /*
     * Initialize BoundManager to numberOfVariables
     */
    void initialize( unsigned numberOfVariables );

    /*
     * Registers a new variable, grows the BoundManager size and bound vectors,
     * initializes bounds to +/-inf, and returns the new index as the new variable.
     */
    unsigned registerNewVariable( );

    /*
     * Returns number of registered variables
     */
    unsigned getNumberOfVariables();

    /*
     * Communicates bounds to the bound Manager and informs _tableau of the changes,
     * so that any necessary updates can be performed.
     */
    bool tightenLowerBound( unsigned variable, double value );
    bool tightenUpperBound( unsigned variable, double value );

    /*
     * Silently sets bounds to the assigned value, if consistent.
     */
    bool setLowerBound( unsigned variable, double value );
    bool setUpperBound( unsigned variable, double value );

    /*
     * Return current bound value.
     */
    double getLowerBound( unsigned variable );
    double getUpperBound( unsigned variable );

    /*
     * Obtain a list of all the bound updates since the last call to getTightenings.
     */
    void getTightenings( List<Tightening> &tightenings );

    /*
          Returns true if the bounds for the variable is valid, used to detectConflict state.
    */
    bool consistentBounds( unsigned variable );


    /*
       Register Tableau reference for callbacks from tighten*Bound methods.
     */
    void registerTableauReference( Tableau *tableau );

private:
    CVC4::context::Context &_context;
    Tableau *_tableau;
    unsigned _size;
    Vector<CVC4::context::CDO<double> *> _lowerBounds;
    Vector<CVC4::context::CDO<double> *> _upperBounds;
    Vector<CVC4::context::CDO<bool> *> _tightenedLower;
    Vector<CVC4::context::CDO<bool> *> _tightenedUpper;
};


#endif // __BoundManager_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
