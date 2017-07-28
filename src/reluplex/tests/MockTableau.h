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
	}

    ~MockTableau()
    {
        if ( lastRightHandSide )
        {
            delete[] lastRightHandSide;
            lastRightHandSide = NULL;
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

    void setEnteringVariable( unsigned nonBasic )
    {
        mockEnteringVariable = nonBasic;
    }

    unsigned getEnteringVariable()
    {
        return mockEnteringVariable;
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
    void initializeTableau()
    {
        TS_ASSERT( !initializeTableauCalled );
        initializeTableauCalled = true;
    }

    double getValue( unsigned /* variable */ ) { return 0; }

    Map<unsigned, double> lastLowerBounds;
    double getLowerBound( unsigned variable ) const
    {
        return lastLowerBounds[variable];
    }

    void setLowerBound( unsigned variable, double value )
    {
        lastLowerBounds[variable] = value;
    }

    Map<unsigned, double> lastUpperBounds;
    double getUpperBound( unsigned variable ) const
    {
        return lastUpperBounds[variable];
    }

    void setUpperBound( unsigned variable, double value )
    {
        lastUpperBounds[variable] = value;
    }

    bool allBoundsValid() const
    {
        for ( auto it : lastLowerBounds.keys() )
        {
            if ( lastUpperBounds.exists(it) &&
                !FloatUtils::lte( lastLowerBounds[it], lastUpperBounds[it] ) )
                    return false;
        }
        return true;
    }

    unsigned getBasicStatus( unsigned /* basic */ ) { return 0; }
    bool existsBasicOutOfBounds() const { return false; }
    void computeBasicStatus() {}
    void computeBasicStatus( unsigned /* basic */ ) {}
    bool pickEnteringVariable( EntrySelectionStrategy */* strategy */ ) { return false; }
    bool eligibleForEntry( unsigned nonBasic ) const
    {
        return mockCandidates.exists( nonBasic );
    }
    unsigned getEnteringVariable() const { return 0; }
    void pickLeavingVariable() {};
    void pickLeavingVariable( double */* d */ ) {}
    unsigned getLeavingVariable() const { return 0; }
    double getChangeRatio() const { return 0; }
    void performPivot() {}
    double ratioConstraintPerBasic( unsigned /* basicIndex */, double /* coefficient */, bool /* decrease */ ) { return 0;}
    bool isBasic( unsigned /* variable */ ) const { return false; }
    void setNonBasicAssignment( unsigned /* variable */, double /* value */ ) {}
    void computeCostFunction() {}
    void computeMultipliers() {}
    void computeReducedCost (unsigned /* variable */) {}

    double *nextCostFunction;
    const double *getCostFunction() const
    {
        return nextCostFunction;
    }

    void dumpCostFunction() const {}
    void computeD() {}
    void computeAssignment() {}
    void dump() const {}
    void dumpAssignment() {}
    void dumpEquations() {}

    Map<unsigned, unsigned> nextNonBasicIndexToVaribale;
    unsigned nonBasicIndexToVariable( unsigned index ) const
    {
        TS_ASSERT( nextNonBasicIndexToVaribale.exists( index ) );
        return nextNonBasicIndexToVaribale.get( index );
    }

    unsigned variableToIndex( unsigned /* index */ ) const
    {
        return 0;
    }

    void addEquation( const Equation &/* equation */ )
    {
    }

    unsigned getM() const
    {
        return 0;
    }

    unsigned getN() const
    {
        return 0;
    }

    void getTableauRow( unsigned /* index */, TableauRow */* row */ )
    {
    }

    void performDegeneratePivot( unsigned /* entering */, unsigned /* leaving */ )
    {
    }

    void storeState( TableauState &/* state */ ) const
    {
    }

    void restoreState( const TableauState &/* state */ )
    {
    }

    void tightenLowerBound( unsigned /* variable */, double /* value */ )
    {
    }

    void tightenUpperBound( unsigned /* variable */, double /* value */ )
    {
    }

    typedef Set<VariableWatcher *> Watchers;
    Map<unsigned, Watchers> lastRegisteredVariableToWatcher;
    void registerToWatchVariable( VariableWatcher *watcher, unsigned variable )
    {
        lastRegisteredVariableToWatcher[variable].insert( watcher );
    }

    Map<unsigned, Watchers> lastUnregisteredVariableToWatcher;
    void unregisterToWatchVariable( VariableWatcher *watcher, unsigned variable )
    {
        lastUnregisteredVariableToWatcher[variable].insert( watcher );
    }

    void computeBasicCosts()
    {
    }

    void setStatistics( Statistics */* statistics */ )
    {
    }
};

#endif // __MockTableau_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
