/*********************                                                        */
/*! \file HeapData.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __HeapData_h__
#define __HeapData_h__

class ConstSimpleData;

class HeapData
{
public:
	HeapData();
	HeapData( void *data, unsigned size );
    HeapData( HeapData &other );
    HeapData( const ConstSimpleData &constSimpleData );

    virtual ~HeapData();

    HeapData( const HeapData &other );

	HeapData &operator=( const ConstSimpleData &data );
	HeapData &operator=( const HeapData &other );
    HeapData &operator+=( const ConstSimpleData &data );
    HeapData &operator+=( const HeapData &data );
    bool operator==( const HeapData &other ) const;
    bool operator!=( const HeapData &other ) const;
    bool operator<( const HeapData &other ) const;
	void *data();
	const void *data() const;
	unsigned size() const;
    void clear();
    const char *asChar() const;

private:
	void *_data;
	unsigned _size;

	bool allocated() const;
	void freeMemory();
	void freeMemoryIfNeeded();
	void allocateMemory( unsigned size );
    void addMemory( unsigned size );
    void copyNewData( const void *newData, unsigned size );
    void adjustSize( unsigned size );
};

#endif // __HeapData_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
