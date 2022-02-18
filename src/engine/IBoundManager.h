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
 ** As soon as bounds become inconsistent, i.e. lowerBound > upperBound, an
 ** InfeasableQueryException is thrown. In the long run, we want the exception
 ** replaced by a flag, and switch to the conflict analysis mode instead.
 **
 **/

#ifndef __IBoundManager_h__
#define __IBoundManager_h__

#include "List.h"

class Tightening;
class IBoundManager
{
public:
  virtual ~IBoundManager() {};

    virtual void initialize( unsigned numberOfVariables ) = 0;

    /*
       Registers a new variable, grows the BoundManager size and bound vectors,
       initializes new bounds to +/-inf, and returns the identifier of the new
       variable.
     */
    virtual unsigned registerNewVariable() = 0;

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
       Obtain a list of all the bound updates since the last call to
       getTightenings.
     */
    virtual void getTightenings( List<Tightening> &tightenings ) = 0;

    /*
      Returns true if the bounds for the variable is valid. Used to
      detect a conflict state.
    */
    virtual bool consistentBounds( unsigned variable ) const = 0;

};

#endif // __IBoundManager_h__
