#include <stdio.h>


#include "AbsConstraint.h"
#include "ITableau.h"
#include "FloatUtils.h"
#include "AbsError.h"
#include "Debug.h"
#include "PiecewiseLinearCaseSplit.h"
#include "MStringf.h"
#include "Statistics.h"
#include "ConstraintBoundTightener.h"



AbsConstraint::AbsConstraint(unsigned b, unsigned f )
//var names
        : _b( b )
        , _f( f )
        // one of our variables terminated
        , _haveEliminatedVariables( false )
{
    //bound tightening
    setPhaseStatus( PhaseStatus::PHASE_NOT_FIXED );
}

//TODO: add serialize?

PiecewiseLinearConstraint *AbsConstraint::duplicateConstraint() const
{
    AbsConstraint *clone = new AbsConstraint( _b, _f );
    *clone = *this;
    return clone;
}

void AbsConstraint::restoreState( const PiecewiseLinearConstraint *state )
{
    const AbsConstraint *abs = dynamic_cast<const AbsConstraint *>( state );
    *this = *abs;
}

void AbsConstraint::registerAsWatcher( ITableau *tableau )
{
    tableau->registerToWatchVariable( this, _b );
    tableau->registerToWatchVariable( this, _f );
}

void AbsConstraint::unregisterAsWatcher( ITableau *tableau )
{
    tableau->unregisterToWatchVariable( this, _b );
    tableau->unregisterToWatchVariable( this, _f );
}


/*
  The variable watcher notification callbacks, about a change in a variable's value or bounds.
  suppose A < x_b < B, C < x_f < D
  if variable == x_b then:
    A < 0 & B < 0 then: max{|B|, C} < x_f < min{|A|, D}
    A < 0 & B > 0 then: max{0, C} < x_f < min{max{|A|, B}, D}
    A > 0 & B < 0 then: ------
    A > 0 & B > 0 then: max{A, C} < x_f < min{B, D}
  if variable == x_f then:
    C > 0 & D > 0 then: min{max{-D, A}, max{A, C}} < x_b < max{min{-C, A}, min{B, D}}

*/
void AbsConstraint::notifyVariableValue( unsigned variable, double value)
{
    _assignment[variable] = value;
}

void AbsConstraint::notifyLowerBound( unsigned variable, double bound)
{
    if ( _statistics )
        _statistics->incNumBoundNotificationsPlConstraints();

    //update the input variable bound
    if ( _lowerBounds.exists( variable ) && !FloatUtils::gt( bound, _lowerBounds[variable] ) )
        return;
    if ((variable == _f) && !_lowerBounds.exists( variable ) )
    {
        _lowerBounds[variable] = 0.0;
    }
    if ((variable == _f) && !FloatUtils::isPositive( bound ) )
    {
        return;
    }
    else{
        _lowerBounds[variable] = bound;
    }


    //fix phase, only by x_b because x_b <= x_f
    if ( (variable == _b) && !FloatUtils::isNegative( bound ) )
        setPhaseStatus( PhaseStatus::PHASE_POSITIVE );

    //update partner's bound
    if ( isActive() && _constraintBoundTightener )
    {
        if ( variable == _b)
        {
            if( bound < 0)
            {
                double newUpperBound = FloatUtils::abs(bound);
                if ( _upperBounds.exists( _f ) )
                {
                    newUpperBound = FloatUtils::min( _upperBounds[_f], newUpperBound );
                }
                _constraintBoundTightener->registerTighterUpperBound( _f, newUpperBound );
            }
            else if ( bound >= 0 )
            {
                double newLowerBound = bound;
                if ( _lowerBounds.exists( _f ) )
                {
                    newLowerBound = FloatUtils::max( _lowerBounds[_f], newLowerBound );
                }
                _constraintBoundTightener->registerTighterLowerBound( _f, newLowerBound );
            }
        }
        else if ( variable == _f)
        {
            double newUpperBound = -1 * bound;
            double newLowerBound = bound;
            if ( _lowerBounds.exists( _b ) )
            {
                newUpperBound = FloatUtils::min( _upperBounds[_b], newUpperBound );
                newLowerBound = FloatUtils::max( _lowerBounds[_b], newLowerBound );
            }
            _constraintBoundTightener->registerTighterLowerBound( _b, newLowerBound );
            _constraintBoundTightener->registerTighterUpperBound( _b, newUpperBound );

        }
    }
}

void AbsConstraint::notifyUpperBound(  unsigned variable, double bound )
{
    if ( _statistics )
        _statistics->incNumBoundNotificationsPlConstraints();

    //update the input variable bound
    if ( _upperBounds.exists( variable ) && !FloatUtils::lt( bound, _upperBounds[variable] ) )
        return;
    if ((variable == _f) && FloatUtils::isZero( bound ) && !_upperBounds.exists( variable ) )
    {
        _upperBounds[variable] = 0.0;
        //todo: lower bound f
        _constraintBoundTightener->registerTighterUpperBound( _b, 0.0 );
        _constraintBoundTightener->registerTighterLowerBound( _b, 0.0 );
        setPhaseStatus( PhaseStatus::PHASE_POSITIVE );
    }
    else if ((variable == _f) && FloatUtils::isNegative( bound ) )
    {
        return;
    }
    else {
        _upperBounds[variable] = bound;
    }

    //fix phase, only by x_b because x_b <= x_f
    if ( ( variable == _b ) && !FloatUtils::isPositive( bound ) )
        setPhaseStatus( PhaseStatus::PHASE_NEGATIVE );

    //update partner's bound
    if ( isActive() && _constraintBoundTightener )
    {
        if ( variable == _b)
        {
            if( bound < 0)
            {
                double newLowerBound = FloatUtils::abs(bound);
                if ( _lowerBounds.exists( _f ) )
                {
                    newLowerBound = FloatUtils::max( _lowerBounds[_f], newLowerBound );
                }
                _constraintBoundTightener->registerTighterLowerBound( _f, newLowerBound );
            }
            else if ( bound >= 0 )
            {
                double newUpperBound = bound;
                if ( _upperBounds.exists( _f ) )
                {
                    newUpperBound = FloatUtils::min( _upperBounds[_f], newUpperBound );
                }
                _constraintBoundTightener->registerTighterUpperBound( _f, newUpperBound );
            }
        }
        else if ( variable == _f)
        {
            double newUpperBound = bound;
            double newLowerBound = -1 * bound;
            if ( _lowerBounds.exists( _b ) )
            {
                newUpperBound = FloatUtils::min( _upperBounds[_b], newUpperBound );
                newLowerBound = FloatUtils::max( _lowerBounds[_b], newLowerBound );
            }
            _constraintBoundTightener->registerTighterLowerBound( _b, newLowerBound );
            _constraintBoundTightener->registerTighterUpperBound( _b, newUpperBound );
        }
    }
}


bool AbsConstraint::participatingVariable(unsigned variable ) const
{
    return ( variable == _b ) || ( variable == _f );
}

List<unsigned> AbsConstraint::getParticipatingVariables() const
{
    return List<unsigned>( { _b, _f } );
}

bool AbsConstraint::satisfied() const
{
    if ( !( _assignment.exists( _b ) && _assignment.exists( _f ) ) )
        throw AbsError( AbsError::PARTICIPATING_VARIABLES_ABSENT );

    double bValue = _assignment.get( _b );
    double fValue = _assignment.get( _f );

    // Possible violations:
    //   1. f is negative
    //   2. f is positive, abs(b) and f are not equal

    if ( FloatUtils::isNegative( fValue ) )
        return false;

    return FloatUtils::areEqual( FloatUtils::abs(bValue), fValue, GlobalConfiguration::ABS_CONSTRAINT_COMPARISON_TOLERANCE);

}


List<PiecewiseLinearConstraint::Fix> AbsConstraint::getPossibleFixes() const
{
    ASSERT( !satisfied() );
    ASSERT( _assignment.exists( _b ) );
    ASSERT( _assignment.exists( _f ) );

    double bValue = _assignment.get( _b );
    double fValue = _assignment.get( _f );

    ASSERT( !FloatUtils::isNegative( fValue ) );

    List<PiecewiseLinearConstraint::Fix> fixes;

    // Possible violations:
    //   1. f is positive, b is positive, b and f are unequal
    //   2. f is positive, b is negative, -b and f are unequal

    fixes.append(PiecewiseLinearConstraint::Fix(_b, fValue));
    fixes.append(PiecewiseLinearConstraint::Fix(_b, -fValue));
    fixes.append(PiecewiseLinearConstraint::Fix(_f, abs(bValue)));
    return fixes;
}

List<PiecewiseLinearConstraint::Fix> AbsConstraint::getSmartFixes( __attribute__((unused)) ITableau *tableau ) const
{
    return getPossibleFixes();
}

List<PiecewiseLinearCaseSplit> AbsConstraint::getCaseSplits() const
{
    if ( _phaseStatus != PhaseStatus::PHASE_NOT_FIXED )
        throw AbsError( AbsError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT );

    List<PiecewiseLinearCaseSplit> splits;
    splits.append( getNegativeSplit() );
    splits.append( getPositiveSplit() );

    //TODO: add some heuristic
    return splits;
}

PiecewiseLinearCaseSplit AbsConstraint::getNegativeSplit() const {
    // Negative phase: b <=0, b + f = 0
    PiecewiseLinearCaseSplit negativePhase;

    //b <= 0
    negativePhase.storeBoundTightening(Tightening(_b, 0.0, Tightening::UB));

    //b + f = 0
    Equation negativeEquation(Equation::EQ);
    negativeEquation.addAddend(1, _b);
    negativeEquation.addAddend(1, _f);
    negativeEquation.setScalar(0);
    negativePhase.addEquation(negativeEquation);

    return negativePhase;
}

PiecewiseLinearCaseSplit AbsConstraint::getPositiveSplit() const {
    // Positive phase: b >= 0, b - f = 0
    PiecewiseLinearCaseSplit positivePhase;

    //b >= 0
    positivePhase.storeBoundTightening(Tightening(_b, 0.0, Tightening::LB));

    //b - f = 0
    Equation positiveEquation(Equation::EQ);
    positiveEquation.addAddend(1, _b);
    positiveEquation.addAddend(-1, _f);
    positiveEquation.setScalar(0);
    positivePhase.addEquation(positiveEquation);

    return positivePhase;
}


bool AbsConstraint::phaseFixed() const
{
    return _phaseStatus != PhaseStatus::PHASE_NOT_FIXED;
}

PiecewiseLinearCaseSplit AbsConstraint::getValidCaseSplit() const
{
    ASSERT( _phaseStatus != PhaseStatus::PHASE_NOT_FIXED );

    if ( _phaseStatus == PhaseStatus::PHASE_POSITIVE )
        return getPositiveSplit();

    return getNegativeSplit();
}


void AbsConstraint::eliminateVariable(__attribute__((unused)) unsigned variable,
                                      __attribute__((unused)) double fixedValue )
{
    ASSERT( variable == _b || variable == _f );

    // In a Abs constraint, if a variable is removed the entire constraint can be discarded.
    _haveEliminatedVariables = true;
}

void AbsConstraint::updateVariableIndex( unsigned oldIndex, unsigned newIndex )
{
    ASSERT( oldIndex == _b || oldIndex == _f );
    ASSERT( !_assignment.exists( newIndex ) &&
            !_lowerBounds.exists( newIndex ) &&
            !_upperBounds.exists( newIndex ) &&
            newIndex != _b && newIndex != _f );
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

    if ( oldIndex == _b )
        _b = newIndex;
    else
        _f = newIndex;
}

/**
 *  check if the constraint is redundant
 * @return true iff and the constraint has become obsolote
 * as a result of variable eliminations.
 */
bool AbsConstraint::constraintObsolete() const
{
    return _haveEliminatedVariables;
}

/*
  Get the tightenings entailed by the constraint.
  suppose A < x_b < B, C < x_f < D, remainder C >= 0 ,D > 0
    A > 0 & B > 0 & C >= 0 then: max{A, C} < x_b, x_f < min{B, D}
    A > 0 & B < 0 & C >= 0 then: ------
    A < 0 & B > 0 & C > 0  then: can't decide need to split
    A < 0 & B > 0 & C = 0  then: max{-D, A} < x_b < min{B,D} & 0 < x_f < min{max{|A|, B}, D}
    A < 0 & B < 0 & c >= 0 then: max{-D, A} < x_b < min{B,-C} & max{|B|, C} < x_f < min{|A|, D}
*/
void AbsConstraint::getEntailedTightenings( List<Tightening> &tightenings ) const
{
    if (! _lowerBounds.exists( _f ))
    {
//        printf("index:%d \n",_f);
        tightenings.append( Tightening( _f, 0.0, Tightening::LB) );
//        printf("index:%d, %d \n",_f, _lowerBounds.exists( _f ));
    }

//    printf("index:%d , the bound: %d, %d, %d,%d,",_f, _lowerBounds.exists( _b ),  _lowerBounds.exists( _f ),
//          _upperBounds.exists( _b ), _upperBounds.exists( _f ));
    ASSERT( _lowerBounds.exists( _b ) && _lowerBounds.exists( _f ) &&
            _upperBounds.exists( _b ) && _upperBounds.exists( _f ) );



    // Upper bounds
    double bUpperBound = _upperBounds[_b];
    double fUpperBound = _upperBounds[_f];
    // Lower bounds
    double bLowerBound = _lowerBounds[_b];
    double fLowerBound = _lowerBounds[_f];

    if (!FloatUtils::isNegative( bLowerBound ) & !FloatUtils::isNegative( bUpperBound ) & !FloatUtils::isNegative( fLowerBound ))
    {
        // update lower bound x_f or x_b
        if ( FloatUtils::lt( fLowerBound, bLowerBound) )
        {
            tightenings.append( Tightening( _f, bLowerBound, Tightening::LB ) );
        }
        else if ( FloatUtils::lt( bLowerBound, fLowerBound ) )
        {
            tightenings.append( Tightening( _b, fLowerBound, Tightening::LB ) );
        }

        // update upper bound x_f or x_b
        if ( FloatUtils::lt( bUpperBound, fUpperBound ) )
            tightenings.append( Tightening( _f, bUpperBound, Tightening::UB ) );
        else if ( FloatUtils::lt(  fUpperBound, bUpperBound ) )
            tightenings.append( Tightening( _b, fUpperBound, Tightening::UB ) );
    }

    if (FloatUtils::isNegative(bLowerBound) & !FloatUtils::isNegative(bUpperBound) & FloatUtils::isZero(fLowerBound))
    {
        //have to be overlap
        // update lower bound x_b
        if ( FloatUtils::gt(-1*fUpperBound, bLowerBound ) )
            tightenings.append( Tightening( _b, -1*fUpperBound , Tightening::LB ) );
//        tightenings.append( Tightening( _f, 0.0 , Tightening::LB ) );

        // update upper bound x_f and x_b
        if ( FloatUtils::lt( fUpperBound, bUpperBound) )
            tightenings.append( Tightening( _b, fUpperBound, Tightening::UB ) );
        double tempBound = FloatUtils::max(FloatUtils::abs(bLowerBound) , bUpperBound);
        if ( FloatUtils::lt( tempBound, fUpperBound) )
            tightenings.append( Tightening( _f, tempBound, Tightening::UB ) );
    }

    if (FloatUtils::isNegative(bLowerBound) & !FloatUtils::isNegative(bUpperBound) & !FloatUtils::isZero(fLowerBound))
    {
        if ( FloatUtils::gt(FloatUtils::abs(bLowerBound),fUpperBound ))
            tightenings.append( Tightening( _b,-1*fUpperBound , Tightening::LB ));
        if ( FloatUtils::gt(fLowerBound, FloatUtils::abs(bLowerBound)))
            tightenings.append( Tightening( _b,fLowerBound , Tightening::LB ) );

        // update upper bound x_f and x_b
        if ( FloatUtils::lt( fUpperBound, bUpperBound))
            tightenings.append( Tightening( _b, fUpperBound, Tightening::UB ) );
        else if ( FloatUtils::gt( fLowerBound, bUpperBound))
            tightenings.append( Tightening( _b, -1*fLowerBound, Tightening::UB ) );

        double tempBound = FloatUtils::max(FloatUtils::abs(bLowerBound) , bUpperBound);
        if ( FloatUtils::lt( tempBound, fUpperBound) )
            tightenings.append( Tightening( _f, tempBound, Tightening::UB ) );
    }

    if (FloatUtils::isNegative(bLowerBound) & FloatUtils::isNegative(bUpperBound) & !FloatUtils::isNegative(fLowerBound))
    {
        //there is no overlap
//        if(FloatUtils::lt( fUpperBound ,FloatUtils::abs(bUpperBound)) ||
//        FloatUtils::lt(FloatUtils::abs(bLowerBound), fLowerBound)) {
//            return;
//        }
        // update lower bound x_f and x_b
        if ( FloatUtils::gt(-1*fUpperBound, bLowerBound ) )
            tightenings.append( Tightening( _b, -1*fUpperBound , Tightening::LB ) );
        if (FloatUtils::gt(FloatUtils::abs(bUpperBound), fLowerBound ))
            tightenings.append( Tightening( _f, FloatUtils::abs(bUpperBound), Tightening::LB ) );

        // update upper bound x_f and x_b
        if ( FloatUtils::lt( -fLowerBound, bUpperBound) )
            tightenings.append( Tightening( _b, -fLowerBound, Tightening::UB ) );
        if ( FloatUtils::lt( FloatUtils::abs(bLowerBound), fUpperBound) )
            tightenings.append( Tightening( _f, FloatUtils::abs(bLowerBound), Tightening::UB ) );
    }
}

void AbsConstraint::getAuxiliaryEquations( __attribute__((unused)) List<Equation> &newEquations ) const {}

String AbsConstraint::serializeToString() const
{
    // Output format is: Abs,f,b
    return Stringf( "Abs,%u,%u", _f, _b );
}

void AbsConstraint::setPhaseStatus( PhaseStatus phaseStatus )
{
    _phaseStatus = phaseStatus;
}