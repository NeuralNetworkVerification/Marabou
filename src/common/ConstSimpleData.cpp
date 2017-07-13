/*********************                                                        */
/*! \file ConstSimpleData.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#include "ConstSimpleData.h"
#include "HeapData.h"
#include "MStringf.h"

ConstSimpleData::ConstSimpleData( const void *data, unsigned size ) : _data( data ), _size( size )
{
}

ConstSimpleData::ConstSimpleData( const HeapData &data ) : _data( data.data() ), _size( data.size() )
{
}

const void *ConstSimpleData::data() const
{
    return _data;
}

unsigned ConstSimpleData::size() const
{
    return _size;
}

const char *ConstSimpleData::asChar() const
{
    return (char *)_data;
}

void ConstSimpleData::hexDump() const
{
    for ( unsigned i = 0; i < size(); ++i )
    {
        printf( "%02x ", (unsigned char)asChar()[i] );
    }
    printf( "\n" );
    fflush( 0 );
}

String ConstSimpleData::toString() const
{
    String result;

    for ( unsigned i = 0; i < size(); ++i )
        result += Stringf( "%02x ", (unsigned char)asChar()[i] );

    return result;
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
