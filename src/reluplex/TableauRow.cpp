/*********************                                                        */
/*! \file Tableau.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#include "ReluplexError.h"
#include "TableauRow.h"

TableauRow::TableauRow( unsigned size )
    : _size( size )
{
    _row = new TableauRow::Entry[size];
    if ( !_row )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "TableauRow::row" );
}

TableauRow::~TableauRow()
{
    delete[] _row;
}

void TableauRow::dump() const
{
    for ( unsigned i = 0; i < _size; ++i )
    {
        if ( FloatUtils::isZero( _row[i]._coefficient ) )
            continue;

        printf( "%.15lf * x%u, ", _row[i]._coefficient, _row[i]._var );
    }
    printf( "\n" );
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
