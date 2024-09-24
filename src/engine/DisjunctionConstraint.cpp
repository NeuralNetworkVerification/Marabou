/*********************                                                        */
/*! \file DisjunctionConstraint.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Haoze Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** See the description of the class in DisjunctionConstraint.h.
 **/

#include "DisjunctionConstraint.h"

#include "Debug.h"
#include "InfeasibleQueryException.h"
#include "MStringf.h"
#include "MarabouError.h"
#include "Query.h"
#include "Statistics.h"

DisjunctionConstraint::DisjunctionConstraint( const List<PiecewiseLinearCaseSplit> &disjuncts )
    : PiecewiseLinearConstraint( disjuncts.size() )
    , _disjuncts( disjuncts.begin(), disjuncts.end() )
    , _feasibleDisjuncts( disjuncts.size(), 0 )
{
    for ( unsigned ind = 0; ind < disjuncts.size(); ++ind )
        _feasibleDisjuncts.append( ind );

    extractParticipatingVariables();
}

DisjunctionConstraint::DisjunctionConstraint( const Vector<PiecewiseLinearCaseSplit> &disjuncts )
    : PiecewiseLinearConstraint( disjuncts.size() )
    , _disjuncts( disjuncts )
    , _feasibleDisjuncts( disjuncts.size(), 0 )
{
    for ( unsigned ind = 0; ind < disjuncts.size(); ++ind )
        _feasibleDisjuncts.append( ind );

    extractParticipatingVariables();
}

DisjunctionConstraint::DisjunctionConstraint( const String &serializedDisjunction )
{
    Vector<PiecewiseLinearCaseSplit> disjuncts;
    String serializedValues =
        serializedDisjunction.substring( 5, serializedDisjunction.length() - 5 );
    List<String> values = serializedValues.tokenize( "," );
    auto val = values.begin();
    unsigned numDisjuncts = atoi( val->ascii() );
    ++val;
    for ( unsigned i = 0; i < numDisjuncts; ++i )
    {
        PiecewiseLinearCaseSplit split;
        unsigned numBounds = atoi( val->ascii() );
        ++val;
        for ( unsigned bi = 0; bi < numBounds; ++bi )
        {
            Tightening::BoundType type = ( *val == "l" ) ? Tightening::LB : Tightening::UB;
            ++val;
            unsigned var = atoi( val->ascii() );
            ++val;
            double bd = atof( val->ascii() );
            ++val;
            split.storeBoundTightening( Tightening( var, bd, type ) );
        }
        unsigned numEquations = atoi( val->ascii() );

        ++val;
        for ( unsigned ei = 0; ei < numEquations; ++ei )
        {
            Equation::EquationType type = Equation::EQ;
            if ( *val == "l" )
                type = Equation::LE;
            else if ( *val == "g" )
                type = Equation::GE;
            else
            {
                ASSERT( *val == "e" );
            }
            Equation eq( type );
            ++val;
            unsigned numAddends = atoi( val->ascii() );
            ++val;
            for ( unsigned ai = 0; ai < numAddends; ++ai )
            {
                double coef = atof( val->ascii() );
                ++val;
                unsigned var = atoi( val->ascii() );
                ++val;
                eq.addAddend( coef, var );
            }
            eq.setScalar( atof( val->ascii() ) );
            ++val;
            split.addEquation( eq );
        }
        disjuncts.append( split );
    }
    _disjuncts = disjuncts;

    for ( unsigned ind = 0; ind < disjuncts.size(); ++ind )
        _feasibleDisjuncts.append( ind );

    extractParticipatingVariables();
}

PiecewiseLinearFunctionType DisjunctionConstraint::getType() const
{
    return PiecewiseLinearFunctionType::DISJUNCTION;
}

PiecewiseLinearConstraint *DisjunctionConstraint::duplicateConstraint() const
{
    DisjunctionConstraint *clone = new DisjunctionConstraint( _disjuncts );
    *clone = *this;
    initializeDuplicateCDOs( clone );
    return clone;
}

void DisjunctionConstraint::restoreState( const PiecewiseLinearConstraint *state )
{
    const DisjunctionConstraint *disjunction = dynamic_cast<const DisjunctionConstraint *>( state );

    CVC4::context::CDO<bool> *activeStatus = _cdConstraintActive;
    CVC4::context::CDO<PhaseStatus> *phaseStatus = _cdPhaseStatus;
    CVC4::context::CDList<PhaseStatus> *infeasibleCases = _cdInfeasibleCases;
    *this = *disjunction;
    _cdConstraintActive = activeStatus;
    _cdPhaseStatus = phaseStatus;
    _cdInfeasibleCases = infeasibleCases;
}

void DisjunctionConstraint::registerAsWatcher( ITableau *tableau )
{
    for ( const auto &variable : _participatingVariables )
        tableau->registerToWatchVariable( this, variable );
}

void DisjunctionConstraint::unregisterAsWatcher( ITableau *tableau )
{
    for ( const auto &variable : _participatingVariables )
        tableau->unregisterToWatchVariable( this, variable );
}

void DisjunctionConstraint::notifyLowerBound( unsigned variable, double bound )
{
    if ( _statistics )
        _statistics->incLongAttribute( Statistics::NUM_BOUND_NOTIFICATIONS_TO_PL_CONSTRAINTS );

    if ( _boundManager == nullptr && existsLowerBound( variable ) &&
         !FloatUtils::gt( bound, getLowerBound( variable ) ) )
        return;

    setLowerBound( variable, bound );

    // Proofs currently don't support case elimination
    if ( !_boundManager || !_boundManager->shouldProduceProofs() )
        updateFeasibleDisjuncts();
}

void DisjunctionConstraint::notifyUpperBound( unsigned variable, double bound )
{
    if ( _statistics )
        _statistics->incLongAttribute( Statistics::NUM_BOUND_NOTIFICATIONS_TO_PL_CONSTRAINTS );

    if ( _boundManager == nullptr && existsUpperBound( variable ) &&
         !FloatUtils::lt( bound, getUpperBound( variable ) ) )
        return;

    setUpperBound( variable, bound );

    // Proofs currently don't support case elimination
    if ( !_boundManager || !_boundManager->shouldProduceProofs() )
        updateFeasibleDisjuncts();
}

bool DisjunctionConstraint::participatingVariable( unsigned variable ) const
{
    return _participatingVariables.exists( variable );
}

List<unsigned> DisjunctionConstraint::getParticipatingVariables() const
{
    List<unsigned> variables;
    for ( const auto &var : _participatingVariables )
        variables.append( var );

    return variables;
}

bool DisjunctionConstraint::satisfied() const
{
    for ( const auto &disjunct : _disjuncts )
        if ( disjunctSatisfied( disjunct ) )
            return true;

    return false;
}

List<PiecewiseLinearConstraint::Fix> DisjunctionConstraint::getPossibleFixes() const
{
    // Reluplex does not currently work with Gurobi.
    ASSERT( _gurobi == NULL );

    return List<PiecewiseLinearConstraint::Fix>();
}

List<PiecewiseLinearConstraint::Fix>
DisjunctionConstraint::getSmartFixes( ITableau * /* tableau */ ) const
{
    // Reluplex does not currently work with Gurobi.
    ASSERT( _gurobi == NULL );

    return getPossibleFixes();
}

List<PiecewiseLinearCaseSplit> DisjunctionConstraint::getCaseSplits() const
{
    return List<PiecewiseLinearCaseSplit>( _disjuncts.begin(), _disjuncts.end() );
}

List<PhaseStatus> DisjunctionConstraint::getAllCases() const
{
    List<PhaseStatus> cases;
    for ( unsigned i = 0; i < _disjuncts.size(); ++i )
        cases.append( indToPhaseStatus( i ) );
    return cases;
}

PiecewiseLinearCaseSplit DisjunctionConstraint::getCaseSplit( PhaseStatus phase ) const
{
    return _disjuncts.get( phaseStatusToInd( phase ) );
}

bool DisjunctionConstraint::phaseFixed() const
{
    return _feasibleDisjuncts.size() == 1;
}

PiecewiseLinearCaseSplit DisjunctionConstraint::getImpliedCaseSplit() const
{
    return _disjuncts.get( *_feasibleDisjuncts.begin() );
}

PiecewiseLinearCaseSplit DisjunctionConstraint::getValidCaseSplit() const
{
    return getImpliedCaseSplit();
}

void DisjunctionConstraint::transformToUseAuxVariables( Query &inputQuery )
{
    Vector<PiecewiseLinearCaseSplit> newDisjuncts;
    for ( const auto &disjunct : _disjuncts )
    {
        PiecewiseLinearCaseSplit newDisjunct;

        // Store the bounds in the old disjunct
        for ( const auto &bound : disjunct.getBoundTightenings() )
            newDisjunct.storeBoundTightening( bound );

        List<Equation> equationsToProcess;
        for ( const auto &equation : disjunct.getEquations() )
        {
            if ( equation._type == Equation::EQ )
            {
                Equation equation1 = equation;
                equation1._type = Equation::GE;
                Equation equation2 = equation;
                equation2._type = Equation::LE;
                equationsToProcess.append( equation1 );
                equationsToProcess.append( equation2 );
            }
            else
            {
                equationsToProcess.append( equation );
            }
        }

        /*
          Given a constraint AX >= b in the disjunct, we introduce an auxiliary
          variable aux, add new linear constraints AX + aux = b and change the
          constraint in the disjunct to aux <= 0

          The LE case is symmetric.
        */
        for ( const auto &equation : equationsToProcess )
        {
            unsigned aux = inputQuery.getNumberOfVariables();
            inputQuery.setNumberOfVariables( aux + 1 );

            // Equation in the disjunct is AX ? b, we want to add AX + aux = b
            Equation newEquation = equation;
            newEquation._type = Equation::EQ;
            newEquation.addAddend( 1, aux );
            inputQuery.addEquation( newEquation );

            switch ( equation._type )
            {
            case Equation::EQ:
            {
                ASSERT( false );
                break;
            }
            case Equation::GE:
            {
                newDisjunct.storeBoundTightening( Tightening( aux, 0, Tightening::UB ) );
                break;
            }
            case Equation::LE:
            {
                newDisjunct.storeBoundTightening( Tightening( aux, 0, Tightening::LB ) );
                break;
            }
            }
        }
        newDisjuncts.append( newDisjunct );
    }

    _disjuncts = newDisjuncts;

    _feasibleDisjuncts.clear();
    for ( unsigned ind = 0; ind < _disjuncts.size(); ++ind )
        _feasibleDisjuncts.append( ind );
    extractParticipatingVariables();
}

void DisjunctionConstraint::dump( String &output ) const
{
    output = Stringf( "DisjunctionConstraint:\n" );

    for ( const auto &disjunct : _disjuncts )
    {
        String disjunctOutput;
        disjunct.dump( disjunctOutput );
        output += Stringf( "\t%s\n", disjunctOutput.ascii() );
    }

    output += Stringf( "Active? %s.", _constraintActive ? "Yes" : "No" );
}

void DisjunctionConstraint::updateVariableIndex( unsigned oldIndex, unsigned newIndex )
{
    // Variable reindexing can only occur in preprocessing before Gurobi is
    // registered.
    ASSERT( _gurobi == NULL );

    ASSERT( !participatingVariable( newIndex ) );

    if ( _assignment.exists( oldIndex ) )
    {
        _assignment[newIndex] = _assignment.get( oldIndex );
        _assignment.erase( oldIndex );
    }

    if ( _lowerBounds.exists( oldIndex ) )
    {
        _lowerBounds[newIndex] = _lowerBounds.get( oldIndex );
        _lowerBounds.erase( oldIndex );
    }

    if ( _upperBounds.exists( oldIndex ) )
    {
        _upperBounds[newIndex] = _upperBounds.get( oldIndex );
        _upperBounds.erase( oldIndex );
    }

    for ( auto &disjunct : _disjuncts )
        disjunct.updateVariableIndex( oldIndex, newIndex );

    extractParticipatingVariables();
}

void DisjunctionConstraint::eliminateVariable( unsigned /* variable */, double /* fixedValue */ )
{
    throw MarabouError( MarabouError::FEATURE_NOT_YET_SUPPORTED,
                        "Eliminate variable from a DisjunctionConstraint" );
}

bool DisjunctionConstraint::constraintObsolete() const
{
    return false;
}

void DisjunctionConstraint::getEntailedTightenings( List<Tightening> &tightenings ) const
{
    for ( const auto &var : _participatingVariables )
    {
        double bound = getMinLowerBound( var );
        if ( FloatUtils::isFinite( bound ) )
            tightenings.append( Tightening( var, bound, Tightening::LB ) );
        bound = getMaxUpperBound( var );
        if ( FloatUtils::isFinite( bound ) )
            tightenings.append( Tightening( var, bound, Tightening::UB ) );
    }
}

String DisjunctionConstraint::serializeToString() const
{
    String s = "disj,";
    s += Stringf( "%u,", _disjuncts.size() );
    for ( const auto &disjunct : _disjuncts )
    {
        s += Stringf( "%u,", disjunct.getBoundTightenings().size() );
        for ( const auto &bound : disjunct.getBoundTightenings() )
        {
            if ( bound._type == Tightening::LB )
                s += Stringf( "l,%u,%f,", bound._variable, bound._value );
            else if ( bound._type == Tightening::UB )
                s += Stringf( "u,%u,%f,", bound._variable, bound._value );
        }
        s += Stringf( "%u,", disjunct.getEquations().size() );
        for ( const auto &equation : disjunct.getEquations() )
        {
            if ( equation._type == Equation::LE )
                s += Stringf( "l," );
            else if ( equation._type == Equation::GE )
                s += Stringf( "g," );
            else
                s += Stringf( "e," );
            s += Stringf( "%u,", equation._addends.size() );
            for ( const auto &addend : equation._addends )
            {
                s += Stringf( "%f,%u,", addend._coefficient, addend._variable );
            }
            s += Stringf( "%f,", equation._scalar );
        }
    }
    return s;
}

void DisjunctionConstraint::extractParticipatingVariables()
{
    _participatingVariables.clear();

    for ( const auto &disjunct : _disjuncts )
    {
        // Extract from bounds
        for ( const auto &bound : disjunct.getBoundTightenings() )
            _participatingVariables.insert( bound._variable );

        // Extract from equations
        for ( const auto &equation : disjunct.getEquations() )
        {
            for ( const auto &addend : equation._addends )
                _participatingVariables.insert( addend._variable );
        }
    }
}

bool DisjunctionConstraint::disjunctSatisfied( const PiecewiseLinearCaseSplit &disjunct ) const
{
    // Check whether the bounds are satisfied
    for ( const auto &bound : disjunct.getBoundTightenings() )
    {
        if ( bound._type == Tightening::LB )
        {
            if ( getAssignment( bound._variable ) < bound._value )
                return false;
        }
        else
        {
            if ( getAssignment( bound._variable ) > bound._value )
                return false;
        }
    }

    // Check whether the equations are satisfied
    for ( const auto &equation : disjunct.getEquations() )
    {
        double result = 0;
        for ( const auto &addend : equation._addends )
            result += addend._coefficient * getAssignment( addend._variable );

        if ( !FloatUtils::areEqual( result, equation._scalar ) )
            return false;
    }

    return true;
}

void DisjunctionConstraint::updateFeasibleDisjuncts()
{
    _feasibleDisjuncts.clear();

    for ( unsigned ind = 0; ind < _disjuncts.size(); ++ind )
    {
        if ( disjunctIsFeasible( ind ) )
            _feasibleDisjuncts.append( ind );
        else if ( _cdInfeasibleCases && !isCaseInfeasible( indToPhaseStatus( ind ) ) )
            markInfeasible( indToPhaseStatus( ind ) );
    }

    if ( _feasibleDisjuncts.size() == 0 )
        throw InfeasibleQueryException();
}

bool DisjunctionConstraint::disjunctIsFeasible( unsigned ind ) const
{
    if ( _cdInfeasibleCases && isCaseInfeasible( indToPhaseStatus( ind ) ) )
        return false;

    return caseSplitIsFeasible( _disjuncts.get( ind ) );
}

bool DisjunctionConstraint::caseSplitIsFeasible( const PiecewiseLinearCaseSplit &disjunct ) const
{
    for ( const auto &bound : disjunct.getBoundTightenings() )
    {
        if ( bound._type == Tightening::LB )
        {
            if ( existsUpperBound( bound._variable ) &&
                 getUpperBound( bound._variable ) < bound._value )
                return false;
        }
        else
        {
            if ( existsLowerBound( bound._variable ) &&
                 getLowerBound( bound._variable ) > bound._value )
                return false;
        }
    }

    return true;
}

List<PiecewiseLinearCaseSplit> DisjunctionConstraint::getFeasibleDisjuncts() const
{
    List<PiecewiseLinearCaseSplit> validDisjuncts = List<PiecewiseLinearCaseSplit>();

    for ( const auto &feasibleDisjunct : _feasibleDisjuncts )
        validDisjuncts.append( _disjuncts.get( feasibleDisjunct ) );

    return validDisjuncts;
}

bool DisjunctionConstraint::removeFeasibleDisjunct( const PiecewiseLinearCaseSplit &disjunct )
{
    for ( unsigned i = 0; i < _disjuncts.size(); ++i )
        if ( _disjuncts[i] == disjunct )
        {
            _feasibleDisjuncts.erase( i );
            return true;
        }

    return false;
}

bool DisjunctionConstraint::addFeasibleDisjunct( const PiecewiseLinearCaseSplit &disjunct )
{
    for ( unsigned i = 0; i < _disjuncts.size(); ++i )
        if ( _disjuncts[i] == disjunct )
        {
            _feasibleDisjuncts.append( i );
            return true;
        }

    return false;
}

// No aux vars in disjunction constraint, so the function is suppressed
void DisjunctionConstraint::addTableauAuxVar( unsigned /* tableauAuxVar */,
                                              unsigned /* constraintAuxVar */ )
{
}

double DisjunctionConstraint::getMinLowerBound( unsigned int var ) const
{
    if ( !participatingVariable( var ) )
    {
        return FloatUtils::negativeInfinity();
    }

    double minLowerBound = FloatUtils::infinity();
    for ( const auto &disjunct : _disjuncts )
    {
        bool foundLowerBound = false;
        for ( const auto &bound : disjunct.getBoundTightenings() )
        {
            if ( bound._variable == var && bound._type == Tightening::LB )
            {
                foundLowerBound = true;
                minLowerBound = FloatUtils::min( minLowerBound, bound._value );
            }
        }

        if ( !foundLowerBound )
        {
            return FloatUtils::negativeInfinity();
        }
    }

    return minLowerBound;
}

double DisjunctionConstraint::getMaxUpperBound( unsigned int var ) const
{
    if ( !participatingVariable( var ) )
    {
        return FloatUtils::infinity();
    }

    double maxUpperBound = FloatUtils::negativeInfinity();
    for ( const auto &disjunct : _disjuncts )
    {
        bool foundUpperBound = false;
        for ( const auto &bound : disjunct.getBoundTightenings() )
        {
            if ( bound._variable == var && bound._type == Tightening::UB )
            {
                foundUpperBound = true;
                maxUpperBound = FloatUtils::max( maxUpperBound, bound._value );
            }
        }

        if ( !foundUpperBound )
        {
            return FloatUtils::infinity();
        }
    }

    return maxUpperBound;
}
