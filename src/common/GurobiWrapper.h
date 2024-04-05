/*********************                                                        */
/*! \file GurobiWrapper.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Teruhiro Tagomori
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
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
    enum VariableType {
        CONTINUOUS = 0,
        BINARY = 1,
        INTEGER = 2,
    };

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
    void addVariable( String name, double lb, double ub, VariableType type = CONTINUOUS );

    // Set the lower or upper bound for an existing variable
    void setLowerBound( String name, double lb );
    void setUpperBound( String name, double ub );

    inline double getLowerBound( const String &name )
    {
        return _model->getVarByName( name.ascii() ).get( GRB_DoubleAttr_LB );
    }

    inline double getUpperBound( const String &name )
    {
        return _model->getVarByName( name.ascii() ).get( GRB_DoubleAttr_UB );
    }

    // Add a new LEQ constraint, e.g. 3x + 4y <= -5
    void addLeqConstraint( const List<Term> &terms, double scalar );

    // Add a new GEQ constraint, e.g. 3x + 4y >= -5
    void addGeqConstraint( const List<Term> &terms, double scalar );

    // Add a new EQ constraint, e.g. 3x + 4y = -5
    void addEqConstraint( const List<Term> &terms, double scalar );

    // Add a piece-wise linear constraint
    void addPiecewiseLinearConstraint( String sourceVariable,
                                       String targetVariable,
                                       unsigned numPoints,
                                       const double *xPoints,
                                       const double *yPoints );

    // Add a new LEQ indicator constraint
    void addLeqIndicatorConstraint( const String binVarName,
                                    const int binVal,
                                    const List<Term> &terms,
                                    double scalar );

    // Add a new GEQ indicator constraint
    void addGeqIndicatorConstraint( const String binVarName,
                                    const int binVal,
                                    const List<Term> &terms,
                                    double scalar );

    // Add a new EQ indicator constraint
    void addEqIndicatorConstraint( const String binVarName,
                                   const int binVal,
                                   const List<Term> &terms,
                                   double scalar );

    // Add a bilinear constraint
    void addBilinearConstraint( const String input1, const String input2, const String output );

    // A cost function to minimize, or an objective function to maximize
    void setCost( const List<Term> &terms, double constant = 0 );
    void setObjective( const List<Term> &terms, double constant = 0 );

    inline double getOptimalCostOrObjective()
    {
        return _model->get( GRB_DoubleAttr_ObjVal );
    }

    // Set a cutoff value for the objective function. For example, if
    // maximizing x with cutoff value 0, Gurobi will return the
    // optimal value if greater than 0, and 0 if the optimal value is
    // less than 0.
    void setCutoff( double cutoff );

    // Returns true iff an optimal solution has been found
    bool optimal();

    // Returns true iff the cutoff value was used
    bool cutoffOccurred();

    // Returns true iff the instance is infeasible
    bool infeasible();

    // Returns true iff the instance timed out
    bool timeout();

    // Returns true iff a feasible solution has been found
    bool haveFeasibleSolution();

    // Specify a time limit, in seconds
    void setTimeLimit( double seconds );

    // Set verbosity
    inline void setVerbosity( unsigned verbosity )
    {
        _model->getEnv().set( GRB_IntParam_OutputFlag, verbosity );
    }

    inline bool containsVariable( String name ) const
    {
        return _nameToVariable.exists( name );
    }

    // Set number of threads
    inline void setNumberOfThreads( unsigned threads )
    {
        _model->getEnv().set( GRB_IntParam_Threads, threads );
    }

    inline void nonConvex()
    {
        _model->getEnv().set( GRB_IntParam_NonConvex, 2 );
    }

    // Solve and extract the solution, or the best known bound on the
    // objective function
    void solve();
    void extractSolution( Map<String, double> &values, double &costOrObjective );
    double getObjectiveBound();

    inline double getAssignment( const String &variable )
    {
        return _nameToVariable[variable]->get( GRB_DoubleAttr_X );
    }

    // Check if the assignment exists or not.
    inline bool existsAssignment( const String &variable )
    {
        return _nameToVariable.exists( variable ) && _model->get( GRB_IntAttr_SolCount ) > 0;
    }

    inline unsigned getNumberOfSimplexIterations()
    {
        return _model->get( GRB_DoubleAttr_IterCount );
    }

    inline unsigned getNumberOfNodes()
    {
        return _model->get( GRB_DoubleAttr_NodeCount );
    }

    inline unsigned getStatusCode()
    {
        return _model->get( GRB_IntAttr_Status );
    }

    inline void updateModel()
    {
        _model->update();
    }

    // Reset the underlying model
    void reset();

    // Clear the underlying model and create a fresh model
    void resetModel();

    // Dump the model to a file. Note that the suffix of the file is
    // used by Gurobi to determine the format. Using ".lp" is a good
    // default
    void dumpModel( String name );

private:
    GRBEnv *_environment;
    GRBModel *_model;
    Map<String, GRBVar *> _nameToVariable;
    double _timeoutInSeconds;

    void addConstraint( const List<Term> &terms, double scalar, char sense );
    // Add a new indicator constraint
    void addIndicatorConstraint( const String binVarName,
                                 const int binVal,
                                 const List<Term> &terms,
                                 double scalar,
                                 char sense );

    void freeModelIfNeeded();
    void freeMemoryIfNeeded();

    static void log( const String &message );
};

#else

#include "MString.h"
#include "Map.h"

class GurobiWrapper
{
public:
    /*
      This is a DUMMY class, for compilation purposes when Gurobi is
      disabled.
    */
    enum VariableType {
        CONTINUOUS = 0,
        BINARY = 1,
        INTEGER = 2,
    };

    struct Term
    {
        Term( double, String )
        {
        }
        Term()
        {
        }
    };

    GurobiWrapper()
    {
    }
    ~GurobiWrapper()
    {
    }

    void addVariable( String, double, double, VariableType type = CONTINUOUS )
    {
        (void)type;
    }
    void setLowerBound( String, double ){};
    void setUpperBound( String, double ){};
    double getLowerBound( const String & )
    {
        return 0;
    };
    double getUpperBound( const String & )
    {
        return 0;
    };
    void addLeqConstraint( const List<Term> &, double )
    {
    }
    void addGeqConstraint( const List<Term> &, double )
    {
    }
    void addEqConstraint( const List<Term> &, double )
    {
    }
    void addPiecewiseLinearConstraint( String, String, unsigned, const double *, const double * )
    {
    }
    void addLeqIndicatorConstraint( const String, const int, const List<Term> &, double )
    {
    }
    void addGeqIndicatorConstraint( const String, const int, const List<Term> &, double )
    {
    }
    void addEqIndicatorConstraint( const String, const int, const List<Term> &, double )
    {
    }
    void addBilinearConstraint( const String, const String, const String )
    {
    }
    void setCost( const List<Term> &, double /* constant */ = 0 )
    {
    }
    void setObjective( const List<Term> &, double /* constant */ = 0 )
    {
    }
    double getOptimalCostOrObjective()
    {
        return 0;
    };
    void setCutoff( double ){};
    void solve()
    {
    }
    void extractSolution( Map<String, double> &, double & )
    {
    }
    void reset()
    {
    }
    void resetModel()
    {
    }
    bool optimal()
    {
        return true;
    }
    bool cutoffOccurred()
    {
        return false;
    };
    bool infeasible()
    {
        return false;
    };
    bool timeout()
    {
        return false;
    };
    bool haveFeasibleSolution()
    {
        return true;
    };
    void setTimeLimit( double ){};
    void setVerbosity( unsigned ){};
    bool containsVariable( String /*name*/ ) const
    {
        return false;
    };
    void setNumberOfThreads( unsigned ){};
    void nonConvex(){};
    double getObjectiveBound()
    {
        return 0;
    };
    double getAssignment( const String & )
    {
        return 0;
    };
    unsigned getNumberOfSimplexIterations()
    {
        return 0;
    };
    unsigned getNumberOfNodes()
    {
        return 0;
    };
    unsigned getStatusCode()
    {
        return 0;
    };
    void updateModel(){};
    bool existsAssignment( const String & )
    {
        return false;
    };

    void dump()
    {
    }
    static void log( const String & );
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
