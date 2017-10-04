/*********************                                                        */
/*! \file TableauRow.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __TableauRow_h__
#define __TableauRow_h__

#include "FloatUtils.h"

#include <cstdio>

class TableauRow
{
public:
    /*
      For a given vasic variable xb, a row is the explicit
      representation as non basic variables:

      xb = scalar + sum_i{ c_i * x_i }

      The basic variable is stored as lhs.
    */

    TableauRow( unsigned size );
    ~TableauRow();

    struct Entry
    {
        Entry()
            : _var( 0 )
            , _coefficient( 0.0 )
        {
        }

        Entry( unsigned var, double coefficient )
            : _var( var )
            , _coefficient( coefficient )
        {
        }

        unsigned _var;
        double _coefficient;
    };

    unsigned _size;
    Entry *_row;
    double _scalar;
    unsigned _lhs;

    double operator[]( unsigned index ) const;

    void dump() const;
};

#endif // __TableauRow_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
