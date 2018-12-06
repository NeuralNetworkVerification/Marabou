/*********************                                                        */
/*! \file SparseTableauRow.cpp
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#include "Debug.h"
#include "ReluplexError.h"
#include "SparseTableauRow.h"

SparseTableauRow::SparseTableauRow( unsigned dimension )
    : _dimension( dimension )
{
}

SparseTableauRow::~SparseTableauRow()
{
}

void SparseTableauRow::append( unsigned var, double coefficient )
{
    ASSERT( var < _dimension );

    if ( FloatUtils::isZero( coefficient ) )
        return;

    _entries.append( Entry( var, coefficient ) );
}

unsigned SparseTableauRow::getNnz() const
{
    return _entries.size();
}

bool SparseTableauRow::empty() const
{
    return _entries.empty();
}

List<SparseTableauRow::Entry>::const_iterator SparseTableauRow::begin() const
{
    return _entries.begin();
}

List<SparseTableauRow::Entry>::const_iterator SparseTableauRow::end() const
{
    return _entries.end();
}

List<SparseTableauRow::Entry>::iterator SparseTableauRow::begin()
{
    return _entries.begin();
}

List<SparseTableauRow::Entry>::iterator SparseTableauRow::end()
{
    return _entries.end();
}

void SparseTableauRow::clear()
{
    _entries.clear();
}

void SparseTableauRow::dump() const
{
    for ( const auto &entry : _entries )
    {
        printf( "%.2lf * x%u, ", entry._coefficient, entry._var );
    }

    printf( "\n\tscalar = %.2lf\n", _scalar );
    printf( "\tlhs = x%u\n", _lhs );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
