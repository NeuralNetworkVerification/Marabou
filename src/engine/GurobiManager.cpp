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

using namespace std;

GurobiManager::GurobiManager(ITableau *tableau)
{
    _tableau = tableau;
}

GurobiManager::~GurobiManager()
{
}

void GurobiManager::printCurrentBounds() {
    printf( "---------------------------\n" );
    printf( "Current bounds:\n" );

    unsigned n = _tableau->getN();
    unsigned m = _tableau->getM();
    for (unsigned i = 0; i < n; ++i) {
        double lb, ub;
        lb = _tableau->getLowerBound( i );
        ub = _tableau->getUpperBound( i );
        printf( "\tx%u: [%8.4lf, %8.4lf]\n", i, lb, ub );
    }

    for (unsigned i = 0; i < m; ++i) {
        TableauRow row( n );
        _tableau->getTableauRow( i, &row );

        unsigned lhs = row._lhs;
        printf( "\tx%u =", lhs );

        for ( unsigned j = 0; j <= n - m; ++j )
        {
            unsigned row_v = row._row[j]._var;
            double row_coef = row._row[j]._coefficient;

            if (row_coef > 0) {
                printf( " +%.2fx%u", row_coef, row_v );
            } else if (row_coef < 0) {
                printf( " %.2fx%u", row_coef, row_v );
            }
        }

        double row_scalar = row._scalar;
        if (row_scalar > 0) {
            printf( " +%.2f", row_scalar );
        } else if (row_scalar < 0) {
            printf( " %.2f", row_scalar );
        }

        printf("\n");
    }
    printf( "---------------------------\n" );
}


// void Tableau::printCurrentBounds()
// {
//     printf("---------------------------\n");
//     printf("Current bounds:\n");

//     try {
//         // GRBEnv env = GRBEnv();
//         // GRBModel model = GRBModel(env);

//         // Create variables
//         unsigned varsNum = _n;
//         // GRBVar vars[varsNum];
//         for (unsigned i = 0; i < varsNum; ++i) {
//             double lb = getLowerBound(i);
//             double ub = getUpperBound(i);
//             // vars[i] = model.addVar(lb, ub, 0.0, GRB_CONTINUOUS, "x" + to_string(i));
//             printf("\tx%u: [%.2f, %.2f]\n", i, lb, ub);
//         }

//         // Add constraints
//         for (unsigned i = 0; i < _m; ++i) {
//             TableauRow row(_n);
//             getTableauRow(i, &row);
            
//             unsigned lhs = row._lhs;
//             printf("\tx%u = ", lhs);
            
//             double row_scalar = row._scalar;
//             // GRBLinExpr rhs = GRBLinExpr(row._scalar);
//             if (row_scalar > 0) {
//                 printf("%.2f ", row_scalar);
//             } else if (row_scalar < 0) {
//                 printf("%.2f ", row_scalar);
//             }
            
//             for (unsigned j = 0; j <= _n - _m; ++j)
//             {
//                 unsigned row_v = row._row[j]._var;
//                 double row_coef = row._row[j]._coefficient;
//                 // rhs.AddTerm(row_coef, vars[row_v]);
//                 if (row_coef > 0) {
//                     printf("+ %.2fx%u ", row_coef, row_v);
//                 } else if (row_coef < 0) {
//                     printf("%.2fx%u ", row_coef, row_v);
//                 }
//             }
            
//             // model.addConstr(lhs, GRB_EQUAL, rhs, "c" + to_string(i));
//             printf("\n");
//         }

//         // Set objective
//         // GRBLinExpr objectiveExpr = vars[0];
        
//         // model.setObjective(objectiveExpr, GRB_MAXIMIZE);
//         // model.optimize();
//         // double max = model.get(GRB_DoubleAttr_ObjVal);
        
//         // model.setObjective(objectiveExpr, GRB_MINIMIZE);
//         // model.optimize();
//         // double min = model.get(GRB_DoubleAttr_ObjVal);

//         // cout << "Max: " << max << endl;
//         // cout << "Min: " << min << endl;
//     // } catch(GRBException e) {
//     //     printf("%s\n", "Error code = " + e.getErrorCode());
//     //     printf("%s\n", e.getMessage());
//     } catch(...) {
//         printf("%s\n", "Exception during optimization");
//     }
    
//     printf("---------------------------\n");
// }


// int calcGurobi() {
//     try {
//         GRBEnv env = GRBEnv();
//         GRBModel model = GRBModel(env);

//         // Create variables
//         unsigned varsNum = 3;
//         GRBVar vars[varsNum];
//         for (unsigned i = 0; i < varsNum; ++i) {
//             vars[i] = model.addVar(-1.0, 1.0, 0.0, GRB_CONTINUOUS, "x" + to_string(i));
//         }
        
//         GRBVar x = vars[0];
//         GRBVar y = vars[1];
//         GRBVar z = vars[2];

//         // Add constraints
//         int t = 2;
//         GRBTempConstr c0 = x + y + 3 * z <= 4;
//         GRBTempConstr c1 = x + y == t;
//         model.addConstr(c0, "c0");
//         model.addConstr(c1, "c1");
        
//         // Set objective
//         GRBLinExpr objectiveExpr = x + y + 2 * z;
        
//         model.setObjective(objectiveExpr, GRB_MAXIMIZE);
//         model.optimize();
//         double max = model.get(GRB_DoubleAttr_ObjVal);
        
//         model.setObjective(objectiveExpr, GRB_MINIMIZE);
//         model.optimize();
//         double min = model.get(GRB_DoubleAttr_ObjVal);

//         cout << "Max: " << max << endl;
//         cout << "Min: " << min << endl;
//     } catch(GRBException e) {
//         cout << "Error code = " << e.getErrorCode() << endl;
//         cout << e.getMessage() << endl;
//     } catch(...) {
//         cout << "Exception during optimization" << endl;
//     }
//     return 0;
// }


//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
