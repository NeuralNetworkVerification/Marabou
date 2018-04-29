/*********************                                                        */
/*! \file Tableau.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "BasisFactorizationFactory.h"
#include "Debug.h"
#include "EntrySelectionStrategy.h"
#include "Equation.h"
#include "FloatUtils.h"
#include "ICostFunctionManager.h"
#include "MStringf.h"
#include "PiecewiseLinearCaseSplit.h"
#include "ReluplexError.h"
#include "Tableau.h"
#include "TableauRow.h"
#include "TableauState.h"

#include <string.h>

Tableau::Tableau()
    : _A( NULL )
    , _a( NULL )
    , _changeColumn( NULL )
    , _pivotRow( NULL )
    , _b( NULL )
    , _work( NULL )
    , _unitVector( NULL )
    , _basisFactorization( NULL )
    , _multipliers( NULL )
    , _basicIndexToVariable( NULL )
    , _nonBasicIndexToVariable( NULL )
    , _variableToIndex( NULL )
    , _nonBasicAssignment( NULL )
    , _lowerBounds( NULL )
    , _upperBounds( NULL )
    , _boundsValid( true )
    , _basicAssignment( NULL )
    , _basicStatus( NULL )
    , _basicAssignmentStatus( ITableau::BASIC_ASSIGNMENT_INVALID )
    , _statistics( NULL )
    , _costFunctionManager( NULL )
{
}

Tableau::~Tableau()
{
    freeMemoryIfNeeded();
}

void Tableau::freeMemoryIfNeeded()
{
    if ( _A )
    {
        delete[] _A;
        _A = NULL;
    }

    if ( _changeColumn )
    {
        delete[] _changeColumn;
        _changeColumn = NULL;
    }

    if ( _pivotRow )
    {
        delete _pivotRow;
        _pivotRow = NULL;
    }

    if ( _b )
    {
        delete[] _b;
        _b = NULL;
    }

    if ( _unitVector )
    {
        delete[] _unitVector;
        _unitVector = NULL;
    }

    if ( _multipliers )
    {
        delete[] _multipliers;
        _multipliers = NULL;
    }

    if ( _basicIndexToVariable )
    {
        delete[] _basicIndexToVariable;
        _basicIndexToVariable = NULL;
    }

    if ( _variableToIndex )
    {
        delete[] _variableToIndex;
        _variableToIndex = NULL;
    }

    if ( _nonBasicIndexToVariable )
    {
        delete[] _nonBasicIndexToVariable;
        _nonBasicIndexToVariable = NULL;
    }

    if ( _nonBasicAssignment )
    {
        delete[] _nonBasicAssignment;
        _nonBasicAssignment = NULL;
    }

    if ( _lowerBounds )
    {
        delete[] _lowerBounds;
        _lowerBounds = NULL;
    }

    if ( _upperBounds )
    {
        delete[] _upperBounds;
        _upperBounds = NULL;
    }

    if ( _basicAssignment )
    {
        delete[] _basicAssignment;
        _basicAssignment = NULL;
    }

    if ( _basicStatus )
    {
        delete[] _basicStatus;
        _basicStatus = NULL;
    }

    if ( _basisFactorization )
    {
        delete _basisFactorization;
        _basisFactorization = NULL;
    }

    if ( _work )
    {
        delete[] _work;
        _work = NULL;
    }
}

void Tableau::setDimensions( unsigned m, unsigned n )
{
    _m = m;
    _n = n;

    _A = new double[n*m];
    if ( !_A )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::A" );
    std::fill( _A, _A + ( n * m ), 0.0 );

    _changeColumn = new double[m];
    if ( !_changeColumn )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::changeColumn" );

    _pivotRow = new TableauRow( n-m );
    if ( !_pivotRow )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::pivotRow" );

    _b = new double[m];
    if ( !_b )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::b" );

    _unitVector = new double[m];
    if ( !_unitVector )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::unitVector" );

    _multipliers = new double[m];
    if ( !_multipliers )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::multipliers" );

    _basicIndexToVariable = new unsigned[m];
    if ( !_basicIndexToVariable )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::basicIndexToVariable" );

    _variableToIndex = new unsigned[n];
    if ( !_variableToIndex )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::variableToIndex" );

    _nonBasicIndexToVariable = new unsigned[n-m];
    if ( !_nonBasicIndexToVariable )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::nonBasicIndexToVariable" );

    _nonBasicAssignment = new double[n-m];
    if ( !_nonBasicAssignment )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::nonBasicAssignment" );

    _lowerBounds = new double[n];
    if ( !_lowerBounds )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::lowerBounds" );
    std::fill_n( _lowerBounds, n, FloatUtils::negativeInfinity() );

    _upperBounds = new double[n];
    if ( !_upperBounds )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::upperBounds" );
    std::fill_n( _upperBounds, n, FloatUtils::infinity() );

    _basicAssignment = new double[m];
    if ( !_basicAssignment )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::assignment" );

    _basicStatus = new unsigned[m];
    if ( !_basicStatus )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::basicStatus" );

    _basisFactorization = BasisFactorizationFactory::createBasisFactorization( _m );
    if ( !_basisFactorization )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::basisFactorization" );

    _work = new double[m];
    if ( !_work )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::work" );
}

void Tableau::setEntryValue( unsigned row, unsigned column, double value )
{
    _A[(column * _m) + row] = value;
}

void Tableau::markAsBasic( unsigned variable )
{
    _basicVariables.insert( variable );
}

void Tableau::assignIndexToBasicVariable( unsigned variable, unsigned index )
{
    _basicIndexToVariable[index] = variable;
    _variableToIndex[variable] = index;
}

void Tableau::initializeTableau()
{
    unsigned nonBasicIndex = 0;

    // Assign variable indices
    for ( unsigned i = 0; i < _n; ++i )
    {
        if ( !_basicVariables.exists( i ) )
        {
            _nonBasicIndexToVariable[nonBasicIndex] = i;
            _variableToIndex[i] = nonBasicIndex;
            ++nonBasicIndex;
        }
    }
    ASSERT( nonBasicIndex == _n - _m );

    // Set non-basics to lower bounds, don't update basics - they will be computed later
    for ( unsigned i = 0; i < _n - _m; ++i )
    {
        unsigned nonBasic = _nonBasicIndexToVariable[i];
        setNonBasicAssignment( nonBasic, _lowerBounds[nonBasic], false );
    }

    // Initialize B0
    double *B0 = new double[_m * _n];
    if ( !B0 )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::initializeTableau::B0" );

    for ( unsigned row = 0; row < _m; ++row )
    {
        for ( unsigned col = 0; col < _m; ++col )
        {
            // B0 is row-major, and A is col-major.
            B0[_m * row + col] = _A[_m * _basicIndexToVariable[col] + row];
        }
    }

    _basisFactorization->setBasis( B0 );
    delete[] B0;

    // Compute assignment
    computeAssignment();
}

void Tableau::computeAssignment()
{
    /*
      The basic assignment is given by the formula:

      xB = inv(B) * b - inv(B) * AN * xN
         = inv(B) * ( b - AN * xN )
                      -----------
                           y

      where B is the basis matrix, AN is the non-basis matrix, xN are
      the value of the non basic variables and b is the original
      right hand side.

      We first compute y (stored in _work), and then do an FTRAN pass to solve B*xB = y
    */

    memcpy( _work, _b, sizeof(double) * _m );

    // Compute a linear combination of the columns of AN
    const double *ANColumn;
    for ( unsigned i = 0; i < _n - _m; ++i )
    {
        unsigned var = _nonBasicIndexToVariable[i];
        double value = _nonBasicAssignment[i];

        ANColumn = _A + ( var * _m );
        for ( unsigned j = 0; j < _m; ++j )
            _work[j] -= ANColumn[j] * value;
    }

    // Solve B*xB = y by performing a forward transformation
    _basisFactorization->forwardTransformation( _work, _basicAssignment );

    computeBasicStatus();

    _basicAssignmentStatus = ITableau::BASIC_ASSIGNMENT_JUST_COMPUTED;

    // Inform the watchers
    for ( unsigned i = 0; i < _m; ++i )
        notifyVariableValue( _basicIndexToVariable[i], _basicAssignment[i] );
}

void Tableau::computeBasicStatus()
{
    for ( unsigned i = 0; i < _m; ++i )
        computeBasicStatus( i );
}

void Tableau::computeBasicStatus( unsigned basic )
{
    double ub = _upperBounds[_basicIndexToVariable[basic]];
    double lb = _lowerBounds[_basicIndexToVariable[basic]];
    double value = _basicAssignment[basic];

    if ( FloatUtils::gt( value , ub, GlobalConfiguration::BOUND_COMPARISON_TOLERANCE ) )
    {
        _basicStatus[basic] = Tableau::ABOVE_UB;
    }
    else if ( FloatUtils::lt( value , lb, GlobalConfiguration::BOUND_COMPARISON_TOLERANCE ) )
    {
        _basicStatus[basic] = Tableau::BELOW_LB;
    }
    else if ( FloatUtils::areEqual( ub, value, GlobalConfiguration::BOUND_COMPARISON_TOLERANCE ) )
    {
        _basicStatus[basic] = Tableau::AT_UB;
    }
    else if ( FloatUtils::areEqual( lb, value, GlobalConfiguration::BOUND_COMPARISON_TOLERANCE ) )
    {
        _basicStatus[basic] = Tableau::AT_LB;
    }
    else
    {
        _basicStatus[basic] = Tableau::BETWEEN;
    }
}

void Tableau::setLowerBound( unsigned variable, double value )
{
    ASSERT( variable < _n );
    _lowerBounds[variable] = value;
    notifyLowerBound( variable, value );
    checkBoundsValid( variable );
}

void Tableau::setUpperBound( unsigned variable, double value )
{
    ASSERT( variable < _n );
    _upperBounds[variable] = value;
    notifyUpperBound( variable, value );
    checkBoundsValid( variable );
}

double Tableau::getLowerBound( unsigned variable ) const
{
    ASSERT( variable < _n );
    return _lowerBounds[variable];
}

double Tableau::getUpperBound( unsigned variable ) const
{
    ASSERT( variable < _n );
    return _upperBounds[variable];
}

const double *Tableau::getLowerBounds() const
{
    return _lowerBounds;
}

const double *Tableau::getUpperBounds() const
{
    return _upperBounds;
}

double Tableau::getValue( unsigned variable )
{
    if ( !_basicVariables.exists( variable ) )
    {
        // The values of non-basics can be extracted even if the
        // assignment is invalid
        unsigned index = _variableToIndex[variable];
        return _nonBasicAssignment[index];
    }

    return _basicAssignment[_variableToIndex[variable]];
}

unsigned Tableau::basicIndexToVariable( unsigned index ) const
{
    return _basicIndexToVariable[index];
}

unsigned Tableau::nonBasicIndexToVariable( unsigned index ) const
{
    return _nonBasicIndexToVariable[index];
}

unsigned Tableau::variableToIndex( unsigned index ) const
{
    return _variableToIndex[index];
}

void Tableau::setRightHandSide( const double *b )
{
    memcpy( _b, b, sizeof(double) * _m );
}

void Tableau::setRightHandSide( unsigned index, double value )
{
    _b[index] = value;
}

const double *Tableau::getCostFunction() const
{
    return _costFunctionManager->getCostFunction();
}

bool Tableau::basicOutOfBounds( unsigned basic ) const
{
    return basicTooHigh( basic ) || basicTooLow( basic );
}

bool Tableau::basicTooLow( unsigned basic ) const
{
    return _basicStatus[basic] == Tableau::BELOW_LB;
}

bool Tableau::basicTooHigh( unsigned basic ) const
{
    return _basicStatus[basic] == Tableau::ABOVE_UB;
}

bool Tableau::existsBasicOutOfBounds() const
{
    for ( unsigned i = 0; i < _m; ++i )
        if ( basicOutOfBounds( i ) )
            return true;

    return false;
}

void Tableau::computeCostFunction()
{
    _costFunctionManager->computeCoreCostFunction();
}

void Tableau::computeMultipliers( double *rowCoefficients )
{
    _basisFactorization->backwardTransformation( rowCoefficients, _multipliers );
}

unsigned Tableau::getBasicStatus( unsigned basic )
{
    return _basicStatus[_variableToIndex[basic]];
}

unsigned Tableau::getBasicStatusByIndex( unsigned basicIndex )
{
    return _basicStatus[basicIndex];
}

void Tableau::getEntryCandidates( List<unsigned> &candidates ) const
{
    candidates.clear();
    const double *costFunction = _costFunctionManager->getCostFunction();
    for ( unsigned i = 0; i < _n - _m; ++i )
    {
        if ( eligibleForEntry( i, costFunction ) )
            candidates.append( i );
    }
}

void Tableau::setEnteringVariableIndex( unsigned nonBasic )
{
    _enteringVariable = nonBasic;
}

void Tableau::setLeavingVariableIndex( unsigned basic )
{
    _leavingVariable = basic;
}

bool Tableau::eligibleForEntry( unsigned nonBasic, const double *costFunction ) const
{
    // A non-basic variable is eligible for entry if one of the two
    //   conditions holds:
    //
    //   1. It has a negative coefficient in the cost function and it
    //      can increase
    //   2. It has a positive coefficient in the cost function and it
    //      can decrease

    if ( FloatUtils::isZero( costFunction[nonBasic] ) )
        return false;

    bool positive = FloatUtils::isPositive( costFunction[nonBasic] );

    return
        ( positive && nonBasicCanDecrease( nonBasic ) ) ||
        ( !positive && nonBasicCanIncrease( nonBasic ) );
}

bool Tableau::nonBasicCanIncrease( unsigned nonBasic ) const
{
    double max = _upperBounds[_nonBasicIndexToVariable[nonBasic]];
    return FloatUtils::lt( _nonBasicAssignment[nonBasic], max );
}

bool Tableau::nonBasicCanDecrease( unsigned nonBasic ) const
{
    double min = _lowerBounds[_nonBasicIndexToVariable[nonBasic]];
    return FloatUtils::gt( _nonBasicAssignment[nonBasic], min );
}

unsigned Tableau::getEnteringVariable() const
{
    return _nonBasicIndexToVariable[_enteringVariable];
}

unsigned Tableau::getEnteringVariableIndex() const
{
    return _enteringVariable;
}

unsigned Tableau::getLeavingVariable() const
{
    if ( _leavingVariable == _m )
        return _nonBasicIndexToVariable[_enteringVariable];

    return _basicIndexToVariable[_leavingVariable];
}

unsigned Tableau::getLeavingVariableIndex() const
{
    return _leavingVariable;
}

bool Tableau::performingFakePivot() const
{
    return _leavingVariable == _m;
}

void Tableau::performPivot()
{
    if ( _leavingVariable == _m )
    {
        if ( _statistics )
            _statistics->incNumTableauBoundHopping();

        // The entering variable is going to be pressed against its bound.
        bool decrease =
            FloatUtils::isPositive( _costFunctionManager->getCostFunction()[_enteringVariable] );
        unsigned nonBasic = _nonBasicIndexToVariable[_enteringVariable];

        log( Stringf( "Performing 'fake' pivot. Variable x%u jumping to %s bound",
                      _nonBasicIndexToVariable[_enteringVariable],
                      decrease ? "LOWER" : "UPPER" ) );
        log( Stringf( "Current value: %.3lf. Range: [%.3lf, %.3lf]\n",
                      _nonBasicAssignment[_enteringVariable],
                      _lowerBounds[nonBasic], _upperBounds[nonBasic] ) );

        updateAssignmentForPivot();
        return;
    }

    if ( _statistics )
        _statistics->incNumTableauPivots();

    unsigned currentBasic = _basicIndexToVariable[_leavingVariable];
    unsigned currentNonBasic = _nonBasicIndexToVariable[_enteringVariable];

    log( Stringf( "Tableau performing pivot. Entering: %u, Leaving: %u",
                  _nonBasicIndexToVariable[_enteringVariable],
                  _basicIndexToVariable[_leavingVariable] ) );
    log( Stringf( "Leaving variable %s. Current value: %.15lf. Range: [%.15lf, %.15lf]",
                  _leavingVariableIncreases ? "increases" : "decreases",
                  _basicAssignment[_leavingVariable],
                  _lowerBounds[currentBasic], _upperBounds[currentBasic] ) );
    log( Stringf( "Entering variable %s. Current value: %.15lf. Range: [%.15lf, %.15lf]",
                  FloatUtils::isNegative( _costFunctionManager->getCostFunction()[_enteringVariable] ) ?
                  "increases" : "decreases",
                  _nonBasicAssignment[_enteringVariable],
                  _lowerBounds[currentNonBasic], _upperBounds[currentNonBasic] ) );
    log( Stringf( "Change ratio is: %.15lf\n", _changeRatio ) );

    updateAssignmentForPivot();
    updateCostFunctionForPivot();

    // Update the database
    _basicVariables.insert( currentNonBasic );
    _basicVariables.erase( currentBasic );

    // Adjust the tableau indexing
    _basicIndexToVariable[_leavingVariable] = currentNonBasic;
    _nonBasicIndexToVariable[_enteringVariable] = currentBasic;
    _variableToIndex[currentBasic] = _enteringVariable;
    _variableToIndex[currentNonBasic] = _leavingVariable;

    computeBasicStatus( _leavingVariable );

    // Check if the pivot is degenerate and update statistics
    if ( FloatUtils::isZero( _changeRatio ) && _statistics )
        _statistics->incNumTableauDegeneratePivots();

    // Update the basis factorization. The column corresponding to the
    // leaving variable is the one that has changed
    _basisFactorization->pushEtaMatrix( _leavingVariable, _changeColumn );
}

void Tableau::performDegeneratePivot()
{
    if ( _statistics )
    {
        _statistics->incNumTableauDegeneratePivots();
        _statistics->incNumTableauDegeneratePivotsByRequest();
    }

    ASSERT( _enteringVariable < _n - _m );
    ASSERT( _leavingVariable < _m );
    ASSERT( !basicOutOfBounds( _leavingVariable ) );

    unsigned currentBasic = _basicIndexToVariable[_leavingVariable];
    unsigned currentNonBasic = _nonBasicIndexToVariable[_enteringVariable];

    updateCostFunctionForPivot();

    // Update the database
    _basicVariables.insert( currentNonBasic );
    _basicVariables.erase( currentBasic );

    // Adjust the tableau indexing
    _basicIndexToVariable[_leavingVariable] = currentNonBasic;
    _nonBasicIndexToVariable[_enteringVariable] = currentBasic;
    _variableToIndex[currentBasic] = _enteringVariable;
    _variableToIndex[currentNonBasic] = _leavingVariable;

    // Update the basis factorization
    _basisFactorization->pushEtaMatrix( _leavingVariable, _changeColumn );

    // Switch assignment values. No call to notify is required,
    // because values haven't changed.
    double temp = _basicAssignment[_leavingVariable];
    _basicAssignment[_leavingVariable] = _nonBasicAssignment[_enteringVariable];
    _nonBasicAssignment[_enteringVariable] = temp;
    computeBasicStatus( _leavingVariable );
}

double Tableau::ratioConstraintPerBasic( unsigned basicIndex, double coefficient, bool decrease )
{
    unsigned basic = _basicIndexToVariable[basicIndex];
    double ratio = 0.0;

    // Negate the coefficient to go to a more convenient form: basic =
    // coefficient * nonBasic, as opposed to basic + coefficient *
    // nonBasic = 0.
    coefficient = -coefficient;

    ASSERT( !FloatUtils::isZero( coefficient ) );

    if ( ( FloatUtils::isPositive( coefficient ) && decrease ) ||
         ( FloatUtils::isNegative( coefficient ) && !decrease ) )
    {
        // Basic variable is decreasing
        double maxChange;

        if ( _basicStatus[basicIndex] == BasicStatus::ABOVE_UB )
        {
            // Maximal change: hitting the upper bound
            maxChange = _upperBounds[basic] - _basicAssignment[basicIndex];
        }
        else if ( ( _basicStatus[basicIndex] == BasicStatus::BETWEEN ) ||
                  ( _basicStatus[basicIndex] == BasicStatus::AT_UB ) )
        {
            // Maximal change: hitting the lower bound
            maxChange = _lowerBounds[basic] - _basicAssignment[basicIndex];

            // Protect against corner cases where LB = UB, variable is "AT_UB" but
            // due to the tolerance is in fact below LB, and the change becomes positive.
            if ( !FloatUtils::isNegative( maxChange ) )
                maxChange = 0;
        }
        else if ( _basicStatus[basicIndex] == BasicStatus::AT_LB )
        {
            // Variable is formally pressed against a bound. However,
            // maybe it's in the tolerance zone but still greater than
            // the bound.
            maxChange = _lowerBounds[basic] - _basicAssignment[basicIndex];
            if ( !FloatUtils::isNegative( maxChange ) )
                maxChange = 0;
        }
        else
        {
            // Variable is below its lower bound, no constraint here
            maxChange = FloatUtils::negativeInfinity() - _basicAssignment[basicIndex];
        }

        ASSERT( !FloatUtils::isPositive( maxChange ) );
        ratio = maxChange / coefficient;
    }
    else if ( ( FloatUtils::isPositive( coefficient ) && !decrease ) ||
              ( FloatUtils::isNegative( coefficient ) && decrease ) )

    {
        // Basic variable is increasing
        double maxChange;

        if ( _basicStatus[basicIndex] == BasicStatus::BELOW_LB )
        {
            // Maximal change: hitting the lower bound
            maxChange = _lowerBounds[basic] - _basicAssignment[basicIndex];
        }
        else if ( ( _basicStatus[basicIndex] == BasicStatus::BETWEEN ) ||
                  ( _basicStatus[basicIndex] == BasicStatus::AT_LB ) )
        {
            // Maximal change: hitting the upper bound
            maxChange = _upperBounds[basic] - _basicAssignment[basicIndex];

            // Protect against corner cases where LB = UB, variable is "AT_LB" but
            // due to the tolerance is in fact above UB, and the change becomes negative.
            if ( !FloatUtils::isPositive( maxChange ) )
                maxChange = 0;

        }
        else if ( _basicStatus[basicIndex] == BasicStatus::AT_UB )
        {
            // Variable is formally pressed against a bound. However,
            // maybe it's in the tolerance zone but still greater than
            // the bound.
            maxChange = _upperBounds[basic] - _basicAssignment[basicIndex];
            if ( !FloatUtils::isPositive( maxChange ) )
                maxChange = 0;
        }
        else
        {
            // Variable is above its upper bound, no constraint here
            maxChange = FloatUtils::infinity() - _basicAssignment[basicIndex];
        }

        ASSERT( !FloatUtils::isNegative( maxChange ) );
        ratio = maxChange / coefficient;
    }
    else
    {
        ASSERT( false );
    }

    return ratio;
}

void Tableau::pickLeavingVariable()
{
    pickLeavingVariable( _changeColumn );
}

void Tableau::pickLeavingVariable( double *changeColumn )
{
    ASSERT( !FloatUtils::isZero( _costFunctionManager->getCostFunction()[_enteringVariable] ) );
    bool decrease = FloatUtils::isPositive( _costFunctionManager->getCostFunction()[_enteringVariable] );

    DEBUG({
            if ( decrease )
            {
                ASSERTM( nonBasicCanDecrease( _enteringVariable ),
                         "Error! Entering variable needs to decrease but is at its lower bound" );
            }
            else
            {
                ASSERTM( nonBasicCanIncrease( _enteringVariable ),
                         "Error! Entering variable needs to increase but is at its upper bound" );
            }
        });

    double lb = _lowerBounds[_nonBasicIndexToVariable[_enteringVariable]];
    double ub = _upperBounds[_nonBasicIndexToVariable[_enteringVariable]];
    double currentValue = _nonBasicAssignment[_enteringVariable];

    // A marker to show that no leaving variable has been selected
    _leavingVariable = _m;

    if ( decrease )
    {
        // The maximum amount by which the entering variable can
        // decrease, as determined by its bounds. This is a negative
        // value.
        _changeRatio = lb - currentValue;

        // Iterate over the basics that depend on the entering
        // variable and see if any of them imposes a tighter
        // constraint.
        for ( unsigned i = 0; i < _m; ++i )
        {
            if ( !FloatUtils::isZero( changeColumn[i], GlobalConfiguration::PIVOT_CHANGE_COLUMN_TOLERANCE ) )
            {
                double ratio = ratioConstraintPerBasic( i, changeColumn[i], decrease );
                if ( ratio > _changeRatio )
                {
                    _changeRatio = ratio;
                    _leavingVariable = i;
                }
            }
        }

        // Only perform this check if pivot isn't fake
        if ( _leavingVariable != _m )
            _leavingVariableIncreases = FloatUtils::isPositive( changeColumn[_leavingVariable] );
    }
    else
    {
        // The maximum amount by which the entering variable can
        // increase, as determined by its bounds. This is a positive
        // value.
        _changeRatio = ub - currentValue;

        // Iterate over the basics that depend on the entering
        // variable and see if any of them imposes a tighter
        // constraint.
        for ( unsigned i = 0; i < _m; ++i )
        {
            if ( !FloatUtils::isZero( changeColumn[i] ) )
            {
                double ratio = ratioConstraintPerBasic( i, changeColumn[i], decrease );
                if ( ratio < _changeRatio )
                {
                    _changeRatio = ratio;
                    _leavingVariable = i;
                }
            }
        }

        // Only perform this check if pivot isn't fake
        if ( _leavingVariable != _m )
            _leavingVariableIncreases = FloatUtils::isNegative( changeColumn[_leavingVariable] );
    }
}

double Tableau::getChangeRatio() const
{
    return _changeRatio;
}

void Tableau::computeChangeColumn()
{
    // _a gets the entering variable's column in A
    _a = _A + ( _nonBasicIndexToVariable[_enteringVariable] * _m );

    // Compute d = inv(B) * a using the basis factorization
    _basisFactorization->forwardTransformation( _a, _changeColumn );

    // printf( "Leaving variable selection: dumping a\n\t" );
    // for ( unsigned i = 0; i < _m; ++i )
    //     printf( "%lf ", _a[i] );
    // printf( "\n" );

    // printf( "Leaving variable selection: dumping d\n\t" );
    // for ( unsigned i = 0; i < _m; ++i )
    //     printf( "%lf ", _d[i] );

    // printf( "\n" );
}

const double *Tableau::getChangeColumn() const
{
    return _changeColumn;
}

void Tableau::setChangeColumn( const double *column )
{
    memcpy( _changeColumn, column, _m * sizeof(double) );
}

void Tableau::computePivotRow()
{
    getTableauRow( _leavingVariable, _pivotRow );
}

const TableauRow *Tableau::getPivotRow() const
{
    return _pivotRow;
}

bool Tableau::isBasic( unsigned variable ) const
{
    return _basicVariables.exists( variable );
}

void Tableau::setNonBasicAssignment( unsigned variable, double value, bool updateBasics )
{
    ASSERT( !_basicVariables.exists( variable ) );

    unsigned nonBasic = _variableToIndex[variable];
    double delta = value - _nonBasicAssignment[nonBasic];
    _nonBasicAssignment[nonBasic] = value;
    notifyVariableValue( variable, value );

    // If we don't need to update the basics, we are done
    if ( !updateBasics )
        return;

    // Treat this like a form of fake pivot and compute the change column
    _enteringVariable = nonBasic;
    computeChangeColumn();

    // Update all the affected basic variables
    for ( unsigned i = 0; i < _m; ++i )
    {
        if ( FloatUtils::isZero( _changeColumn[i] ) )
            continue;

        _basicAssignment[i] -= _changeColumn[i] * delta;
        notifyVariableValue( _basicIndexToVariable[i], _basicAssignment[i] );

        unsigned oldStatus = _basicStatus[i];
        computeBasicStatus( i );

        // If these updates resulted in a change to the status of some basic variable,
        // the cost function is invalidated
        if ( oldStatus != _basicStatus[i] )
            _costFunctionManager->invalidateCostFunction();

        _basicAssignmentStatus = ITableau::BASIC_ASSIGNMENT_UPDATED;
    }
}

void Tableau::dumpAssignment()
{
    printf( "Dumping assignment\n" );
    for ( unsigned i = 0; i < _n; ++i )
    {
        bool basic = _basicVariables.exists( i );
        printf( "\tx%u  -->  %.5lf [%s]. ", i, getValue( i ), basic ? "B" : "NB" );
        if ( _lowerBounds[i] != FloatUtils::negativeInfinity() )
            printf( "Range: [ %.5lf, ", _lowerBounds[i] );
        else
            printf( "Range: [ -INFTY, " );

        if ( _upperBounds[i] != FloatUtils::infinity() )
            printf( "%.5lf ] ", _upperBounds[i] );
        else
            printf( "INFTY ] " );

        if ( basic && basicOutOfBounds( _variableToIndex[i] ) )
            printf( "*" );

        printf( "\n" );
    }
}

void Tableau::dump() const
{
    printf( "\nDumping A:\n" );
    for ( unsigned i = 0; i < _m; ++i )
    {
        for ( unsigned j = 0; j < _n; ++j )
        {
            printf( "%5.1lf ", _A[j * _m + i] );
        }
        printf( "\n" );
    }
}

unsigned Tableau::getM() const
{
    return _m;
}

unsigned Tableau::getN() const
{
    return _n;
}

void Tableau::getTableauRow( unsigned index, TableauRow *row )
{
    /*
      Let e denote a unit matrix with 1 in its *index* entry.
      A row is then computed by: e * inv(B) * -AN. e * inv(B) is
      solved by invoking BTRAN.
    */

    ASSERT( index < _m );

    std::fill( _unitVector, _unitVector + _m, 0.0 );
    _unitVector[index] = 1;
    computeMultipliers( _unitVector );

    const double *ANColumn;
    for ( unsigned i = 0; i < _n - _m; ++i )
    {
        row->_row[i]._var = _nonBasicIndexToVariable[i];
        ANColumn = _A + ( _nonBasicIndexToVariable[i] * _m );
        row->_row[i]._coefficient = 0;
        for ( unsigned j = 0; j < _m; ++j )
            row->_row[i]._coefficient -= ( _multipliers[j] * ANColumn[j] );
    }

    _basisFactorization->forwardTransformation( _b, _work );
    row->_scalar = _work[index];

    row->_lhs = _basicIndexToVariable[index];
}

const double *Tableau::getA() const
{
    return _A;
}

const double *Tableau::getAColumn( unsigned variable ) const
{
    return _A + ( variable * _m );
}

void Tableau::dumpEquations()
{
    TableauRow row( _n - _m );

    printf( "Dumping tableau equations:\n" );
    for ( unsigned i = 0; i < _m; ++i )
    {
        printf( "x%u = ", _basicIndexToVariable[i] );
        getTableauRow( i, &row );
        row.dump();
        printf( "\n" );
    }
}

void Tableau::storeState( TableauState &state ) const
{
    // Set the dimensions
    state.setDimensions( _m, _n );

    // Store matrix A
    memcpy( state._A, _A, sizeof(double) * _n * _m );

    // Store right hand side vector _b
    memcpy( state._b, _b, sizeof(double) * _m );

    // Store the bounds
    memcpy( state._lowerBounds, _lowerBounds, sizeof(double) *_n );
    memcpy( state._upperBounds, _upperBounds, sizeof(double) *_n );

    // Basic variables
    state._basicVariables = _basicVariables;

    // Store the assignments
    memcpy( state._basicAssignment, _basicAssignment, sizeof(double) *_m );
    memcpy( state._nonBasicAssignment, _nonBasicAssignment, sizeof(double) * ( _n - _m  ) );
    state._basicAssignmentStatus = _basicAssignmentStatus;

    // Store the indices
    memcpy( state._basicIndexToVariable, _basicIndexToVariable, sizeof(unsigned) * _m );
    memcpy( state._nonBasicIndexToVariable, _nonBasicIndexToVariable, sizeof(unsigned) * ( _n - _m ) );
    memcpy( state._variableToIndex, _variableToIndex, sizeof(unsigned) * _n );

    // Store the basis factorization
    _basisFactorization->storeFactorization( state._basisFactorization );

    // Store the _boundsValid indicator
    state._boundsValid = _boundsValid;
}

void Tableau::restoreState( const TableauState &state )
{
    freeMemoryIfNeeded();
    setDimensions( state._m, state._n );

    // Restore matrix A
    memcpy( _A, state._A, sizeof(double) * _n * _m );

    // Restore right hand side vector _b
    memcpy( _b, state._b, sizeof(double) * _m );

    // Restore the bounds and valid status
    // TODO: should notify all the constraints.
    memcpy( _lowerBounds, state._lowerBounds, sizeof(double) *_n );
    memcpy( _upperBounds, state._upperBounds, sizeof(double) *_n );

    // Basic variables
    _basicVariables = state._basicVariables;

    // Restore the assignments
    memcpy( _basicAssignment, state._basicAssignment, sizeof(double) *_m );
    memcpy( _nonBasicAssignment, state._nonBasicAssignment, sizeof(double) * ( _n - _m  ) );
    _basicAssignmentStatus = state._basicAssignmentStatus;

    // Restore the indices
    memcpy( _basicIndexToVariable, state._basicIndexToVariable, sizeof(unsigned) * _m );
    memcpy( _nonBasicIndexToVariable, state._nonBasicIndexToVariable, sizeof(unsigned) * ( _n - _m ) );
    memcpy( _variableToIndex, state._variableToIndex, sizeof(unsigned) * _n );

    // Restore the basis factorization
    _basisFactorization->restoreFactorization( state._basisFactorization );

    // Restore the _boundsValid indicator
    _boundsValid = state._boundsValid;

    computeAssignment();
    _costFunctionManager->initialize();
    computeCostFunction();
}

void Tableau::checkBoundsValid()
{
    _boundsValid = true;
    for ( unsigned i = 0; i < _n ; ++i )
    {
        checkBoundsValid( i );
        if ( !_boundsValid )
            return;
    }
}

void Tableau::checkBoundsValid( unsigned variable )
{
    ASSERT( variable < _n );
    if ( !FloatUtils::lte( _lowerBounds[variable], _upperBounds[variable] ) )
    {
        _boundsValid = false;
        return;
    }
}

bool Tableau::allBoundsValid() const
{
    return _boundsValid;
}

void Tableau::tightenLowerBound( unsigned variable, double value )
{
    ASSERT( variable < _n );

    if ( !FloatUtils::gt( value, _lowerBounds[variable] ) )
        return;

    if ( _statistics )
        _statistics->incNumTightenedBounds();

    setLowerBound( variable, value );

    // Ensure that non-basic variables are within bounds
    unsigned index = _variableToIndex[variable];
    if ( !_basicVariables.exists( variable ) )
    {
        if ( FloatUtils::gt( value, _nonBasicAssignment[index] ) )
            setNonBasicAssignment( variable, value, true );
    }
    else
    {
        // Recompute the status of an affected basic variable
        // If the status changes, invalidate the cost function
        unsigned oldStatus = _basicStatus[index];
        computeBasicStatus( index );
        if ( _basicStatus[index] != oldStatus )
            _costFunctionManager->invalidateCostFunction();
    }
}

void Tableau::tightenUpperBound( unsigned variable, double value )
{
    ASSERT( variable < _n );

    if ( !FloatUtils::lt( value, _upperBounds[variable] ) )
        return;

    if ( _statistics )
        _statistics->incNumTightenedBounds();

    setUpperBound( variable, value );

    // Ensure that non-basic variables are within bounds
    unsigned index = _variableToIndex[variable];
    if ( !_basicVariables.exists( variable ) )
    {
        if ( FloatUtils::lt( value, _nonBasicAssignment[index] ) )
            setNonBasicAssignment( variable, value, true );
    }
    else
    {
        // Recompute the status of an affected basic variable
        // If the status changes, invalidate the cost function
        unsigned oldStatus = _basicStatus[index];
        computeBasicStatus( index );
        if ( _basicStatus[index] != oldStatus )
            _costFunctionManager->invalidateCostFunction();
    }
}

void Tableau::addEquation( const Equation &equation )
{
    // The aux variable in the equation has to be a new variable
    if ( equation._auxVariable != _n )
        throw ReluplexError( ReluplexError::INVALID_EQUATION_ADDED_TO_TABLEAU );

    // Prepare to update the basis factorization.
    // First condense the Etas so that we can access B0 explicitly
    _basisFactorization->makeExplicitBasisAvailable();
    const double *oldB0 = _basisFactorization->getBasis();

    // Allocate a larger basis factorization and copy the old rows of B0
    unsigned newM = _m + 1;
    double *newB0 = new double[newM * newM];
    for ( unsigned i = 0; i < _m; ++i )
        memcpy( newB0 + i * newM, oldB0 + i * _m, sizeof(double) * _m );

    // Zero out the new row and column
    for ( unsigned i = 0; i < newM - 1; ++i )
    {
        newB0[( i * newM ) + newM - 1] = 0.0;
        newB0[( newM - 1 ) * newM + i] = 0.0;
    }
    newB0[( newM - 1 ) * newM + newM - 1] = 1.0;

    // Add an actual row to the talbeau, adjust the data structures
    addRow();
    _costFunctionManager->invalidateCostFunction();

    // Mark the auxiliary variable as basic, add to indices
    _basicVariables.insert( equation._auxVariable );
    _basicIndexToVariable[_m - 1] = equation._auxVariable;
    _variableToIndex[equation._auxVariable] = _m - 1;

    // Populate the new row of A, compute the assignment for the new basic variable
    _b[_m - 1] = equation._scalar;
    _basicAssignment[_m - 1] = equation._scalar;
    double auxCoefficient = 0.0;
    for ( const auto &addend : equation._addends )
    {
        setEntryValue( _m - 1, addend._variable, addend._coefficient );

        if ( addend._variable == equation._auxVariable )
            auxCoefficient = addend._coefficient;
        else
            _basicAssignment[_m - 1] -= addend._coefficient * getValue( addend._variable );

        // The new equation is given over the original non-basic variables.
        // However, some of them may have become basic in previous iterations.
        // Consequently, the last row of B0 may need to be adjusted.
        if ( _basicVariables.exists( addend._variable ) )
        {
            unsigned index = _variableToIndex[addend._variable];
            newB0[( newM - 1 ) * newM + index] = addend._coefficient;
        }
    }
    ASSERT( !FloatUtils::isZero( auxCoefficient ) );

    _basicAssignment[_m - 1] = _basicAssignment[_m - 1] / auxCoefficient;

    ASSERT( FloatUtils::wellFormed( _basicAssignment[_m - 1] ) );

    if ( FloatUtils::isZero( _basicAssignment[_m - 1] ) )
        _basicAssignment[_m - 1] = 0.0;

    notifyVariableValue( _basicIndexToVariable[_m - 1], _basicAssignment[_m - 1] );
    computeBasicStatus( _m - 1 );

    // Finally, give the extended B0 matrix to the basis factorization
    _basisFactorization->setBasis( newB0 );

    delete[] newB0;
}

void Tableau::addRow()
{
    unsigned newM = _m + 1;
    unsigned newN = _n + 1;

    /*
      This function increases the sizes of the data structures used by
      the tableau to match newM and newN. Notice that newM = _m + 1 and
      newN = _n + 1, and so newN - newM = _n - _m. Consequently, structures
      that are of size _n - _m are left as is.
    */

    // Allocate a new A, copy the columns of the old A
    double *newA = new double[newN * newM];
    if ( !newA )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::newA" );
    std::fill( newA, newA + ( newM * newN ), 0.0 );

    double *AColumn, *newAColumn;
    for ( unsigned i = 0; i < _n; ++i )
    {
        AColumn = _A + ( i * _m );
        newAColumn = newA + ( i * newM );
        memcpy( newAColumn, AColumn, _m * sizeof(double) );
    }

    delete[] _A;
    _A = newA;

    // Allocate a new changeColumn. Don't need to initialize
    double *newChangeColumn = new double[newM];
    if ( !newChangeColumn )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::newChangeColumn" );
    delete[] _changeColumn;
    _changeColumn = newChangeColumn;

    // Allocate a new b and copy the old values
    double *newB = new double[newM];
    if ( !newB )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::newB" );
    std::fill( newB + _m, newB + newM, 0.0 );
    memcpy( newB, _b, _m * sizeof(double) );
    delete[] _b;
    _b = newB;

    // Allocate a new unit vector. Don't need to initialize
    double *newUnitVector = new double[newM];
    if ( !newUnitVector )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::newUnitVector" );
    delete[] _unitVector;
    _unitVector = newUnitVector;

    // Allocate new multipliers. Don't need to initialize
    double *newMultipliers = new double[newM];
    if ( !newMultipliers )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::newMultipliers" );
    delete[] _multipliers;
    _multipliers = newMultipliers;

    // Allocate new index arrays. Copy old indices, but don't assign indices to new variables yet.
    unsigned *newBasicIndexToVariable = new unsigned[newM];
    if ( !newBasicIndexToVariable )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::newBasicIndexToVariable" );
    memcpy( newBasicIndexToVariable, _basicIndexToVariable, _m * sizeof(unsigned) );
    delete[] _basicIndexToVariable;
    _basicIndexToVariable = newBasicIndexToVariable;

    unsigned *newVariableToIndex = new unsigned[newN];
    if ( !newVariableToIndex )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::newVariableToIndex" );
    memcpy( newVariableToIndex, _variableToIndex, _n * sizeof(unsigned) );
    delete[] _variableToIndex;
    _variableToIndex = newVariableToIndex;

    // Allocate a new basic assignment vector, copy old values
    double *newBasicAssignment = new double[newM];
    if ( !newBasicAssignment )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::newAssignment" );
    memcpy( newBasicAssignment, _basicAssignment, sizeof(double) * _m );
    delete[] _basicAssignment;
    _basicAssignment = newBasicAssignment;

    unsigned *newBasicStatus = new unsigned[newM];
    if ( !newBasicStatus )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::newBasicStatus" );
    memcpy( newBasicStatus, _basicStatus, sizeof(unsigned) * _m );
    delete[] _basicStatus;
    _basicStatus = newBasicStatus;

    // Allocate new lower and upper bound arrays, and copy old values
    double *newLowerBounds = new double[newN];
    if ( !newLowerBounds )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::newLowerBounds" );
    memcpy( newLowerBounds, _lowerBounds, _n * sizeof(double) );
    delete[] _lowerBounds;
    _lowerBounds = newLowerBounds;

    double *newUpperBounds = new double[newN];
    if ( !newUpperBounds )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::newUpperBounds" );
    memcpy( newUpperBounds, _upperBounds, _n * sizeof(double) );
    delete[] _upperBounds;
    _upperBounds = newUpperBounds;

    // Mark the new variable as unbounded
    _lowerBounds[_n] = FloatUtils::negativeInfinity();
    _upperBounds[_n] = FloatUtils::infinity();

    // Allocate a larger basis factorization
    IBasisFactorization *newBasisFactorization =
        BasisFactorizationFactory::createBasisFactorization( newM );
    if ( !newBasisFactorization )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::newBasisFactorization" );
    delete _basisFactorization;
    _basisFactorization = newBasisFactorization;

    // Allocate a larger _work. Don't need to initialize.
    double *newWork = new double[newM];
    if ( !newWork )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::newWork" );
    delete[] _work;
    _work = newWork;

    _m = newM;
    _n = newN;
    _costFunctionManager->initialize();
}

void Tableau::registerToWatchVariable( VariableWatcher *watcher, unsigned variable )
{
    _variableToWatchers[variable].append( watcher );
}

void Tableau::unregisterToWatchVariable( VariableWatcher *watcher, unsigned variable )
{
    _variableToWatchers[variable].erase( watcher );
}

void Tableau::registerToWatchAllVariables( VariableWatcher *watcher )
{
    _globalWatchers.append( watcher );
}

void Tableau::notifyVariableValue( unsigned variable, double value )
{
    for ( auto &watcher : _globalWatchers )
        watcher->notifyVariableValue( variable, value );

    if ( _variableToWatchers.exists( variable ) )
    {
        for ( auto &watcher : _variableToWatchers[variable] )
            watcher->notifyVariableValue( variable, value );
    }
}

void Tableau::notifyLowerBound( unsigned variable, double bound )
{
    for ( auto &watcher : _globalWatchers )
        watcher->notifyLowerBound( variable, bound );

    if ( _variableToWatchers.exists( variable ) )
    {
        for ( auto &watcher : _variableToWatchers[variable] )
            watcher->notifyLowerBound( variable, bound );
    }
}

void Tableau::notifyUpperBound( unsigned variable, double bound )
{
    for ( auto &watcher : _globalWatchers )
        watcher->notifyUpperBound( variable, bound );

    if ( _variableToWatchers.exists( variable ) )
    {
        for ( auto &watcher : _variableToWatchers[variable] )
            watcher->notifyUpperBound( variable, bound );
    }
}

const double *Tableau::getRightHandSide() const
{
    return _b;
}

void Tableau::forwardTransformation( const double *y, double *x ) const
{
    _basisFactorization->forwardTransformation( y, x );
}

void Tableau::backwardTransformation( const double *y, double *x ) const
{
    _basisFactorization->backwardTransformation( y, x );
}

double Tableau::getSumOfInfeasibilities() const
{
    double result = 0;
    for ( unsigned i = 0; i < _m; ++i )
    {
        if ( basicTooLow( i ) )
            result += _lowerBounds[_basicIndexToVariable[i]] - _basicAssignment[i];
        else if ( basicTooHigh( i ) )
            result += _basicAssignment[i] - _upperBounds[_basicIndexToVariable[i]];
    }

    return result;
}

void Tableau::setStatistics( Statistics *statistics )
{
    _statistics = statistics;
}

void Tableau::log( const String &message )
{
    if ( GlobalConfiguration::TABLEAU_LOGGING )
        printf( "Tableau: %s\n", message.ascii() );
}

void Tableau::verifyInvariants()
{
    for ( unsigned i = 0; i < _m; ++i )
    {
        if ( !FloatUtils::wellFormed( _basicAssignment[i] ) )
        {
            printf( "Assignment for basic variable %u (index %u) is not well formed: %.15lf. "
                    "Range: [%.15lf, %.15lf]\n",
                    _basicIndexToVariable[i],
                    i,
                    _basicAssignment[i],
                    _lowerBounds[_basicIndexToVariable[i]],
                    _upperBounds[_basicIndexToVariable[i]] );
            exit( 1 );
        }
    }

    for ( unsigned i = 0; i < _n - _m; ++i )
    {
        if ( !FloatUtils::wellFormed( _nonBasicAssignment[i] ) )
        {
            printf( "Assignment for non-basic variable is not well formed: %.15lf\n",
                    _nonBasicAssignment[i] );
            exit( 1 );
        }
    }

    for ( unsigned i = 0; i < _n - _m; ++i )
    {
        unsigned var = _nonBasicIndexToVariable[i];
        if ( !( FloatUtils::gte( _nonBasicAssignment[i],
                                 _lowerBounds[var],
                                 GlobalConfiguration::BOUND_COMPARISON_TOLERANCE ) &&
                FloatUtils::lte( _nonBasicAssignment[i],
                                 _upperBounds[var],
                                 GlobalConfiguration::BOUND_COMPARISON_TOLERANCE ) ) )
        {
            // This behavior is okay iff lb > ub, and this is going to be caught
            // soon anyway

            if ( FloatUtils::gt( _lowerBounds[var], _upperBounds[var] ) )
                continue;

            printf( "Tableau test invariants: bound violation!\n" );
            printf( "Variable %u (non-basic #%u). Assignment: %lf. Range: [%lf, %lf]\n",
                    var, i, _nonBasicAssignment[i], _lowerBounds[var], _upperBounds[var] );

            exit( 1 );
        }
    }

    for ( unsigned i = 0; i < _m; ++i )
    {
        unsigned currentStatus = _basicStatus[i];
        computeBasicStatus( i );
        if ( _basicStatus[i] != currentStatus )
        {
            printf( "Error! Status[%u] was %s, but when recomputed got %s\n",
                    i,
                    basicStatusToString( currentStatus ).ascii(),
                    basicStatusToString( _basicStatus[i] ).ascii() );
            printf( "Variable: x%u, index: %u. Value: %.15lf, range: [%.15lf, %.15lf]\n",
                    _basicIndexToVariable[i],
                    i,
                    _basicAssignment[i],
                    _lowerBounds[_basicIndexToVariable[i]],
                    _upperBounds[_basicIndexToVariable[i]] );

            exit( 1 );
        }
    }
}

String Tableau::basicStatusToString( unsigned status )
{
    switch ( status )
    {
    case BELOW_LB:
        return "BELOW_LB";

    case AT_LB:
        return "AT_LB";

    case BETWEEN:
        return "BETWEEN";

    case AT_UB:
        return "AT_UB";

    case ABOVE_UB:
        return "ABOVE_UB";
    }

    return "UNKNOWN";
}

void Tableau::updateAssignmentForPivot()
{
    /*
      This method is invoked when the non-basic _enteringVariable and
      basic _leaving variable are about to be swapped. We adjust their
      values prior to actually switching their indices. We also update
      any other basic variables variables that are affected by this change.

      Specifically, we perform the following steps:

      1. Update _nonBasicAssignment[_enteringVariable] to the new
      value that the leaving variable will have after the swap. This
      is determined using the _leavingVariableIncreases flag and the
      current status of the leaving variable.

      2. Update _basicAssignment[_leavingVariable] to the new value
      that the entering variable will have after the swap. This is
      determined using the change in value of the leaving variable,
      and the appropriate entry in the change column.

      3. Update the assignment for any basic variables that depend on
      the entering variable, using the change column.


      If the pivot is fake (non-basic hopping to other bound), we just
      update the affected basics and the non-basic itself.
    */

    _basicAssignmentStatus = ITableau::BASIC_ASSIGNMENT_UPDATED;

    if ( performingFakePivot() )
    {
        // A non-basic is hopping from one bound to the other.

        bool nonBasicDecreases =
            FloatUtils::isPositive( _costFunctionManager->getCostFunction()[_enteringVariable] );
        unsigned nonBasic = _nonBasicIndexToVariable[_enteringVariable];

        double nonBasicDelta;
        if ( nonBasicDecreases )
            nonBasicDelta = _lowerBounds[nonBasic] - _nonBasicAssignment[_enteringVariable];
        else
            nonBasicDelta = _upperBounds[nonBasic] - _nonBasicAssignment[_enteringVariable];

        // Update all the affected basic variables
        for ( unsigned i = 0; i < _m; ++i )
        {
            if ( FloatUtils::isZero( _changeColumn[i] ) )
                 continue;

            _basicAssignment[i] -= _changeColumn[i] * nonBasicDelta;
            notifyVariableValue( _basicIndexToVariable[i], _basicAssignment[i] );
            computeBasicStatus( i );
        }

        // Update the assignment for the non-basic variable
        _nonBasicAssignment[_enteringVariable] = nonBasicDecreases ? _lowerBounds[nonBasic] : _upperBounds[nonBasic];
        notifyVariableValue( nonBasic, _nonBasicAssignment[_enteringVariable] );
    }
    else
    {
        // An actual pivot is taking place.

        // Discover by how much the leaving variable is going to change
        double basicDelta;
        unsigned currentBasic = _basicIndexToVariable[_leavingVariable];
        double currentBasicValue = _basicAssignment[_leavingVariable];
        bool basicGoingToUpperBound;

        if ( _leavingVariableIncreases )
        {
            if ( _basicStatus[_leavingVariable] == Tableau::BELOW_LB )
                basicGoingToUpperBound = false;
            else
                basicGoingToUpperBound = true;
        }
        else
        {
            if ( _basicStatus[_leavingVariable] == Tableau::ABOVE_UB )
                basicGoingToUpperBound = true;
            else
                basicGoingToUpperBound = false;
        }

        if ( basicGoingToUpperBound )
            basicDelta = _upperBounds[currentBasic] - currentBasicValue;
        else
            basicDelta = _lowerBounds[currentBasic] - currentBasicValue;

        // Now that we know by how much the leaving variable changes,
        // we can calculate by how much the entering variable is going
        // to change.
        double nonBasicDelta = basicDelta / -_changeColumn[_leavingVariable];

        // Update all the other basic variables
        for ( unsigned i = 0; i < _m; ++i )
        {
            if ( i == _leavingVariable )
                continue;

            if ( FloatUtils::isZero( _changeColumn[i] ) )
                 continue;

            _basicAssignment[i] -= _changeColumn[i] * nonBasicDelta;
            notifyVariableValue( _basicIndexToVariable[i], _basicAssignment[i] );
            computeBasicStatus( i );
        }

        // Update the assignment for the entering variable
        _basicAssignment[_leavingVariable] = _nonBasicAssignment[_enteringVariable] + nonBasicDelta;
        notifyVariableValue( _nonBasicIndexToVariable[_enteringVariable], _basicAssignment[_leavingVariable] );

        // Update the assignment for the leaving variable
        _nonBasicAssignment[_enteringVariable] =
            basicGoingToUpperBound ? _upperBounds[currentBasic] : _lowerBounds[currentBasic];
        notifyVariableValue( currentBasic, _nonBasicAssignment[_enteringVariable] );
    }
}

void Tableau::updateCostFunctionForPivot()
{
    // If the pivot is fake, the cost function does not change
    if ( performingFakePivot() )
        return;

    double pivotElement = -_changeColumn[_leavingVariable];
    _costFunctionManager->updateCostFunctionForPivot( _enteringVariable,
                                                      _leavingVariable,
                                                      pivotElement,
                                                      _pivotRow );
}

ITableau::BasicAssignmentStatus Tableau::getBasicAssignmentStatus() const
{
    return _basicAssignmentStatus;
}

void Tableau::setBasicAssignmentStatus( ITableau::BasicAssignmentStatus status )
{
    _basicAssignmentStatus = status;
}

bool Tableau::basisMatrixAvailable() const
{
    return _basisFactorization->explicitBasisAvailable();
}

void Tableau::getBasisEquations( List<Equation *> &equations ) const
{
    if ( !basisMatrixAvailable() )
        _basisFactorization->makeExplicitBasisAvailable();

    for ( unsigned i = 0; i < _m; ++i )
        equations.append( getBasisEquation( i ) );
}

Equation *Tableau::getBasisEquation( unsigned row ) const
{
    Equation *equation = new Equation;

    // Add the scalar
    equation->setScalar( _b[row] );

    // Add the basic variables
    const double *b0 = _basisFactorization->getBasis();
    for ( unsigned i = 0; i < _m; ++i )
    {
        unsigned basicVariable = _basicIndexToVariable[i];
        double coefficient = b0[_m * row + i];

        if ( !FloatUtils::isZero( coefficient ) )
            equation->addAddend( coefficient, basicVariable );
    }

    // Add the non-basic variables
    for ( unsigned i = 0; i < _n - _m; ++i )
    {
        unsigned nonBasicVariable = _nonBasicIndexToVariable[i];
        double coefficient = _A[nonBasicVariable * _m + row];

        if ( !FloatUtils::isZero( coefficient ) )
            equation->addAddend( coefficient, nonBasicVariable );
    }

    return equation;
}

double *Tableau::getInverseBasisMatrix() const
{
    double *result = new double[_m * _m];
    _basisFactorization->invertBasis( result );
    return result;
}

Set<unsigned> Tableau::getBasicVariables() const
{
    return _basicVariables;
}

void Tableau::registerCostFunctionManager( ICostFunctionManager *costFunctionManager )
{
    _costFunctionManager = costFunctionManager;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
