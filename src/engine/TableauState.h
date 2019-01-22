/*********************                                                        */
/*! \file TableauState.h
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

#ifndef __TableauState_h__
#define __TableauState_h__

#include "IBasisFactorization.h"
#include "ITableau.h"
#include "Map.h"
#include "Set.h"
#include "SparseMatrix.h"

class TableauState
{
    /*
      A Tableu state includes the following elements:

      - Tableau dimensions
      - The matrix A
      - The right hand side vector b
      - Lower and upper bounds
      - Basic variables
      - Basic and non-basic assignments
      - Basic assignment status
      - The current indexing
      - The current basis
    */
public:
    TableauState();
    ~TableauState();

    void setDimensions( unsigned m, unsigned n, const IBasisFactorization::BasisColumnOracle &oracle );

    /*
      The dimensions of matrix A
    */
    unsigned _m;
    unsigned _n;

    /*
      The matrix
    */
    SparseMatrix *_A;
    SparseUnsortedList **_sparseColumnsOfA;
    SparseUnsortedList **_sparseRowsOfA;
    double *_denseA;

    /*
      The right hand side
    */
    double *_b;

    /*
      Upper and lower bounds for all variables
    */
    double *_lowerBounds;
    double *_upperBounds;

    /*
      The set of current basic variables
    */
    Set<unsigned> _basicVariables;

    /*
      The current assignment for the basic variables
    */
    double *_basicAssignment;

    /*
      The assignment of the non basic variables.
    */
    double *_nonBasicAssignment;

    /*
      The status of the basic assignment.
    */
    ITableau::BasicAssignmentStatus _basicAssignmentStatus;

    /*
      Mapping between basic variables and indices (length m)
    */
    unsigned *_basicIndexToVariable;

    /*
      Mapping between non-basic variables and indices (length n - m)
    */
    unsigned *_nonBasicIndexToVariable;

    /*
      Mapping from variable to index, either basic or non-basic
    */
    unsigned *_variableToIndex;

    /*
      The factorization of the basis
    */
    IBasisFactorization *_basisFactorization;

    /*
      Indicator whether the bounds are valid
    */
    bool _boundsValid;

    /*
      _mergedVariables[x] = y means that x = y, and that
      variable x has been merged into variable y. So, when
      extracting a solution for x, we should read the value of y.
     */
    Map<unsigned, unsigned> _mergedVariables;
};

#endif // __TableauState_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
