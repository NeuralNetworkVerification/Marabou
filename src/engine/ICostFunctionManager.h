/*********************                                                        */
/*! \file ICostFunctionManager.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __ICostFunctionManager_h__
#define __ICostFunctionManager_h__

#include "Map.h"

class TableauRow;

class ICostFunctionManager
{
public:
    enum CostFunctionStatus {
        COST_FUNCTION_INVALID = 0,
        COST_FUNCTION_JUST_COMPUTED = 1,
        COST_FUNCTION_UPDATED = 2,
    };

    virtual ~ICostFunctionManager() {};

    virtual void initialize() = 0;
    virtual ICostFunctionManager::CostFunctionStatus getCostFunctionStatus() const = 0;
    virtual void computeCostFunction( const Map<unsigned, double> &heuristicCost ) = 0;
    virtual void computeCoreCostFunction() = 0;
    virtual const double *getCostFunction() const = 0;
    virtual void dumpCostFunction() const = 0;
    virtual void setCostFunctionStatus( ICostFunctionManager::CostFunctionStatus status ) = 0;
    virtual double updateCostFunctionForPivot( unsigned enteringVariableIndex,
                                               unsigned leavingVariableIndex,
                                               double pivotElement,
                                               const TableauRow *pivotRow,
                                               const double *changeColumn ) = 0;

    virtual bool costFunctionInvalid() const = 0;
    virtual bool costFunctionJustComputed() const = 0;
    virtual void invalidateCostFunction() = 0;
};

#endif // __ICostFunctionManager_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
