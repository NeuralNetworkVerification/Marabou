/*********************                                                        */
/*! \file BoundExplainer.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Omri Isac, Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2022 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]
 **/

#ifndef __BoundsExplainer_h__
#define __BoundsExplainer_h__

#include "SparseUnsortedList.h"
#include "TableauRow.h"
#include "Vector.h"
#include "context/cdo.h"
#include "context/context.h"

/*
  A class which encapsulates bounds explanations of all variables of a tableau
*/
class BoundExplainer
{
public:
    BoundExplainer( unsigned numberOfVariables,
                    unsigned numberOfRows,
                    CVC4::context::Context &ctx );
    ~BoundExplainer();

    BoundExplainer &operator=( const BoundExplainer &other );
    /*
      Returns the number of rows
     */
    unsigned getNumberOfRows() const;

    /*
      Returns the number of variables
    */
    unsigned getNumberOfVariables() const;

    /*
      Returns a bound explanation
    */
    const SparseUnsortedList &getExplanation( unsigned var, bool isUpper );

    /*
      Given a row, updates the values of the bound explanations of its lhs according to the row
    */
    void updateBoundExplanation( const TableauRow &row, bool isUpper );

    /*
      Given a row, updates the values of the bound explanations of a var according to the row
    */
    void updateBoundExplanation( const TableauRow &row, bool isUpper, unsigned var );

    /*
      Given a row as SparseUnsortedList, updates the values of the bound explanations of a var
      according to the row
    */
    void updateBoundExplanationSparse( const SparseUnsortedList &row, bool isUpper, unsigned var );

    /*
      Adds a zero explanation at the end, and a zero entry to all explanations
     */
    void addVariable();

    /*
      Resets an explanation (to be a zero vec, represented by an empty vec)
     */
    void resetExplanation( unsigned var, bool isUpper );

    /*
      Updates an explanation, without necessarily using the recursive rule
     */
    void setExplanation( const Vector<double> &explanation, unsigned var, bool isUpper );

    void setExplanation( const SparseUnsortedList &explanation, unsigned var, bool isUpper );

    /*
     * Returns true iff an explanation is the zero vector
     */
    bool isExplanationTrivial( unsigned var, bool isUpper ) const;

private:
    CVC4::context::Context &_context;

    unsigned _numberOfVariables;
    unsigned _numberOfRows;

    Vector<CVC4::context::CDO<SparseUnsortedList> *> _upperBoundExplanations;
    Vector<CVC4::context::CDO<SparseUnsortedList> *> _lowerBoundExplanations;

    Vector<CVC4::context::CDO<bool> *> _trivialUpperBoundExplanation;
    Vector<CVC4::context::CDO<bool> *> _trivialLowerBoundExplanation;

    /*
      Adds a multiplication of an array by scalar to another array
    */
    void
    addVecTimesScalar( Vector<double> &sum, const SparseUnsortedList &input, double scalar ) const;

    void addVecTimesScalar( Vector<double> &sum, const Vector<double> &input, double scalar ) const;

    /*
      Upon receiving a row, extract coefficients of the original tableau's equations that create the
      row Equivalently, extract the coefficients of the slack variables. Assumption - the slack
      variables indices are always the last m. All coefficients are divided by ci, the coefficient
      of the explained var, for normalization.
    */
    void
    extractRowCoefficients( const TableauRow &row, Vector<double> &coefficients, double ci ) const;

    /*
      Upon receiving a row given as a SparseUnsortedList, extract coefficients of the original
      tableau's equations that create the row Equivalently, extract the coefficients of the slack
      variables. Assumption - the slack variables indices are always the last m. All coefficients
      are divided by ci, the coefficient of the explained var, for normalization.
    */
    void extractSparseRowCoefficients( const SparseUnsortedList &row,
                                       Vector<double> &coefficients,
                                       double ci ) const;
};
#endif // __BoundsExplainer_h__
