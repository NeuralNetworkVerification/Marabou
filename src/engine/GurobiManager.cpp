/*********************                                                        */
/*! \file GurobiManager.cpp
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

#include "GurobiManager.h"

GurobiManager::GurobiManager()
    : _tableau( NULL )
{
}

GurobiManager::~GurobiManager()
{
}

void GurobiManager::setTableau(ITableau *tableau)
{
    _tableau = tableau;
}

void GurobiManager::tightenBoundsOfVar(int objectiveVar) {
    printf( "---------------------------\n" );
    printf( "Current bounds:\n" );
    printf( "Var: %d\n", objectiveVar );

    unsigned n = _tableau->getN();
    unsigned m = _tableau->getM();
    for (unsigned i = 0; i < n; ++i) {
        double lb, ub;
        lb = _tableau->getLowerBound( i );
        ub = _tableau->getUpperBound( i );
        printf( "\tx%u: [%.2f, %.2f]\n", i, lb, ub );
    }

    for (unsigned i = 0; i < m; ++i) {
        TableauRow row( n );
        _tableau->getTableauRow( i, &row );

        unsigned lhs = row._lhs;
        printf( "\tx%u =", lhs );

        for ( unsigned j = 0; j <= n - m; ++j )
        {
            unsigned row_v = row._row[j]._var;
            double row_coef = row._row[j]._coefficient;

            if (row_coef > 0) {
                printf( " +%.2fx%u", row_coef, row_v );
            } else if (row_coef < 0) {
                printf( " %.2fx%u", row_coef, row_v );
            }
        }

        double row_scalar = row._scalar;
        if (row_scalar > 0) {
            printf( " +%.2f", row_scalar );
        } else if (row_scalar < 0) {
            printf( " %.2f", row_scalar );
        }

        printf("\n");
    }
    printf( "---------------------------\n" );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
