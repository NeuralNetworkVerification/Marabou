/*********************                                                        */
/*! \file IBoundManager.h
 ** \verbatim
 ** Top contributors (to current version):
 **  Aleksandar Zeljic, Haoze Wu,
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** IBoundManager is interface for implementation of a BoundManager class.
 **
 ** IBoundManager provides a method to obtain a new variable with:
 ** registerNewVariable().
 **
 ** There are two sets of methods to set bounds:
 **   * set*Bounds     - local method used to update bounds
 **   * tighten*Bounds - shared method to update bounds, propagates the new bounds
 **
 ** As soon as bounds become inconsistent, i.e. lowerBound > upperBound, a
 ** _consistentBounds flag is set which can be checked using consistentBounds() method.
 **
 **/

#ifndef __IBoundManager_h__
#define __IBoundManager_h__

#include "List.h"

class Tightening;
class ITableau;
class IRowBoundTightener;
class IBoundManager
{
public:
    virtual ~IBoundManager() {};

    /*
       Registers a new variable, grows the BoundManager size and bound vectors,
       initializes new bounds to +/-inf, and returns the identifier of the new
       variable.
     */
    virtual unsigned registerNewVariable() = 0;

    /*
       Initialize BoundManager to a given number of variables;
     */
    virtual void initialize( unsigned numberOfVariables ) = 0;

    /*
       Returns number of registered variables
     */
    virtual unsigned getNumberOfVariables() const = 0;

    /*
       Records a new bound if tighter than the current bound.
     */
    virtual bool tightenLowerBound( unsigned variable, double value ) = 0;
    virtual bool tightenUpperBound( unsigned variable, double value ) = 0;

    /*
       Sets the bould to the assigned value, checks bound consistency.
     */
    virtual bool setLowerBound( unsigned variable, double value ) = 0;
    virtual bool setUpperBound( unsigned variable, double value ) = 0;

    /*
       Return bound value.
     */
    virtual double getLowerBound( unsigned variable ) const = 0;
    virtual double getUpperBound( unsigned variable ) const = 0;

    /*
      Get pointers to latest bounds used for access by tableau and tighteners
     */
    virtual const double * getLowerBounds() const = 0;
    virtual const double * getUpperBounds() const = 0;

    /*
     * Store and restore bounds after push/pop
     */
    /*
       Obtain a list of all the bound updates since the last call to
       getTightenings.
     */
    virtual void getTightenings( List<Tightening> &tightenings ) = 0;
    virtual void clearTightenings() = 0;

    /*
      Returns true if the bounds for the variable is valid. Used to
      detect a conflict state.
    */
    virtual bool consistentBounds() const = 0;

    /*
      Register Tableau for callbacks.
     */
    virtual void registerTableau( ITableau *tableau ) = 0;

    /*
       Register RowBoundTightener for callbacks.
     */
    virtual void registerRowBoundTightener( IRowBoundTightener *ptrRowBoundTightener ) = 0;


};

#endif // __IBoundManager_h__
