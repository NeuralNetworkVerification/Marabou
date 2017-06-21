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

#include "List.h"
#include "Map.h"

class InputQuery
{
public:
    /* A class representing a single input equation. For now, all
    equations are interpreted as equalities, i.e. the sum of all
    addends equals the scalar */
    struct Equation
    {
    public:
        struct Addend
        {
        public:
            Addend( double coefficient, unsigned variable );

            double _coefficient;
            unsigned _variable;
        };

        void addAddend( double coefficient, unsigned variable );
        void setScalar( double scalar );
        void markAuxiliaryVariable( unsigned auxVariable );

        List<Addend> _addends;
        double _scalar;
        unsigned _auxVariable;
    };

    InputQuery();
    ~InputQuery();

    /*
      Methods for setting and getting the input part of the query
    */
    void setNumberOfVariables( unsigned numberOfVariables );
    void setLowerBound( unsigned variable, double bound );
    void setUpperBound( unsigned variable, double bound );

    void addEquation( const Equation &equation );

    double getNumberOfVariables() const;
    double getLowerBound( unsigned variable ) const;
    double getUpperBound( unsigned variable ) const;

    const List<InputQuery::Equation> &getEquations() const;

    /*
      Methods for setting and getting the solution
    */

    void setSolutionValue( unsigned variable, double value );
    double getSolutionValue( unsigned variable ) const;

private:
    unsigned _numberOfVariables;
    List<InputQuery::Equation> _equations;
    Map<unsigned, double> _lowerBounds;
    Map<unsigned, double> _upperBounds;

    Map<unsigned, double> _solution;
};

#endif // __InputQuery_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
