/*********************                                                        */
/*! \file TableauRow.cpp
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
    if ( _row )
    {
        delete[] _row;
        _row = NULL;
    }
}

double TableauRow::operator[]( unsigned index ) const
{
    return _row[index]._coefficient;
}

void TableauRow::dump() const
{
    for ( unsigned i = 0; i < _size; ++i )
    {
        if ( FloatUtils::isZero( _row[i]._coefficient ) )
            continue;

        printf( "%.2lf * x%u, ", _row[i]._coefficient, _row[i]._var );
    }

    printf( "\n\tscalar = %.2lf\n", _scalar );
    printf( "\tlhs = x%u\n", _lhs );
}

TableauRow &TableauRow::operator=( const TableauRow &other )
{
    printf( "Row equality operator called\n" );

    if ( _size != other._size )
    {
        if ( _row )
        {
            delete[] _row;
            _row = NULL;
        }

        _row = new TableauRow::Entry[other._size];
        if ( !_row )
            throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "TableauRow::row" );
    }

    _size = other._size;
    _scalar = other._scalar;
    _lhs = other._lhs;

    for  ( unsigned i = 0; i < _size; ++i )
        _row[i] = other._row[i];

    return *this;
}

bool TableauRow::emptyRow() const
{
    for ( unsigned i = 0; i < _size; ++i )
    {
        if ( !FloatUtils::isZero( _row[i]._coefficient ) )
            return false;
    }

    return true;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
