/*********************                                                        */
/*! \file ReluConstraint.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __ReluConstraint_h__
#define __ReluConstraint_h__

#include "PiecewiseLinearConstraint.h"

class ReluConstraint : public PiecewiseLinearConstraint
{
public:
    ReluConstraint( unsigned b, unsigned f );

    /*
      Returns true iff the variable participates in this piecewise
      linear constraint
    */
    bool participatingVariable( unsigned variable ) const;

    /*
      Returns true iff the given assignment satisfies the constraint
    */
    bool satisfied( const Map<unsigned, double> &assignment ) const;

private:
    unsigned _b;
    unsigned _f;
};

#endif // __ReluConstraint_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
