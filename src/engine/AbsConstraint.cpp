

#include "AbsConstraint.h"
#include "ITableau.h"
#include "FloatUtils.h"


AbsConstraint::AbsConstraint(unsigned b, unsigned f)
{
    _b = b;
    _f = f;
}


PiecewiseLinearConstraint *AbsConstraint::duplicateConstraint() const
{
    AbsConstraint *clone = new AbsConstraint( _b, _f );
    *clone = *this;
    return clone;
}

void AbsConstraint::restoreState( const PiecewiseLinearConstraint *state )
{
    const AbsConstraint *relu = dynamic_cast<const AbsConstraint *>( state );
    *this = *relu;
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


bool AbsConstraint::participatingVariable( unsigned variable ) const
{
    return ( variable == _b ) || ( variable == _f );
}

bool AbsConstraint::satisfied() const
{
    if ( !( _assignment.exists( _b ) && _assignment.exists( _f ) ) )
        throw AbsError( AbsError::PARTICIPATING_VARIABLES_ABSENT );

    double bValue = _assignment.get( _b );
    double fValue = _assignment.get( _f );
    //todo: add tolerance

    if ( FloatUtils::areEqual( bValue, fValue, GlobalConfiguration::RELU_CONSTRAINT_COMPARISON_TOLERANCE );)
        return false;
    else
        return true;
}