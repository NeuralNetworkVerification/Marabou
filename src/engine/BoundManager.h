/*********************                                                        */
/*! \file BoundManager.h
 ** \verbatim
 ** Top contributors (to current version):
 **  Aleksandar Zeljic, Haoze Wu,
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** BoundManager class is a context-dependent implementation of a centralized
 ** variable registry and their bounds. The intent it so use a single
 ** BoundManager object between multiple bound tightener classes, which enables
 ** those classes to care only about bounds and forget about book-keeping.
 **
 ** BoundManager provides a method to obtain a new variable with:
 ** registerNewVariable().
 **
 ** The bound values and tighten flags are stored using context-dependent objects,
 ** which backtrack automatically with the central _context object.
 **
 ** There are two sets of methods to set bounds:
 **   * set*Bounds     - local method used to update bounds
 **   * tighten*Bounds - shared method to update bounds, propagates the new bounds
 **                        to the _tableau (if registered) to keep the assignment and
 **                      basic/non-basic variables updated accordingly.
 **
 ** As soon as bounds become inconsistent, i.e. lowerBound > upperBound, an
 ** InfeasableQueryException is thrown. In the long run, we want the exception
 ** replaced by a flag, and switch to the conflict analysis mode instead.
 **
 ** It is assumed that variables are not introduced on the fly, and as such
 ** interaction with context-dependent features is not implemented.
 **/

#ifndef __BoundManager_h__
#define __BoundManager_h__

#include "List.h"
#include "Vector.h"
#include "context/cdo.h"
#include "context/context.h"

class Tableau;
class Tightening;
class BoundManager
{
public:
    BoundManager( CVC4::context::Context &ctx );
    ~BoundManager();

    /*
       Initialize BoundManager and register numberOfVariables of variables
     */
    void initialize( unsigned numberOfVariables );

    /*
       Registers a new variable, grows the BoundManager size and bound vectors,
       initializes new bounds to +/-inf, and returns the index of the new
       variable.
     */
    unsigned registerNewVariable();

    /*
       Returns number of registered variables
     */
    unsigned getNumberOfVariables() const;

    /*
       Communicates bounds to the bound Manager and informs _tableau of the
       changes, so that any necessary updates can be performed.
     */
    bool tightenLowerBound( unsigned variable, double value );
    bool tightenUpperBound( unsigned variable, double value );

    /*
       Silently sets bounds to the assigned value, checks bound consistency.
     */
    bool setLowerBound( unsigned variable, double value );
    bool setUpperBound( unsigned variable, double value );

    /*
       Return current bound value.
     */
    double getLowerBound( unsigned variable ) const;
    double getUpperBound( unsigned variable ) const;

    /*
       Obtain a list of all the bound updates since the last call to
       getTightenings.
     */
    void getTightenings( List<Tightening> &tightenings );

    /*
      Returns true if the bounds for the variable is valid, used to
      detect a conflict state.
    */
    bool consistentBounds( unsigned variable );

    /*
       Register Tableau reference for callbacks from tighten*Bound methods.
     */
    void registerTableauReference( Tableau *tableau );

private:
    CVC4::context::Context &_context;
    unsigned _size;
    Tableau *_tableau; // Used only by callbacks

    Vector<CVC4::context::CDO<double> *> _lowerBounds;
    Vector<CVC4::context::CDO<double> *> _upperBounds;

    Vector<CVC4::context::CDO<bool> *> _tightenedLower;
    Vector<CVC4::context::CDO<bool> *> _tightenedUpper;
};

#endif // __BoundManager_h__
