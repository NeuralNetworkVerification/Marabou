/*********************                                                        */
/*! \file CostFunctionManager.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __CostFunctionManager_h__
#define __CostFunctionManager_h__

#include "ICostFunctionManager.h"
#include "Map.h"
#include "SparseVector.h"

class ITableau;

class CostFunctionManager : public ICostFunctionManager
{
public:
    CostFunctionManager( ITableau *talbeau );
    ~CostFunctionManager();

    /*
      Initialize the data structures according to the tableau's dimensions.
    */
    void initialize();

    /*
      Compute the cost function, from scratch.
      Either compute just the core cost function, or add to it also
      a heuristic-based component.
    */
    void computeCostFunction( const Map<unsigned, double> &heuristicCost );
    void computeCoreCostFunction();

    /*
      Get the current cost function.
    */
    const double *getCostFunction() const;

    /*
      Get/set the status of the cost function
    */
    ICostFunctionManager::CostFunctionStatus getCostFunctionStatus() const;
    void setCostFunctionStatus( ICostFunctionManager::CostFunctionStatus status );
    bool costFunctionInvalid() const;
    bool costFunctionJustComputed() const;
    void invalidateCostFunction();

    /*
      Update the cost fucntion just before a coming pivot step, to avoid having to compute
      it from scratch afterwards.
    */
    double updateCostFunctionForPivot( unsigned enteringVariableIndex,
                                       unsigned leavingVariableIndex,
                                       double pivotElement,
                                       const TableauRow *pivotRow,
                                       const double *changeColumn );

    /*
      For debugging purposes: dump the cost function.
    */
    void dumpCostFunction() const;

private:
    /*
      The tableau.
    */
    ITableau *_tableau;

    /*
      The cost function and auxiliary variable for computing it.
    */
    double *_costFunction;
    double *_basicCosts;
    double *_multipliers;

    /*
      Tableau dimensions.
    */
    unsigned _n;
    unsigned _m;

    /*
      Status of the cost function.
    */
    CostFunctionStatus _costFunctionStatus;

    /*
      Work memeory
    */
    SparseVector _ANColumn;

    /*
      Free memory.
    */
    void freeMemoryIfNeeded();

    /*
      Compute the basic costs for basic variables that are out-of-bounds.
    */
    void computeBasicOOBCosts();
    void computeMultipliers();
    void computeReducedCosts();
    void computeReducedCost( unsigned nonBasic );
};

#endif // __CostFunctionManager_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
