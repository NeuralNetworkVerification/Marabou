/*********************                                                        */
/*! \file GurobiManager.cpp
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

#include "GurobiManager.h"

GurobiManager::GurobiManager(ITableau *tableau)
    : _tableau( tableau )
{
}

GurobiManager::~GurobiManager()
{
}

void GurobiManager::tightenBoundsOfVar(unsigned objectiveVar)
{
    printf("---------------------------\n");

    try {
        GRBEnv env = GRBEnv();
        GRBModel model = GRBModel(env);
        model.set(GRB_IntParam_OutputFlag, 0);

        // Create variables
        unsigned n = _tableau->getN();
        unsigned m = _tableau->getM();
        GRBVar vars[n];
        for (unsigned i = 0; i < n; ++i) {
            double lb = _tableau->getLowerBound(i);
            double ub = _tableau->getUpperBound(i);
            vars[i] = model.addVar(lb, ub, 1.0, GRB_CONTINUOUS, "x" + std::to_string(i));
            // printf("\tx%u: [%.2f, %.2f]\n", i, lb, ub);
        }

        // Add constraints
        for (unsigned i = 0; i < m; ++i) {
            TableauRow row(n);
            _tableau->getTableauRow(i, &row);
            
            unsigned lhs_var_id = row._lhs;
            GRBLinExpr lhs = vars[lhs_var_id];
            // printf("\tx%u = ", lhs_var_id);

            double row_scalar = row._scalar;
            GRBLinExpr rhs = GRBLinExpr(row_scalar);
            // if (row_scalar > 0) {
            //     printf("%.2f ", row_scalar);
            // } else if (row_scalar < 0) {
            //     printf("%.2f ", row_scalar);
            // }
            
            for (unsigned j = 0; j <= n - m; ++j)
            {
                unsigned var_id = row._row[j]._var;
                double coef = row._row[j]._coefficient;
                GRBVar var = vars[var_id];
                if (coef != 0) {
                    rhs += coef * var;
                }
                // if (coef > 0) {
                //     printf("+ %.2fx%u ", coef, var_id);
                // } else if (coef < 0) {
                //     printf("%.2fx%u ", coef, var_id);
                // }
            }
            
            model.addConstr(lhs, GRB_EQUAL, rhs, "c" + std::to_string(i));
            // printf("\n");
        }

        // Set objective
        GRBLinExpr objectiveExpr = vars[objectiveVar];

        // Optimize min value
        try {
            model.setObjective(objectiveExpr, GRB_MINIMIZE);
            model.optimize();
            double minValue = model.get(GRB_DoubleAttr_ObjVal);
            _tableau->tightenLowerBound(objectiveVar, minValue);
            // printf("\tMin: %.2f\n", minValue);
        } catch (...) {
            // printf("\tMin value haven't changed\n");
        }

        // Optimize max value
        try {
            model.setObjective(objectiveExpr, GRB_MAXIMIZE);
            model.optimize();
            double maxValue = model.get(GRB_DoubleAttr_ObjVal);
            _tableau->tightenUpperBound(objectiveVar, maxValue);
            // printf("\tMax: %.2f\n", maxValue);
        } catch(...) {
            // printf("\tMax value haven't changed\n");
        }

        // model.write("/Users/omricohen/Desktop/model" + std::to_string(objectiveVar) + ".lp");
    } catch(GRBException e) {
        printf("%s%d\n", "Error code = ", e.getErrorCode());
        printf("%s\n", e.getMessage().c_str());
    } catch(...) {
        printf("%s\n", "Exception during optimization");
    }
    
    printf("---------------------------\n");
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
