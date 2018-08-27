/*********************                                                        */
/*! \file SparseUnsortedList.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __SparseUnsortedList_h__
#define __SparseUnsortedList_h__

#include "HashMap.h"
#include "SparseMatrix.h"

class SparseUnsortedList
{
public:
    struct Entry
    {
        Entry( unsigned index, double value )
            : _index( index )
            , _value( value )
        {
        }

        unsigned _index;
        double _value;
    };

    /*
      Initialization: the size determines the dimension of the
      underlying storage.

      A unsortedList can be initialized from a dense unsortedList, or it
      can remain empty.
    */
    SparseUnsortedList();
    ~SparseUnsortedList();
    SparseUnsortedList( unsigned size );
    SparseUnsortedList( const double *V, unsigned size );
    void initialize( const double *V, unsigned size );
    void initializeToEmpty();

    /*
      Remove the unsortedList's elements, without touching the
      allocated memory
    */
    void clear();

    /*
      Set a value
    */
    void set( unsigned index, double value );

    /*
      The number of non-zero elements in the unsortedList
    */
    unsigned getNnz() const;
    bool empty() const;

    /*
      Retrieve an element
    */
    double get( unsigned entry ) const;

    /*
      Convert the unsortedList to dense format
    */
    void toDense( double *result ) const;

    /*
      Increase list size and add new entry
    */
    void addLastEntry( double entry );
    void incrementSize();

    /*
      Cloning
    */
    SparseUnsortedList &operator=( const SparseUnsortedList &other );
    void storeIntoOther( SparseUnsortedList *other ) const;

    /*
      Retrieve entries
    */
    List<Entry>::const_iterator begin() const;
    List<Entry>::const_iterator end() const;
    List<Entry>::iterator begin();
    List<Entry>::iterator end();

    /*
      Debugging
    */
    void dump() const;
    void dumpDense() const;

private:
    unsigned _size;
    List<Entry> _list;
};

#endif // __SparseUnsortedList_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
