/*********************                                                        */
/*! \file CostFunctionManager.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "CostFunctionManager.h"
#include "Debug.h"
#include "FloatUtils.h"
#include "ITableau.h"
#include "ReluplexError.h"
#include "SparseVector.h"
#include "TableauRow.h"

CostFunctionManager::CostFunctionManager( ITableau *tableau )
    : _tableau( tableau )
    , _costFunction( NULL )
    , _basicCosts( NULL )
    , _multipliers( NULL )
    , _n( 0 )
    , _m( 0 )
    , _costFunctionStatus( COST_FUNCTION_INVALID )
{
}

CostFunctionManager::~CostFunctionManager()
{
    freeMemoryIfNeeded();
}

void CostFunctionManager::freeMemoryIfNeeded()
{
    if ( _multipliers )
    {
        delete[] _multipliers;
        _multipliers = NULL;
    }

    if ( _basicCosts )
    {
        delete[] _basicCosts;
        _basicCosts = NULL;
    }

    if ( _costFunction )
    {
        delete[] _costFunction;
        _costFunction = NULL;
    }
}

void CostFunctionManager::initialize()
{
    _n = _tableau->getN();
    _m = _tableau->getM();

    _ANColumn.initializeToEmpty( _m );

    freeMemoryIfNeeded();

    _costFunction = new double[_n - _m];
    if ( !_costFunction )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "CostFunctionManager::costFunction" );

    _basicCosts = new double[_m];
    if ( !_basicCosts )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "CostFunctionManager::basicCosts" );

    _multipliers = new double[_m];
    if ( !_multipliers )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "CostFunctionManager::multipliers" );

    invalidateCostFunction();
}

void CostFunctionManager::computeCostFunction( const Map<unsigned, double> &heuristicCost )
{
    /*
      A heuristic-based cost function is computed by computing the core
      cost function and adding to it the provided heuristic cost.

      The heuristic cost may include variables that are basic and variables
      that are non-basic. The basic variables are added to the vector of basic
      costs, which is normally used in computing the core cost fuction.
      Afterwards, once the modified core cost function has been computed,
      the remaining, non-basic variables are added.
    */

    // Reset cost function
    std::fill( _costFunction, _costFunction + _n - _m, 0.0 );

    // Compute the core basic costs
    computeBasicOOBCosts();

    // Iterate over the heuristic costs. Add any basic variables to the basic
    // cost vector, and the rest directly to the cost function.
    for ( const auto &variableCost : heuristicCost )
    {
        unsigned variable = variableCost.first;
        double cost = variableCost.second;
        unsigned variableIndex = _tableau->variableToIndex( variable );
        if ( _tableau->isBasic( variable ) )
            _basicCosts[variableIndex] += cost;
        else
            _costFunction[variableIndex] += cost;
    }

    // Complete the calculation of the modified core cost function
    computeMultipliers();
    computeReducedCosts();
}

void CostFunctionManager::computeCoreCostFunction()
{
    /*
      The core cost function is computed in three steps:

      1. Compute the basic costs c.
         These costs indicate whether a basic variable's row in
         the tableau should be added as is (variable too great;
         cost = 1), should be added negatively (variable is too
         small; cost = -1), or should be ignored (variable
         within bounds; cost = 0).

      2. Compute the multipliers p.
         p = c' * inv(B)
         This is solved by invoking BTRAN for pB = c'

      3. Compute the non-basic (reduced) costs.
         These are given by -p * AN

      Comment: the correctness follows from the fact that

      xB = inv(B)(b - AN xN)

      we ignore b because the constants don't matter for the cost
      function, and we omit xN because we want the function and not an
      evaluation thereof on a specific point.
    */

    std::fill( _costFunction, _costFunction + _n - _m, 0.0 );

    computeBasicOOBCosts();
    computeMultipliers();
    computeReducedCosts();

    _costFunctionStatus = ICostFunctionManager::COST_FUNCTION_JUST_COMPUTED;
}

void CostFunctionManager::computeBasicOOBCosts()
{
    for ( unsigned i = 0; i < _m; ++i )
    {
        if ( _tableau->basicTooLow( i ) )
            _basicCosts[i] = -1;
        else if ( _tableau->basicTooHigh( i ) )
            _basicCosts[i] = 1;
        else
            _basicCosts[i] = 0;
    }
}

void CostFunctionManager::computeMultipliers()
{
    _tableau->backwardTransformation( _basicCosts, _multipliers );
}

void CostFunctionManager::computeReducedCosts()
{
    for ( unsigned i = 0; i < _n - _m; ++i )
        computeReducedCost( i );
}

void CostFunctionManager::computeReducedCost( unsigned nonBasic )
{
    unsigned nonBasicIndex = _tableau->nonBasicIndexToVariable( nonBasic );
    _tableau->getSparseAColumn( nonBasicIndex, &_ANColumn );

    for ( unsigned entry = 0; entry < _ANColumn.getNnz(); ++entry )
        _costFunction[nonBasic] -= ( _multipliers[_ANColumn.getIndexOfEntry( entry )] * _ANColumn.getValueOfEntry( entry) );
}

void CostFunctionManager::dumpCostFunction() const
{
    printf( "Cost function:\n\t" );

    for ( unsigned i = 0; i < _n - _m; ++i )
    {
        double coefficient = _costFunction[i];
        if ( FloatUtils::isZero( coefficient ) )
            continue;

        if ( FloatUtils::isPositive( coefficient ) )
            printf( "+" );
        printf( "%lfx%u ", coefficient, _tableau->nonBasicIndexToVariable( i ) );
    }

    printf( "\n" );
}

ICostFunctionManager::CostFunctionStatus CostFunctionManager::getCostFunctionStatus() const
{
    return _costFunctionStatus;
}

void CostFunctionManager::setCostFunctionStatus( ICostFunctionManager::CostFunctionStatus status )
{
    _costFunctionStatus = status;
}

const double *CostFunctionManager::getCostFunction() const
{
    return _costFunction;
}

double CostFunctionManager::updateCostFunctionForPivot( unsigned enteringVariableIndex,
                                                        unsigned leavingVariableIndex,
                                                        double pivotElement,
                                                        const TableauRow *pivotRow,
                                                        const double *changeColumn
                                                        )
{
    /*
      This method is invoked when the non-basic _enteringVariable and
      basic _leaving variable are about to be swapped. It updates the
      reduced costs, without recomputing them from scratch.

      Suppose the pivot row is y = 5x + 3z, where x is entering and y
      is leaving. This corresponds to a new equation, x = 1/5y - 3/5z.
      If the previous cost for x was 10, then the new cost for y should
      be ( 10 / pivotElement ) = 10 / 5 = 2. This means that 2 units of y
      have the same cost as 10 units of x.

      However, now that we have 2 units of y, we have actually added 6 units
      of z to the overall cost - so the price of z needs to be adjusted by -6.

      Finally, we need to adjust the cost to reflect the pivot operation itself.
      The entering variable was, and remains, within bounds. The leaving variable
      is pressed against one of its bounds. So, if it was previously out-of-bounds
      (and contributed to the cost function), this needs to be removed.
    */

    ASSERT( _tableau->getM() == _m );
    ASSERT( _tableau->getN() == _n );

    /*
      The current reduced cost of the entering variable is stored in
      _costFunction, but since we have the change column we can compute a
      more accurate version from scratch
    */
    double enteringVariableCost = 0;
    for ( unsigned i = 0; i < _m; ++i )
        enteringVariableCost -= _basicCosts[i] * changeColumn[i];

    double normalizedError =
        FloatUtils::abs( enteringVariableCost - _costFunction[enteringVariableIndex] ) /
        ( FloatUtils::abs( enteringVariableCost ) + 1.0 );

    // Update the cost of the new non-basic
    _costFunction[enteringVariableIndex] = enteringVariableCost / pivotElement;

    for ( unsigned i = 0; i < _n - _m; ++i )
    {
        if ( i != enteringVariableIndex )
            _costFunction[i] -= (*pivotRow)[i] * _costFunction[enteringVariableIndex];
    }

    /*
      The leaving variable might have contributed to the cost function, but it will
      soon be made within bounds. So, we adjust the reduced costs accordingly.
    */
    _costFunction[enteringVariableIndex] -= _basicCosts[leavingVariableIndex];

    // The entering varibale is non-basic, so it is within bounds.
    _basicCosts[leavingVariableIndex] = 0;

    _costFunctionStatus = ICostFunctionManager::COST_FUNCTION_UPDATED;
    return normalizedError;
}

bool CostFunctionManager::costFunctionInvalid() const
{
    return _costFunctionStatus == ICostFunctionManager::COST_FUNCTION_INVALID;
}

void CostFunctionManager::invalidateCostFunction()
{
    _costFunctionStatus = ICostFunctionManager::COST_FUNCTION_INVALID;
}

bool CostFunctionManager::costFunctionJustComputed() const
{
    return _costFunctionStatus == ICostFunctionManager::COST_FUNCTION_JUST_COMPUTED;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
