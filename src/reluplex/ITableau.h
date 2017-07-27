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

class EntrySelectionStrategy;
class Equation;
class Statistics;
class TableauRow;
class TableauState;

class ITableau
{
public:
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
        virtual void notifyVariableValue( unsigned variable, double value ) = 0;

        /*
          These callbacks will be invoked when the variable's
          lower/upper bounds change.
        */
        virtual void notifyLowerBound( unsigned variable, double bound ) = 0;
        virtual void notifyUpperBound( unsigned variable, double bound ) = 0;
    };

    virtual void registerToWatchVariable( VariableWatcher *watcher, unsigned variable ) = 0;
    virtual void unregisterToWatchVariable( VariableWatcher *watcher, unsigned variable ) = 0;

    virtual ~ITableau() {};

    virtual void setDimensions( unsigned m, unsigned n ) = 0;
    virtual void setEntryValue( unsigned row, unsigned column, double value ) = 0;
    virtual void setRightHandSide( const double *b ) = 0;
    virtual void setRightHandSide( unsigned index, double value ) = 0;
    virtual void markAsBasic( unsigned variable ) = 0;
    virtual void initializeTableau() = 0;
    virtual double getValue( unsigned variable ) = 0;
    virtual bool boundsValid( unsigned variable ) const = 0;
    virtual double getLowerBound( unsigned variable ) const = 0;
    virtual double getUpperBound( unsigned variable ) const = 0;
    virtual void setLowerBound( unsigned variable, double value ) = 0;
    virtual void setUpperBound( unsigned variable, double value ) = 0;
    virtual void tightenLowerBound( unsigned variable, double value ) = 0;
    virtual void tightenUpperBound( unsigned variable, double value ) = 0;
    virtual unsigned getBasicStatus( unsigned basic ) = 0;
    virtual bool existsBasicOutOfBounds() const = 0;
    virtual void setEnteringVariable( unsigned nonBasic ) = 0;
    virtual void computeBasicStatus() = 0;
    virtual void computeBasicStatus( unsigned basic ) = 0;
    virtual bool eligibleForEntry( unsigned nonBasic ) const = 0;
    virtual unsigned getEnteringVariable() const = 0;
    virtual void pickLeavingVariable() = 0;
    virtual void pickLeavingVariable( double *d ) = 0;
    virtual unsigned getLeavingVariable() const = 0;
    virtual double getChangeRatio() const = 0;
    virtual void performPivot() = 0;
    virtual double ratioConstraintPerBasic( unsigned basicIndex, double coefficient, bool decrease ) = 0;
    virtual bool isBasic( unsigned variable ) const = 0;
    virtual void setNonBasicAssignment( unsigned variable, double value ) = 0;
    virtual void computeCostFunction() = 0;
    virtual void getEntryCandidates( List<unsigned> &candidates ) const = 0;
    virtual void computeMultipliers() = 0;
    virtual void computeReducedCost( unsigned nonBasic ) = 0;
    virtual const double *getCostFunction() const = 0;
    virtual void dumpCostFunction() const = 0;
    virtual void computeD() = 0;
    virtual void computeAssignment() = 0;
    virtual void dump() const = 0;
    virtual void dumpAssignment() = 0;
    virtual void dumpEquations() = 0;
    virtual unsigned nonBasicIndexToVariable( unsigned index ) const = 0;
    virtual unsigned variableToIndex( unsigned index ) const = 0;
    virtual void addEquation( const Equation &equation ) = 0;
    virtual unsigned getM() const = 0;
    virtual unsigned getN() const = 0;
    virtual void getTableauRow( unsigned index, TableauRow *row ) = 0;
    virtual void performDegeneratePivot( unsigned entering, unsigned leaving ) = 0;
    virtual void storeState( TableauState &state ) const = 0;
    virtual void restoreState( const TableauState &state ) = 0;
    virtual void computeBasicCosts() = 0;
    virtual void setStatistics( Statistics *statistics ) = 0;
};

#endif // __ITableau_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
