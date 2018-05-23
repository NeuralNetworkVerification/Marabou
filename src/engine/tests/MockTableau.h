/*********************                                                        */
/*! \file MockTableau.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __MockTableau_h__
#define __MockTableau_h__

#include "FloatUtils.h"
#include "ITableau.h"
#include "Map.h"
#include "TableauRow.h"

#include <cstring>

class MockTableau : public ITableau
{
public:
	MockTableau()
	{
		wasCreated = false;
		wasDiscarded = false;

        setDimensionsCalled = false;
        lastRightHandSide = NULL;
        initializeTableauCalled = false;

        lastBtranInput = NULL;
        nextBtranOutput = NULL;

        lastEntries = NULL ;
        nextCostFunction = NULL;

        lastCostFunctionManager = NULL;
    }

    ~MockTableau()
    {
        if ( lastRightHandSide )
        {
            delete[] lastRightHandSide;
            lastRightHandSide = NULL;
        }

        if ( lastBtranInput )
        {
            delete[] lastBtranInput;
            lastBtranInput = NULL;
        }

        if ( nextBtranOutput )
        {
            delete[] nextBtranOutput;
            nextBtranOutput = NULL;
        }

        if ( lastEntries )
        {
            delete[] lastEntries;
            lastEntries = NULL;
        }

        if ( nextCostFunction )
        {
            delete[] nextCostFunction;
            nextCostFunction = NULL;
        }
    }

	bool wasCreated;
	bool wasDiscarded;

    List<unsigned> mockCandidates;
    unsigned mockEnteringVariable;
    void getEntryCandidates( List<unsigned> &candidates ) const
    {
        candidates = mockCandidates;
    }

    void setEnteringVariableIndex( unsigned nonBasic )
    {
        mockEnteringVariable = nonBasic;
    }

	void mockConstructor()
	{
		TS_ASSERT( !wasCreated );
		wasCreated = true;
	}

	void mockDestructor()
	{
		TS_ASSERT( wasCreated );
		TS_ASSERT( !wasDiscarded );
		wasDiscarded = true;
	}

    bool setDimensionsCalled;
    unsigned lastM;
    unsigned lastN;
    void setDimensions( unsigned m, unsigned n )
    {
        TS_ASSERT( !initializeTableauCalled );
        setDimensionsCalled = true;
        lastRightHandSide = new double[m];
        lastM = m;
        lastN = n;
        lastEntries = new double[m*n];
        std::fill( lastEntries, lastEntries + ( n * m ), 0.0 );

        nextCostFunction = new double[n - m];
        std::fill( nextCostFunction, nextCostFunction + ( n - m ), 0.0 );

        lastBtranInput = new double[m];
        nextBtranOutput = new double[m];
    }

    double *lastEntries;
    void setEntryValue( unsigned row, unsigned column, double value )
    {
        TS_ASSERT( setDimensionsCalled );
        lastEntries[(row * lastN) + column] = value;
    }

    double *lastRightHandSide;
    void setRightHandSide( const double * b )
    {
        TS_ASSERT( setDimensionsCalled );
        TS_ASSERT( !initializeTableauCalled );
        memcpy( lastRightHandSide, b, sizeof(double) * lastM );
    }

    void setRightHandSide( unsigned index, double value )
    {
        TS_ASSERT( setDimensionsCalled );
        TS_ASSERT( !initializeTableauCalled );
        lastRightHandSide[index] = value;
    }

    Set<unsigned> lastBasicVariables;
    void markAsBasic( unsigned variable )
    {
        TS_ASSERT( !initializeTableauCalled );
        lastBasicVariables.insert( variable );
    }

    bool initializeTableauCalled;
    void initializeTableau( const List<unsigned> &initialBasicVariables )
    {
        TS_ASSERT( !initializeTableauCalled );
        initializeTableauCalled = true;

        for ( const auto &basic : initialBasicVariables )
            lastBasicVariables.insert( basic );
    }

    Map<unsigned, double> nextValues;
    double getValue( unsigned variable )
    {
        TS_ASSERT( nextValues.exists( variable ) );
        return nextValues[variable];
    }

    Map<unsigned, double> lowerBounds;
    double getLowerBound( unsigned variable ) const
    {
        return lowerBounds[variable];
    }

    void setLowerBound( unsigned variable, double value )
    {
        lowerBounds[variable] = value;
    }

    Map<unsigned, double> upperBounds;
    double getUpperBound( unsigned variable ) const
    {
        return upperBounds[variable];
    }

    void setUpperBound( unsigned variable, double value )
    {
        upperBounds[variable] = value;
    }

    const double *getLowerBounds() const
    {
        return NULL;
    }

    const double *getUpperBounds() const
    {
        return NULL;
    }

    bool allBoundsValid() const
    {
        for ( auto it : lowerBounds.keys() )
        {
            if ( lowerBounds.exists(it) &&
                !FloatUtils::lte( lowerBounds[it], upperBounds[it] ) )
                    return false;
        }
        return true;
    }

    unsigned getBasicStatus( unsigned /* basic */ ) { return 0; }
    unsigned getBasicStatusByIndex( unsigned /* basicIndex */ ) { return 0; }

    bool existsBasicOutOfBounds() const { return false; }
    void computeBasicStatus() {}
    void computeBasicStatus( unsigned /* basic */ ) {}
    bool pickEnteringVariable( EntrySelectionStrategy */* strategy */ ) { return false; }
    bool eligibleForEntry( unsigned nonBasic, const double */* costFunction */ ) const
    {
        return mockCandidates.exists( nonBasic );
    }

    unsigned nextEnteringVariable;
    unsigned getEnteringVariable() const
    {
        return nextEnteringVariable;
    }

    unsigned nextEnteringVariableIndex;
    unsigned getEnteringVariableIndex() const
    {
        return nextEnteringVariableIndex;
    }

    void pickLeavingVariable() {};
    void pickLeavingVariable( double */* d */ ) {}

    unsigned mockLeavingVariable;
    void setLeavingVariableIndex( unsigned basic )
    {
        mockLeavingVariable = basic;
    }

    Set<unsigned> getBasicVariables() const
    {
        return Set<unsigned>();
    }

    unsigned getLeavingVariable() const
    {
        return mockLeavingVariable;
    }

    unsigned getLeavingVariableIndex() const
    {
        return mockLeavingVariable;
    }

    double getChangeRatio() const { return 0; }
    void performPivot() {}
    bool performingFakePivot() const
    {
        return false;
    }

    double ratioConstraintPerBasic( unsigned /* basicIndex */, double /* coefficient */, bool /* decrease */ ) { return 0;}

    Set<unsigned> nextIsBasic;
    bool isBasic( unsigned variable ) const
    {
        return nextIsBasic.exists( variable );
    }

    void setNonBasicAssignment( unsigned /* variable */, double /* value */, bool /* updateBasics */ ) {}
    void computeCostFunction() {}
    void computeReducedCost (unsigned /* variable */) {}

    double *nextCostFunction;
    const double *getCostFunction() const
    {
        return nextCostFunction;
    }

    BasicAssignmentStatus getBasicAssignmentStatus() const
    {
        return BASIC_ASSIGNMENT_INVALID;
    }

    void setBasicAssignmentStatus( ITableau::BasicAssignmentStatus /* status */ )
    {
    }

    Set<unsigned> nextBasicTooHigh;
    Set<unsigned> nextBasicTooLow;

    bool basicOutOfBounds( unsigned basic ) const
    {
        return basicTooLow( basic ) || basicTooHigh( basic );
    }

    bool basicTooHigh( unsigned basic ) const
    {
        return nextBasicTooHigh.exists( basic );
    }

    bool basicTooLow( unsigned basic ) const
    {
        return nextBasicTooLow.exists( basic );
    }

    void computeChangeColumn() {}

    double *nextChangeColumn;
    const double *getChangeColumn() const
    {
        return nextChangeColumn;
    }

    void setChangeColumn( const double * )
    {
    }

    TableauRow *nextPivotRow;
    void computePivotRow()
    {
        getTableauRow( mockLeavingVariable, nextPivotRow );
    }

    const TableauRow *getPivotRow() const
    {
        return nextPivotRow;
    }

    void computeAssignment() {}
    void dump() const {}
    void dumpAssignment() {}
    void dumpEquations() {}

    Map<unsigned, unsigned> nextNonBasicIndexToVariable;
    unsigned nonBasicIndexToVariable( unsigned index ) const
    {
        TS_ASSERT( nextNonBasicIndexToVariable.exists( index ) );
        return nextNonBasicIndexToVariable.get( index );
    }

    Map<unsigned, unsigned> nextBasicIndexToVariable;
    unsigned basicIndexToVariable( unsigned index ) const
    {
        TS_ASSERT( nextBasicIndexToVariable.exists( index ) );
        return nextBasicIndexToVariable.get( index );
    }

    Map<unsigned, unsigned> nextVariableToIndex;
    unsigned variableToIndex( unsigned index ) const
    {
        TS_ASSERT( nextVariableToIndex.exists( index ) );
        return nextVariableToIndex.at( index );
    }

    void addEquation( const Equation &/* equation */ )
    {
    }

    unsigned getM() const
    {
        return lastM;
    }

    unsigned getN() const
    {
        return lastN;
    }

    unsigned lastGetRowIndex;
    TableauRow *nextRow;
    void getTableauRow( unsigned index, TableauRow *row )
    {
        lastGetRowIndex = index;
        TS_ASSERT_EQUALS( row->_size, nextRow->_size );

        for ( unsigned i = 0; i < row->_size; ++i )
            row->_row[i] = nextRow->_row[i];
        row->_scalar = nextRow->_scalar;
    }

    Map<unsigned, const double *> nextAColumn;
    const double *getAColumn( unsigned index ) const
    {
        TS_ASSERT( nextAColumn.exists( index ) );
        TS_ASSERT( nextAColumn.get( index ) );
        return nextAColumn.get( index );
    }

    double *A;
    const double *getA() const
    {
        return A;
    }

    void performDegeneratePivot()
    {
    }

    void storeState( TableauState &/* state */ ) const
    {
    }

    void restoreState( const TableauState &/* state */ )
    {
    }

    Map<unsigned, double> tightenedLowerBounds;
    void tightenLowerBound( unsigned variable, double value )
    {
        tightenedLowerBounds[variable] = value;
    }

    Map<unsigned, double> tightenedUpperBounds;
    void tightenUpperBound( unsigned variable, double value )
    {
        tightenedUpperBounds[variable] = value;
    }

    void applySplit( const PiecewiseLinearCaseSplit &/* split */)
    {
    }

    void registerToWatchAllVariables( VariableWatcher */* watcher */ )
    {
    }

    Set<ResizeWatcher *> lastResizeWatchers;
    void registerResizeWatcher( ResizeWatcher *watcher )
    {
        lastResizeWatchers.insert( watcher );
    }

    ICostFunctionManager *lastCostFunctionManager;
    void registerCostFunctionManager( ICostFunctionManager *costFunctionManager )
    {
        lastCostFunctionManager = costFunctionManager;
    }

    typedef List<VariableWatcher *> Watchers;
    Map<unsigned, Watchers> lastRegisteredVariableToWatcher;
    void registerToWatchVariable( VariableWatcher *watcher, unsigned variable )
    {
        lastRegisteredVariableToWatcher[variable].append( watcher );
    }

    Map<unsigned, Watchers> lastUnregisteredVariableToWatcher;
    void unregisterToWatchVariable( VariableWatcher *watcher, unsigned variable )
    {
        lastUnregisteredVariableToWatcher[variable].append( watcher );
    }

    void computeBasicCosts()
    {
    }

    void setStatistics( Statistics */* statistics */ )
    {
    }

    double *b;
    const double *getRightHandSide() const
    {
        return b;
    }

    void forwardTransformation( const double *, double * ) const {}

    mutable double *lastBtranInput;
    double *nextBtranOutput;
    void backwardTransformation( const double *input, double *output ) const
    {
        memcpy( lastBtranInput, input, lastM * sizeof(double) );
        memcpy( output, nextBtranOutput, lastM * sizeof(double) );
    }

    double getSumOfInfeasibilities() const
    {
        return 0;
    }

    void assignIndexToBasicVariable( unsigned /* variable */, unsigned /* index */ )
    {
    }

    void verifyInvariants()
    {
    }

    bool basisMatrixAvailable() const
    {
        return true;
    }

    void getBasisEquations( List<Equation *> &/* equations */ ) const
    {
    }

    Equation *getBasisEquation( unsigned /* row */ ) const
    {
        return NULL;
    }

    double *getInverseBasisMatrix() const
    {
        return NULL;
    }
};

#endif // __MockTableau_h__

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
