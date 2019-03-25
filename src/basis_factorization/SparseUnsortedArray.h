/*********************                                                        */
/*! \file SparseUnsortedArray.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifndef __SparseUnsortedArray_h__
#define __SparseUnsortedArray_h__

#include "HashMap.h"

class SparseUnsortedList;

class SparseUnsortedArray
{
public:
    struct Entry
    {
        Entry()
            : _index( 0 )
            , _value( 0 )
        {
        }

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
    */
    SparseUnsortedArray();
    ~SparseUnsortedArray();
    SparseUnsortedArray( unsigned size );
    SparseUnsortedArray( const double *V, unsigned size );
    void initialize( const double *V, unsigned size );
    void initializeToEmpty();
    void initializeFromList( const SparseUnsortedList *list );

    /*
      Remove the elements, without changing the allocated memory
    */
    void clear();

    /*
      Set a value.
      Call "append" only if certain that the value is not zero and
      that the index does not already exist in the sparse array.
    */
    void set( unsigned index, double value );
    void append( unsigned index, double value );

    /*
      The number of non-zero elements in the unsortedList
    */
    unsigned getNnz() const;
    bool empty() const;

    /*
      Retrieve an element
    */
    double get( unsigned entry ) const;
    Entry getByArrayIndex( unsigned index ) const;

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
    SparseUnsortedArray &operator=( const SparseUnsortedArray &other );
    void storeIntoOther( SparseUnsortedArray *other ) const;

    /*
      Erasing an element by its index in the underlying array
    */
    void erase( unsigned index );

    /*
      Addes the coefficient for entry 'source' to entry 'target'
      and erases entry 'source'
    */
    void mergeEntries( unsigned source, unsigned target );

    /*
      Debugging
    */
    void dump() const;
    void dumpDense() const;

private:
    Entry *_array;
    unsigned _maxSize;
    unsigned _allocatedSize;
    unsigned _nnz;

    // The chunk size by which the capacity is increased when exceeded
    enum {
        CHUNK_SIZE = 20,
    };

    void freeMemoryIfNeeded();
    void increaseCapacity();
};

#endif // __SparseUnsortedArray_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
