/*********************                                                        */
/*! \file SparseTableauRow.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __SparseTableauRow_h__
#define __SparseTableauRow_h__

#include "FloatUtils.h"

#include <cstdio>

class SparseTableauRow
{
public:
    /*
      For a given vasic variable xb, a row is the explicit
      representation as non basic variables:

      xb = scalar + sum_i{ c_i * x_i }

      The basic variable is stored as lhs.
    */

    SparseTableauRow( unsigned dimension );
    ~SparseTableauRow();

    struct Entry
    {
        Entry()
            : _index( 0 )
            , _variable( 0 )
            , _coefficient( 0.0 )
        {
        }

        Entry( unsigned index, unsigned variable, double coefficient )
            : _index( index )
            , _variable( variable )
            , _coefficient( coefficient )
        {
        }

        // The index of the non-basic variable
        unsigned _index;

        // The actual non-basic variable
        unsigned _variable;

        // The coefficient
        double _coefficient;
    };

    /*
      Insert an element, assuming it does not exist already in the row
    */
    void append( unsigned index, unsigned variable, double coefficient );

    /*
      The number of non-zero elements in the row
    */
    unsigned getNnz() const;
    bool empty() const;

    /*
      Clear the stored row
    */
    void clear();

    /*
      Retrieve entries
    */
    List<Entry>::const_iterator begin() const;
    List<Entry>::const_iterator end() const;
    List<Entry>::iterator begin();
    List<Entry>::iterator end();

    unsigned _dimension;
    List<Entry> _entries;
    double _scalar;
    unsigned _lhs;

    void dump() const;
};

#endif // __SparseTableauRow_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
