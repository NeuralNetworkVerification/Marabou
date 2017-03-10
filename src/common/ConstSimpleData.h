/*********************                                                        */
/*! \file ConstSimpleData.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Reluplex project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __ConstSimpleData_h__
#define __ConstSimpleData_h__

class IHeapData;
class String;

#include <cstdio>

class ConstSimpleData
{
public:
	ConstSimpleData( const void *data, unsigned size );
    ConstSimpleData( const IHeapData &data );
	const void *data() const;
    unsigned size() const;
    const char *asChar() const;
    void hexDump() const;
    String toString() const;

private:
	const void *_data;
	unsigned _size;
};

#endif // __ConstSimpleData_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
