/*********************                                                        */
/*! \file InputQuery.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __InputQuery_h__
#define __InputQuery_h__

#include "Equation.h"
#include "List.h"
#include "Map.h"
#include "PiecewiseLinearConstraint.h"

class InputQuery
{
public:
    InputQuery();
    ~InputQuery();

    /*
      Methods for setting and getting the input part of the query
    */
    void setNumberOfVariables( unsigned numberOfVariables );
    void setLowerBound( unsigned variable, double bound );
    void setUpperBound( unsigned variable, double bound );

    void addEquation( const Equation &equation );

    unsigned getNumberOfVariables() const;
    double getLowerBound( unsigned variable ) const;
    double getUpperBound( unsigned variable ) const;
    const Map<unsigned, double> &getLowerBounds() const;
    const Map<unsigned, double> &getUpperBounds() const;

    const List<Equation> &getEquations() const;
	List<Equation> &getEquations();

    void addPiecewiseLinearConstraint( PiecewiseLinearConstraint *constraint );
    const List<PiecewiseLinearConstraint *> &getPiecewiseLinearConstraints() const;
	List<PiecewiseLinearConstraint *> &getPiecewiseLinearConstraints();

    /*
      Methods for setting and getting the solution.
    */
    void setSolutionValue( unsigned variable, double value );
    double getSolutionValue( unsigned variable ) const;

    /*
      Count the number of infinite bounds in the input query.
    */
    unsigned countInfiniteBounds();

    /*
      Assignment operator and copy constructor, duplicate the constraints.
    */
    InputQuery &operator=( const InputQuery &other );
    InputQuery( const InputQuery &other );

    /*
      For debugging purposes only - store a correct possible solution
    */
    void storeDebuggingSolution( unsigned variable, double value );
    Map<unsigned, double> _debuggingSolution;
    
    void printQuery(std::string fileName);
    void loadQueury(std::string fileName);

private:
    unsigned _numberOfVariables;
    List<Equation> _equations;
    Map<unsigned, double> _lowerBounds;
    Map<unsigned, double> _upperBounds;
    List<PiecewiseLinearConstraint *> _plConstraints;

    Map<unsigned, double> _solution;

    /*
      Free any stored pl constraints.
    */
    void freeConstraintsIfNeeded();
};

#endif // __InputQuery_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
