/*********************                                                        */
/*! \file InputQuery.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** This implementation of the IQuery abstract class allows the pushing/poping
 ** of constraints in an incremental manner.
 **
 **/

#ifndef __InputQuery_h__
#define __InputQuery_h__

#include "CDMap.h"
#include "CommonError.h"
#include "Equation.h"
#include "IQuery.h"
#include "List.h"
#include "MString.h"
#include "NonlinearConstraint.h"
#include "PiecewiseLinearConstraint.h"
#include "context/cdo.h"

#include <context/cdlist.h>
#include <context/context.h>

class InputQuery : public IQuery
{
    typedef CDMap<unsigned, double> VariableValueMap;
    typedef CDMap<unsigned, unsigned> VariableIndexMap;
    typedef CDMap<unsigned, unsigned> IndexVariableMap;

    struct CleanUpEquation
    {
        void operator()( Equation **ptr ) const
        {
            delete *ptr;
        }
    };

    struct CleanUpPLConstraint
    {
        void operator()( PiecewiseLinearConstraint **ptr ) const
        {
            delete *ptr;
        }
    };

    struct CleanUpNLConstraint
    {
        void operator()( NonlinearConstraint **ptr ) const
        {
            delete *ptr;
        }
    };

    typedef CVC4::context::CDList<Equation *, CleanUpEquation> EquationList;
    typedef CVC4::context::CDList<PiecewiseLinearConstraint *, CleanUpPLConstraint>
        PLConstraintsList;
    typedef CVC4::context::CDList<NonlinearConstraint *, CleanUpNLConstraint> NLConstraintsList;

public:
    InputQuery();
    ~InputQuery();

    void setNumberOfVariables( unsigned numberOfVariables );
    unsigned getNumberOfVariables() const;
    unsigned getNewVariable();

    /*
      The set*Bound methods will overwrite the currently stored bound of the variable.
      They will throw an error if the context level is positive and the variable bound
      has already been set.

      The tighten*Bound methods will have update the currently stored bound of the
      variable if and only if the new bound is tighter. The methods return true if and
      if only the bound is tightened.
    */
    void setLowerBound( unsigned variable, double bound );
    void setUpperBound( unsigned variable, double bound );
    bool tightenLowerBound( unsigned variable, double bound );
    bool tightenUpperBound( unsigned variable, double bound );

    double getLowerBound( unsigned variable ) const;
    double getUpperBound( unsigned variable ) const;

    /*
      Note: currently there is no API call for removing equations, PLConstraints or NLConstraints,
      becaues CDList does not support removal.
    */
    void addEquation( const Equation &equation );
    unsigned getNumberOfEquations() const;
    ;
    void getEquations( List<Equation> &equations ) const;

    void addPiecewiseLinearConstraint( PiecewiseLinearConstraint *constraint );
    void getPiecewiseLinearConstraints( List<PiecewiseLinearConstraint *> &constraints ) const;
    void addClipConstraint( unsigned b, unsigned f, double floor, double ceiling );

    void addNonlinearConstraint( NonlinearConstraint *constraint );
    void getNonlinearConstraints( Vector<NonlinearConstraint *> &constraints ) const;

    /*
      Methods for handling input and output variables
    */
    void markInputVariable( unsigned variable, unsigned inputIndex );
    void markOutputVariable( unsigned variable, unsigned outputIndex );
    unsigned inputVariableByIndex( unsigned index ) const;
    unsigned outputVariableByIndex( unsigned index ) const;
    unsigned getNumInputVariables() const;
    unsigned getNumOutputVariables() const;
    void getInputVariables( List<unsigned> &inputVariables ) const;
    void getOutputVariables( List<unsigned> &outputVariables ) const;
    List<unsigned> getInputVariables() const;
    List<unsigned> getOutputVariables() const;

    /*
      Methods for setting and getting the solution.
    */
    void setSolutionValue( unsigned variable, double value );
    double getSolutionValue( unsigned variable ) const;

    /*
      Store a correct possible solution
    */
    void storeDebuggingSolution( unsigned variable, double value );

    /*
      Serializes the query to a file which can then be loaded using QueryLoader.
    */
    void saveQuery( const String &fileName );
    void saveQueryAsSmtLib( const String &filename ) const;

    /*
      Generate a non-context-dependent version of the Query
    */
    Query *generateQuery() const;

    void dump() const;

    /*
      The following methods perform directly read/write to the _userContext object.
    */
    inline unsigned getLevel()
    {
        return _userContext.getLevel();
    };
    inline void push()
    {
        _userContext.push();
    };
    inline void pop()
    {
        if ( getLevel() == 0 )
            throw CommonError( CommonError::POPPING_ZERO_CONTEXT_LEVEL,
                               "Cannot pop when context level is 0." );
        _userContext.pop();
    };
    inline void popTo( unsigned toLevel )
    {
        _userContext.popto( toLevel );
    };

private:
    /*
      This is an outward-facing context which manages incrementally pushing and popping constraints
    */
    CVC4::context::Context _userContext;

    /*
      The following constitutes a verification query
      Mapping between input/output variables and their indices are optional
    */
    CVC4::context::CDO<unsigned> _numberOfVariables;
    EquationList _equations;
    VariableValueMap _lowerBounds;
    VariableValueMap _upperBounds;
    PLConstraintsList _plConstraints;
    NLConstraintsList _nlConstraints;
    VariableIndexMap _variableToInputIndex;
    IndexVariableMap _inputIndexToVariable;
    VariableIndexMap _variableToOutputIndex;
    IndexVariableMap _outputIndexToVariable;

    /*
      Stores the satisfying assignment.
    */
    VariableValueMap _solution;

    /*
      Stores a correct possible assignment.
    */
    VariableValueMap _debuggingSolution;

    /*
      Free any stored pl constraints.
    */
    void freeConstraintsIfNeeded();
};

#endif // __InputQuery_h__
