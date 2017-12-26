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

class ITableau;

class CostFunctionManager : public ICostFunctionManager
{
public:
    enum CostFunctionStatus {
        COST_FUNCTION_INVALID = 0,
        COST_FUNCTION_JUST_COMPUTED = 1,
        COST_FUNCTION_UPDATED = 2,
    };

    CostFunctionManager( ITableau *talbeau );
    ~CostFunctionManager();

    void initialize();
    void computeLinearCostFunction();
    void dumpCostFunction() const;
    const double *getCostFucntion() const;

    CostFunctionManager::CostFunctionStatus getCostFunctionStatus();

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
