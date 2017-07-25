/*********************                                                        */
/*! \file PiecewiseLinearCaseSplit.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __PiecewiseLinearCaseSplit_h__
#define __PiecewiseLinearCaseSplit_h__

#include "Equation.h"

class PiecewiseLinearCaseSplit
{
public:
    struct Bound
    {
    public:
        enum Type {
            LOWER = 0,
            UPPER = 1,
        };

        Bound( unsigned variable, Bound::Type boundType, double newBound )
            : _variable( variable )
            , _boundType( boundType )
            , _newBound( newBound )
        {
        }

        /*
          The variable whos bound is being tightened.
        */
        unsigned _variable;

        /*
          Indicates whether this is an upper or a lower bound.
        */
        Bound::Type _boundType;

        /*
          Value of the new bound.
        */
        double _newBound;
    };

    /*
      Store information regarding a bound tightening.
    */
    void storeBoundTightening( const Bound &bound );
    List<PiecewiseLinearCaseSplit::Bound> getBoundTightenings() const;

    /*
      Store information regarding a new equation to be added.
    */
    void addEquation( const Equation &equation );

	List<Equation> getEquation() const;

private:
    /*
      Bound tightening information.
    */
    List<PiecewiseLinearCaseSplit::Bound> _bounds;

    /*
      The equation that needs to be added.
    */
    List<Equation> _equations;
};

#endif // __PiecewiseLinearCaseSplit_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
