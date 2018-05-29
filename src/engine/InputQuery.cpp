/*********************                                                        */
/*! \file InputQuery.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "InputQuery.h"
#include "MStringf.h"
#include "ReluplexError.h"
#include "FloatUtils.h"
#include <iostream>
#include <fstream>

InputQuery::InputQuery()
{
}

InputQuery::~InputQuery()
{
    freeConstraintsIfNeeded();
}

void InputQuery::setNumberOfVariables( unsigned numberOfVariables )
{
    _numberOfVariables = numberOfVariables;
}

void InputQuery::setLowerBound( unsigned variable, double bound )
{
    if ( variable >= _numberOfVariables )
    {
        throw ReluplexError( ReluplexError::VARIABLE_INDEX_OUT_OF_RANGE,
                             Stringf( "Variable = %u, number of variables = %u (setLowerBound)",
                                      variable, _numberOfVariables ).ascii() );
    }

    _lowerBounds[variable] = bound;
}

void InputQuery::setUpperBound( unsigned variable, double bound )
{
    if ( variable >= _numberOfVariables )
    {
        throw ReluplexError( ReluplexError::VARIABLE_INDEX_OUT_OF_RANGE,
                             Stringf( "Variable = %u, number of variables = %u (setUpperBound)",
                                      variable, _numberOfVariables ).ascii() );
    }

    _upperBounds[variable] = bound;
}

void InputQuery::addEquation( const Equation &equation )
{
    _equations.append( equation );
}

unsigned InputQuery::getNumberOfVariables() const
{
    return _numberOfVariables;
}

double InputQuery::getLowerBound( unsigned variable ) const
{
    if ( variable >= _numberOfVariables )
    {
        throw ReluplexError( ReluplexError::VARIABLE_INDEX_OUT_OF_RANGE,
                             Stringf( "Variable = %u, number of variables = %u (getLowerBound)",
                                      variable, _numberOfVariables ).ascii() );
    }

    if ( !_lowerBounds.exists( variable ) )
        return FloatUtils::negativeInfinity();

    return _lowerBounds.get( variable );
}

double InputQuery::getUpperBound( unsigned variable ) const
{
    if ( variable >= _numberOfVariables )
    {
        throw ReluplexError( ReluplexError::VARIABLE_INDEX_OUT_OF_RANGE,
                             Stringf( "Variable = %u, number of variables = %u (getUpperBound)",
                                      variable, _numberOfVariables ).ascii() );
    }

    if ( !_upperBounds.exists( variable ) )
        return FloatUtils::infinity();

    return _upperBounds.get( variable );
}

List<Equation> &InputQuery::getEquations()
{
    return _equations;
}

const List<Equation> &InputQuery::getEquations() const
{
    return _equations;
}

void InputQuery::setSolutionValue( unsigned variable, double value )
{
    _solution[variable] = value;
}

double InputQuery::getSolutionValue( unsigned variable ) const
{
    if ( !_solution.exists( variable ) )
        throw ReluplexError( ReluplexError::VARIABLE_DOESNT_EXIST_IN_SOLUTION,
                             Stringf( "Variable: %u", variable ).ascii() );

    return _solution.get( variable );
}

void InputQuery::addPiecewiseLinearConstraint( PiecewiseLinearConstraint *constraint )
{
    _plConstraints.append( constraint );
}

List<PiecewiseLinearConstraint *> &InputQuery::getPiecewiseLinearConstraints()
{
    return _plConstraints;
}

const List<PiecewiseLinearConstraint *> &InputQuery::getPiecewiseLinearConstraints() const
{
    return _plConstraints;
}

unsigned InputQuery::countInfiniteBounds()
{
    unsigned result = 0;

    for ( const auto &lowerBound : _lowerBounds )
        if ( lowerBound.second == FloatUtils::negativeInfinity() )
            ++result;

    for ( const auto &upperBound : _upperBounds )
        if ( upperBound.second == FloatUtils::infinity() )
            ++result;

    for ( unsigned i = 0; i < _numberOfVariables; ++i )
    {
        if ( !_lowerBounds.exists( i ) )
            ++result;
        if ( !_upperBounds.exists( i ) )
            ++result;
    }

    return result;
}

InputQuery &InputQuery::operator=( const InputQuery &other )
{
    _numberOfVariables = other._numberOfVariables;
    _equations = other._equations;
    _lowerBounds = other._lowerBounds;
    _upperBounds = other._upperBounds;
    _solution = other._solution;
    _debuggingSolution = other._debuggingSolution;

    freeConstraintsIfNeeded();
    for ( const auto &constraint : other._plConstraints )
        _plConstraints.append( constraint->duplicateConstraint() );

    return *this;
}

InputQuery::InputQuery( const InputQuery &other )
{
    *this = other;
}

void InputQuery::freeConstraintsIfNeeded()
{
    for ( auto &it : _plConstraints )
        delete it;

    _plConstraints.clear();
}

const Map<unsigned, double> &InputQuery::getLowerBounds() const
{
    return _lowerBounds;
}

const Map<unsigned, double> &InputQuery::getUpperBounds() const
{
    return _upperBounds;
}

void InputQuery::storeDebuggingSolution( unsigned variable, double value )
{
    _debuggingSolution[variable] = value;
}

void InputQuery::saveQuery(std::string fileName)
{
  std::ofstream queryFile;
  queryFile.open(fileName);
  // GENERAL QUERY INFORMATION
  // inspect that this does not match
  //queryFile << Stringf( "Total Variables1:  %04u \n", std::distance(_lowerBounds.begin(), _lowerBounds.end())).ascii();
  //

  //Amount of variables
  queryFile << Stringf( "%u\n", _numberOfVariables).ascii();

  //Amount of Equations
  queryFile << Stringf( "%u\n", _equations.size()).ascii();

  //Amount of constraints
  queryFile << Stringf( "%u", _plConstraints.size()).ascii();

  // BOUNDS
  for (Map<unsigned, double>::iterator it = _lowerBounds.begin(); it != _lowerBounds.end(); it++)
  {
     queryFile << Stringf("\n%d,%f,%f", it->first, it->second, _upperBounds[it->first]).ascii();
  }

  // EQUATIONS
  //queryFile << "Equations:\n";
  unsigned i = 0;
  for (List<Equation>::iterator eq_iter = _equations.begin(); eq_iter != _equations.end(); eq_iter++)
  {
    Equation e = *eq_iter;
    //equation number
    queryFile << Stringf( "\n%u,", i ).ascii();

    //equation type
    queryFile << Stringf("%01u,", e._type).ascii();

    //equation scalar
    queryFile << Stringf("%f", e._scalar).ascii();
    for (List<Equation::Addend>::iterator add_iter = e._addends.begin(); add_iter != e._addends.end(); add_iter++)
    {
      Equation::Addend a = *add_iter;
      queryFile << Stringf( ",%u,%f", a._variable, a._coefficient).ascii();         
    }
     //queryFile << Stringf("\n\tScalar: %.6f,\n\tAux Variable: %04u,\n", e._scalar, e._auxVariable).ascii();
     i+=1;
  }

  //queryFile << "\nConstraints:";
  unsigned j = 0;
  //for (const auto plc_iter = _plConstraints.begin(); plc_iter != _plConstraints.end(); plc_iter++)
  for ( const auto &constraint : _plConstraints )
  {
    //constraint number
    //queryFile << Stringf( "\nConstraint %04u: ", j ).ascii();
    queryFile << Stringf( "\n%u,", j).ascii();
    //auto aux = 
    queryFile << constraint->serializeToString().ascii();
  j++;
  }


  // Future: _plConstraints
  queryFile.close();
}

//void InputQuery::loadQuery(std::string filename){
//  (void)
//}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//