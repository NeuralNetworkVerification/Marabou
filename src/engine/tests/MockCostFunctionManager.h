/*********************                                                        */
/*! \file MockCostFunctionManager.h
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

#ifndef __MockCostFunctionManager_h__
#define __MockCostFunctionManager_h__

#include "ICostFunctionManager.h"
#include "ITableau.h"

#include <cstring>

class MockCostFunctionManager : public ICostFunctionManager
{
public:
	MockCostFunctionManager()
	{
		wasCreated = false;
		wasDiscarded = false;

        initializeWasCalled = false;
        lastTableau = NULL;
        nextCostFunction = NULL;
        computeCoreCostFunctionCalled = false;
    }

    ~MockCostFunctionManager()
    {
        if ( nextCostFunction )
            delete[] nextCostFunction;
    }

	bool wasCreated;
	bool wasDiscarded;

    ITableau *lastTableau;
	void mockConstructor( ITableau *tableau )
	{
		TS_ASSERT( !wasCreated );
		wasCreated = true;

        lastTableau = tableau;
	}

	void mockDestructor()
	{
		TS_ASSERT( wasCreated );
		TS_ASSERT( !wasDiscarded );
		wasDiscarded = true;
	}

    bool initializeWasCalled;
    void initialize()
    {
        initializeWasCalled = true;
    }

    ICostFunctionManager::CostFunctionStatus getCostFunctionStatus() const
    {
        return ICostFunctionManager::COST_FUNCTION_INVALID;
    }

    void setCostFunctionStatus( ICostFunctionManager::CostFunctionStatus /* status */ )
    {
    }

    bool computeCoreCostFunctionCalled;
    void computeCoreCostFunction()
    {
        computeCoreCostFunctionCalled = true;
    }

    void computeCostFunction( const Map<unsigned, double> &/* heuristicCost */ )
    {
    }

    void computeGivenCostFunction( const Map<unsigned, double> &/* heuristicCost */ )
    {
    }

    double computeGivenCostFunctionDirectly( const Map<unsigned, double> &/* heuristicCost */ )
    {
        return 0;
    }

    double *nextCostFunction;
    const double *getCostFunction() const
    {
        return nextCostFunction;
    }

    void dumpCostFunction() const
    {
    }

    double updateCostFunctionForPivot( unsigned /* enteringVariableIndex */,
                                       unsigned /* leavingVariableIndex */,
                                       double /* pivotElement */,
                                       const TableauRow */* pivotRow */,
                                       const double */* changeColumn */
                                       )
    {
        return 0;
    }

    Map <unsigned, double> nextBasicCost;
    double getBasicCost( unsigned basicIndex ) const
    {
        TS_ASSERT( nextBasicCost.exists( basicIndex ) );
        return nextBasicCost[basicIndex];
    }

    void adjustBasicCostAccuracy()
    {
    }

    bool costFunctionInvalid() const
    {
        return true;
    }

    bool costFunctionJustComputed() const
    {
        return false;
    }

    void invalidateCostFunction()
    {
    }
};

#endif // __MockCostFunctionManager_h__

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
