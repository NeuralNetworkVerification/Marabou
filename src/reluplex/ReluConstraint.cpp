/*********************                                                        */
/*! \file ReluConstraint.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "Debug.h"
#include "FloatUtils.h"
#include "FreshVariables.h"
#include "PiecewiseLinearCaseSplit.h"
#include "ReluConstraint.h"
#include "ReluplexError.h"

ReluConstraint::ReluConstraint( unsigned b, unsigned f )
    : _b( b )
    , _f( f )
{
}

bool ReluConstraint::participatingVariable( unsigned variable ) const
{
    return ( variable == _b ) || ( variable == _f );
}

List<unsigned> ReluConstraint::getParticiatingVariables() const
{
    return List<unsigned>( { _b, _f } );
}

bool ReluConstraint::satisfied( const Map<unsigned, double> &assignment ) const
{
    if ( !( assignment.exists( _b ) && assignment.exists( _f ) ) )
        throw ReluplexError( ReluplexError::PARTICIPATING_VARIABLES_ABSENT );

    double bValue = assignment.get( _b );
    double fValue = assignment.get( _f );

    ASSERT( !FloatUtils::isNegative( fValue ) );

    if ( FloatUtils::isPositive( fValue ) )
        return FloatUtils::areEqual( bValue, fValue );
    else
        return !FloatUtils::isPositive( bValue );
}

List<PiecewiseLinearConstraint::Fix> ReluConstraint::getPossibleFixes( const Map<unsigned, double> &assignment ) const
{
    ASSERT( !satisfied( assignment ) );
    ASSERT( assignment.exists( _b ) );
    ASSERT( assignment.exists( _f ) );

    double bValue = assignment.get( _b );
    double fValue = assignment.get( _f );

    ASSERT( !FloatUtils::isNegative( fValue ) );

    List<PiecewiseLinearConstraint::Fix> fixes;

    // Possible violations:
    //   1. f is positive, b is positive, b and f are disequal
    //   2. f is positive, b is non-positive
    //   3. f is zero, b is positive
    if ( FloatUtils::isPositive( fValue ) )
    {
        if ( FloatUtils::isPositive( bValue ) )
        {
            fixes.append( PiecewiseLinearConstraint::Fix( _b, fValue ) );
            fixes.append( PiecewiseLinearConstraint::Fix( _f, bValue ) );
        }
        else
        {
            fixes.append( PiecewiseLinearConstraint::Fix( _b, fValue ) );
            fixes.append( PiecewiseLinearConstraint::Fix( _f, 0 ) );
        }
    }
    else
    {
        fixes.append( PiecewiseLinearConstraint::Fix( _b, 0 ) );
        fixes.append( PiecewiseLinearConstraint::Fix( _f, bValue ) );
    }

    return fixes;
}

List<PiecewiseLinearCaseSplit> ReluConstraint::getCaseSplits() const
{
    List<PiecewiseLinearCaseSplit> splits;

    PiecewiseLinearCaseSplit activePhase;
    PiecewiseLinearCaseSplit inactivePhase;

    bool lowerBound = false;
    bool upperBound = true;

    // Active phase: b >= 0, b - f = 0
    activePhase.setBoundTightening( _b, lowerBound, 0.0 );
    Equation activeEquation;
    activeEquation.addAddend( 1, _b );
    activeEquation.addAddend( -1, _f );

    unsigned auxVariable = FreshVariables::getNextVariable();
    activeEquation.addAddend( 1, auxVariable );
    activeEquation.markAuxiliaryVariable( auxVariable );

    activeEquation.setScalar( 0 );
    activePhase.setEquation( activeEquation );

    // Inactive phase: b <= 0, f = 0
    inactivePhase.setBoundTightening( _b, upperBound, 0.0 );
    Equation inactiveEquation;
    inactiveEquation.addAddend( 1, _f );
    auxVariable = FreshVariables::getNextVariable();
    inactiveEquation.addAddend( 1, auxVariable );
    inactiveEquation.markAuxiliaryVariable( auxVariable );

    inactiveEquation.setScalar( 0 );
    inactivePhase.setEquation( inactiveEquation );

    splits.append( activePhase );
    splits.append( inactivePhase );

    return splits;
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
