/*********************                                                        */
/*! \file GurobiWrapper.h
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

#ifndef __GurobiWrapper_h__
#define __GurobiWrapper_h__

#ifdef ENABLE_GUROBI

#include "MString.h"
#include "Map.h"

#include "gurobi_c++.h"

class GurobiWrapper
{
public:
    /*
      A term has the form: coefficient * variable
    */
    struct Term
    {
        Term( double coefficient, String variable )
            : _coefficient( coefficient )
            , _variable( variable )
        {
        }

        Term()
            : _coefficient( 0 )
            , _variable( "" )
        {
        }

        double _coefficient;
        String _variable;
    };

    GurobiWrapper();
    ~GurobiWrapper();

    // Add a new variabel to the model
    void addVariable( String name, double lb, double ub );

    // Add a new LEQ constraint, e.g. 3x + 4y <= -5
    void addLeqConstraint( const List<Term> &terms, double scalar );

    // Add a new GEQ constraint, e.g. 3x + 4y >= -5
    void addGeqConstraint( const List<Term> &terms, double scalar );

    // Add a new EQ constraint, e.g. 3x + 4y = -5
    void addEqConstraint( const List<Term> &terms, double scalar );

    // A cost function to minimize, or an objective function to maximize
    void setCost( const List<Term> &terms );
    void setObjective( const List<Term> &terms );

    // Solve and extract the solution
    void solve();
    void extractSolution( Map<String, double> &values, double &cost );

    void dump()
    {
        _model->write( "model.lp" );
    }

private:
    GRBEnv *_environment;
    GRBModel *_model;
    Map<String, GRBVar *> _nameToVariable;

    void addConstraint( const List<Term> &terms, double scalar, char sense );

    // Create a fresh model
    void reset();

    void freeMemoryIfNeeded();
};

#endif // ENABLE_GUROBI

#endif // __GurobiWrapper_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
