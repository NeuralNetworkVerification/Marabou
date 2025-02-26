/*********************                                                        */
/*! \file IQuery.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** This is an abstract class that contains methods to encode a verification query
 **/

#ifndef __IQuery_h__
#define __IQuery_h__

#include "Equation.h"
#include "List.h"
#include "MString.h"
#include "Map.h"
#include "NetworkLevelReasoner.h"
#include "NonlinearConstraint.h"
#include "PiecewiseLinearConstraint.h"

class Query;

class IQuery
{
public:
    IQuery(){};
    virtual ~IQuery(){};

    virtual void setNumberOfVariables( unsigned numberOfVariables ) = 0;
    virtual unsigned getNumberOfVariables() const = 0;
    virtual unsigned getNewVariable() = 0;

    virtual void setLowerBound( unsigned variable, double bound ) = 0;
    virtual void setUpperBound( unsigned variable, double bound ) = 0;
    virtual bool tightenLowerBound( unsigned variable, double bound ) = 0;
    virtual bool tightenUpperBound( unsigned variable, double bound ) = 0;
    virtual double getLowerBound( unsigned variable ) const = 0;
    virtual double getUpperBound( unsigned variable ) const = 0;

    virtual void addEquation( const Equation &equation ) = 0;
    virtual unsigned getNumberOfEquations() const = 0;

    virtual void addPiecewiseLinearConstraint( PiecewiseLinearConstraint *constraint ) = 0;
    virtual void addClipConstraint( unsigned b, unsigned f, double floor, double ceiling ) = 0;
    virtual void addNonlinearConstraint( NonlinearConstraint *constraint ) = 0;
    virtual void getNonlinearConstraints( Vector<NonlinearConstraint *> &constraints ) const = 0;

    virtual void markInputVariable( unsigned variable, unsigned inputIndex ) = 0;
    virtual void markOutputVariable( unsigned variable, unsigned outputIndex ) = 0;
    virtual unsigned inputVariableByIndex( unsigned index ) const = 0;
    virtual unsigned outputVariableByIndex( unsigned index ) const = 0;
    virtual unsigned getNumInputVariables() const = 0;
    virtual unsigned getNumOutputVariables() const = 0;
    virtual List<unsigned> getInputVariables() const = 0;
    virtual List<unsigned> getOutputVariables() const = 0;

    /*
      Methods for setting and getting the solution.
    */
    virtual void setSolutionValue( unsigned variable, double value ) = 0;
    virtual double getSolutionValue( unsigned variable ) const = 0;

    /*
      Store a correct possible solution
    */
    virtual void storeDebuggingSolution( unsigned variable, double value ) = 0;

    /*
      Serializes the query to a file which can then be loaded using QueryLoader.
    */
    virtual void saveQuery( const String &fileName ) = 0;

    /*
      Generate a non-context-dependent version of the Query
    */
    virtual Query *generateQuery() const = 0;
};

#endif // __IQuery_h__
