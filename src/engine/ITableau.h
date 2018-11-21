/*********************                                                        */
/*! \file ITableau.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __ITableau_h__
#define __ITableau_h__

#include "List.h"
#include "Set.h"

class EntrySelectionStrategy;
class Equation;
class ICostFunctionManager;
class PiecewiseLinearCaseSplit;
class SparseMatrix;
class SparseUnsortedList;
class SparseVector;
class Statistics;
class TableauRow;
class TableauState;

class ITableau
{
public:
    enum BasicStatus {
        BELOW_LB = 0,
        BETWEEN = 1,
        ABOVE_UB = 2,
    };

    enum CostFunctionStatus {
        COST_FUNCTION_INVALID = 0,
        COST_FUNCTION_JUST_COMPUTED = 1,
        COST_FUNCTION_UPDATED = 2,
    };

    enum BasicAssignmentStatus {
        BASIC_ASSIGNMENT_INVALID = 0,
        BASIC_ASSIGNMENT_JUST_COMPUTED = 1,
        BASIC_ASSIGNMENT_UPDATED = 2,
    };

    /*
      A class for allowing objects (e.g., piecewise linear
      constraints) to register and receive updates regarding changes
      in variable assignments and variable bounds.
    */
    class VariableWatcher
    {
    public:
        /*
          This callback will be invoked when the variable's value
          changes.
        */
        virtual void notifyVariableValue( unsigned /* variable */, double /* value */ ) {}

        /*
          These callbacks will be invoked when the variable's
          lower/upper bounds change.
        */
        virtual void notifyLowerBound( unsigned /* variable */, double /* bound */ ) {}
        virtual void notifyUpperBound( unsigned /* variable */, double /* bound */ ) {}
    };

    class ResizeWatcher
    {
    public:
        /*
          This callback will be invoked when the tableau size changes,
          typically when new variables are added.
        */
        virtual void notifyDimensionChange( unsigned /* m */, unsigned /* n */ ) {}
    };

    virtual void registerToWatchAllVariables( VariableWatcher *watcher ) = 0;
    virtual void registerToWatchVariable( VariableWatcher *watcher, unsigned variable ) = 0;
    virtual void unregisterToWatchVariable( VariableWatcher *watcher, unsigned variable ) = 0;

    virtual void registerResizeWatcher( ResizeWatcher *watcher ) = 0;

    virtual void registerCostFunctionManager( ICostFunctionManager *costFunctionManager ) = 0;

    virtual ~ITableau() {};

    virtual void setDimensions( unsigned m, unsigned n ) = 0;
    virtual void setConstraintMatrix( const double *A ) = 0;
    virtual void setRightHandSide( const double *b ) = 0;
    virtual void setRightHandSide( unsigned index, double value ) = 0;
    virtual void markAsBasic( unsigned variable ) = 0;
    virtual void initializeTableau( const List<unsigned> &initialBasicVariables ) = 0;
    virtual double getValue( unsigned variable ) = 0;
    virtual bool allBoundsValid() const = 0;
    virtual double getLowerBound( unsigned variable ) const = 0;
    virtual double getUpperBound( unsigned variable ) const = 0;
    virtual const double *getLowerBounds() const = 0;
    virtual const double *getUpperBounds() const = 0;
    virtual void setLowerBound( unsigned variable, double value ) = 0;
    virtual void setUpperBound( unsigned variable, double value ) = 0;
    virtual void tightenLowerBound( unsigned variable, double value ) = 0;
    virtual void tightenUpperBound( unsigned variable, double value ) = 0;
    virtual unsigned getBasicStatus( unsigned basic ) = 0;
    virtual unsigned getBasicStatusByIndex( unsigned basicIndex ) = 0;
    virtual bool existsBasicOutOfBounds() const = 0;
    virtual void setEnteringVariableIndex( unsigned nonBasic ) = 0;
    virtual void setLeavingVariableIndex( unsigned basic ) = 0;
    virtual Set<unsigned> getBasicVariables() const = 0;
    virtual void computeBasicStatus() = 0;
    virtual void computeBasicStatus( unsigned basic ) = 0;
    virtual bool eligibleForEntry( unsigned nonBasic, const double *costFunction ) const = 0;
    virtual unsigned getEnteringVariable() const = 0;
    virtual unsigned getEnteringVariableIndex() const = 0;
    virtual void pickLeavingVariable() = 0;
    virtual void pickLeavingVariable( double *d ) = 0;
    virtual unsigned getLeavingVariable() const = 0;
    virtual unsigned getLeavingVariableIndex() const = 0;
    virtual double getChangeRatio() const = 0;
    virtual void setChangeRatio( double changeRatio ) = 0;
    virtual bool performingFakePivot() const = 0;
    virtual void performPivot() = 0;
    virtual double ratioConstraintPerBasic( unsigned basicIndex, double coefficient, bool decrease ) = 0;
    virtual bool isBasic( unsigned variable ) const = 0;
    virtual void setNonBasicAssignment( unsigned variable, double value, bool updateBasics ) = 0;
    virtual void computeCostFunction() = 0;
    virtual void getEntryCandidates( List<unsigned> &candidates ) const = 0;
    virtual const double *getCostFunction() const = 0;
    virtual void computeChangeColumn() = 0;
    virtual const double *getChangeColumn() const = 0;
    virtual void setChangeColumn( const double *column ) = 0;
    virtual void computePivotRow() = 0;
    virtual const TableauRow *getPivotRow() const = 0;
    virtual void computeAssignment() = 0;
    virtual void dump() const = 0;
    virtual void dumpAssignment() = 0;
    virtual void dumpEquations() = 0;
    virtual unsigned nonBasicIndexToVariable( unsigned index ) const = 0;
    virtual unsigned basicIndexToVariable( unsigned index ) const = 0;
    virtual void assignIndexToBasicVariable( unsigned variable, unsigned index ) = 0;
    virtual unsigned variableToIndex( unsigned index ) const = 0;
    virtual unsigned addEquation( const Equation &equation ) = 0;
    virtual unsigned getM() const = 0;
    virtual unsigned getN() const = 0;
    virtual void getTableauRow( unsigned index, TableauRow *row ) = 0;
    virtual const double *getAColumn( unsigned variable ) const = 0;
    virtual void getSparseAColumn( unsigned variable, SparseUnsortedList *result ) const = 0;
    virtual void getSparseARow( unsigned row, SparseUnsortedList *result ) const = 0;
    virtual const SparseUnsortedList *getSparseAColumn( unsigned variable ) const = 0;
    virtual const SparseUnsortedList *getSparseARow( unsigned row ) const = 0;
    virtual const SparseMatrix *getSparseA() const = 0;
    virtual void performDegeneratePivot() = 0;
    virtual void storeState( TableauState &state ) const = 0;
    virtual void restoreState( const TableauState &state ) = 0;
    virtual void setStatistics( Statistics *statistics ) = 0;
    virtual const double *getRightHandSide() const = 0;
    virtual void forwardTransformation( const double *y, double *x ) const = 0;
    virtual void backwardTransformation( const double *y, double *x ) const = 0;
    virtual double getSumOfInfeasibilities() const = 0;
    virtual BasicAssignmentStatus getBasicAssignmentStatus() const = 0;
    virtual double getBasicAssignment( unsigned basicIndex ) const = 0;
    virtual void setBasicAssignmentStatus( ITableau::BasicAssignmentStatus status ) = 0;
    virtual bool basicOutOfBounds( unsigned basic ) const = 0;
    virtual bool basicTooHigh( unsigned basic ) const = 0;
    virtual bool basicTooLow( unsigned basic ) const = 0;
    virtual void verifyInvariants() = 0;
    virtual bool basisMatrixAvailable() const = 0;
    virtual double *getInverseBasisMatrix() const = 0;
    virtual void refreshBasisFactorization() = 0;
    virtual void mergeColumns( unsigned x1, unsigned x2 ) = 0;
    virtual unsigned getVariableAfterMerging( unsigned variable ) const = 0;
};

#endif // __ITableau_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
