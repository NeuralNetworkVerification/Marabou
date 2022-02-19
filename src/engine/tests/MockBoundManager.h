/*********************                                                        */
/*! \file MockBoundManager.h
 ** \verbatim
 ** Top contributors (to current version):
 **  Aleksandar Zeljic, Haoze Wu,
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 **/

#ifndef __MockBoundManager_h__
#define __MockBoundManager_h__

#include "IBoundManager.h"
#include "FloatUtils.h"
#include "List.h"
#include "Vector.h"
#include "Map.h"
#include "Tightening.h"

class Tableau;
class MockBoundManager : public IBoundManager
{
public:
    MockBoundManager()
      : _size( 0 )
      , _lowerBounds()
      , _upperBounds()
      , _tightenedLower()
      , _tightenedUpper()
    {
    };

    ~MockBoundManager()
    {
    };

    /*
       Registers a new variable, grows the BoundManager size and bound vectors,
       initializes new bounds to +/-inf, and returns the index of the new
       variable.
     */
    unsigned registerNewVariable()
    {
      return -1;
    };

    /*
       Returns number of registered variables
     */
    unsigned getNumberOfVariables() const
    {
      return _size;
    };

    double getLowerBound( unsigned variable ) const
    {
        return _lowerBounds[variable];
    }

    /*
       Records a new bound if tighter the current bound.
     */
    bool tightenLowerBound( unsigned variable, double value )
    {
      if ( FloatUtils::lt( getLowerBound( variable), value ) )
      {
        setLowerBound( variable, value);
        _tightenedLower[variable] = true;
        return true;
      }

      return false;
    }

    double getUpperBound( unsigned variable ) const
    {
      return _upperBounds[variable];
    }

    bool tightenUpperBound( unsigned variable, double value )
    {
      if ( FloatUtils::gt( getUpperBound( variable ), value ) )
      {
        setUpperBound( variable, value);
        _tightenedUpper[variable] = true;
        return true;
      }

      return false;
    };

    /*
       Silently sets bounds to the assigned value, checks bound consistency.
     */
    bool setLowerBound( unsigned variable, double value )
    {
      _lowerBounds[variable] = value;
      return true;
    };

    bool setUpperBound( unsigned variable, double value )
    {
      _upperBounds[variable] = value;
      return true;
    };

    /*
       Obtain a list of all the bound updates since the last call to
       getTightenings.
     */
    void getTightenings( List<Tightening> &tightenings )
    {

        for ( auto it : _tightenedLower )
        {
            unsigned var =  it.first;
            bool tightened =  it.second;

            if ( tightened )
            {
                tightenings.append(
                    Tightening( var, _lowerBounds[var], Tightening::LB ) );
                it.second = false;
            }
        }

        for ( auto it : _tightenedUpper )
        {
            unsigned var =  it.first;
            bool tightened =  it.second;
            if ( tightened )
            {
                tightenings.append(
                    Tightening( var, _upperBounds[var], Tightening::UB ) );
                it.second = false;
            }
        }
    }

    /*
      Returns true if the bounds for the variable is valid, used to
      detect a conflict state.
    */
    bool consistentBounds( unsigned variable ) const
    {
      return getLowerBound( variable ) <= getUpperBound( variable );
    };

private:
    unsigned _size;

    Map<unsigned, double> _lowerBounds;
    Map<unsigned, double> _upperBounds;
    Map<unsigned, bool> _tightenedLower;
    Map<unsigned, bool> _tightenedUpper;
    /* double *_lowerBounds; */
    /* double *_upperBounds; */

    /* bool *_tightenedLower; */
    /* bool *_tightenedUpper; */
};

#endif // __MockBoundManager_h__
