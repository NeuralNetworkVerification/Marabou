/*********************                                                        */
/*! \file Tableau.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Duligur Ibeling, Parth Shah
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#include "BasisFactorizationFactory.h"
#include "CSRMatrix.h"
#include "ConstraintMatrixAnalyzer.h"
#include "Debug.h"
#include "EntrySelectionStrategy.h"
#include "Equation.h"
#include "FloatUtils.h"
#include "ICostFunctionManager.h"
#include "MStringf.h"
#include "MalformedBasisException.h"
#include "MarabouError.h"
#include "PiecewiseLinearCaseSplit.h"
#include "Tableau.h"
#include "TableauRow.h"
#include "TableauState.h"

#include <string.h>

Tableau::Tableau()
    : _n ( 0 )
    , _m ( 0 )
    , _A( NULL )
    , _sparseColumnsOfA( NULL )
    , _sparseRowsOfA( NULL )
    , _denseA( NULL )
    , _changeColumn( NULL )
    , _pivotRow( NULL )
    , _b( NULL )
    , _workM( NULL )
    , _workN( NULL )
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
    , _rhsIsAllZeros( true )
    , _boundsExplanator( NULL )
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
        delete _A;
        _A = NULL;
    }

    if ( _sparseColumnsOfA )
    {
        for ( unsigned i = 0; i < _n; ++i )
        {
            if ( _sparseColumnsOfA[i] )
            {
                delete _sparseColumnsOfA[i];
                _sparseColumnsOfA[i] = NULL;
            }
        }

        delete[] _sparseColumnsOfA;
        _sparseColumnsOfA = NULL;
    }

    if ( _sparseRowsOfA )
    {
        for ( unsigned i = 0; i < _m; ++i )
        {
            if ( _sparseRowsOfA[i] )
            {
                delete _sparseRowsOfA[i];
                _sparseRowsOfA[i] = NULL;
            }
        }

        delete[] _sparseRowsOfA;
        _sparseRowsOfA = NULL;
    }

    if ( _denseA )
    {
        delete[] _denseA;
        _denseA = NULL;
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

    if ( _workM )
    {
        delete[] _workM;
        _workM = NULL;
    }

    if ( _workN )
    {
        delete[] _workN;
        _workN = NULL;
    }

    if ( _boundsExplanator )
    {
        delete _boundsExplanator;
        _boundsExplanator = NULL;
    }
}

void Tableau::setDimensions( unsigned m, unsigned n )
{
    _m = m;
    _n = n;

    _A = new CSRMatrix();
    if ( !_A )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::A" );

    _sparseColumnsOfA = new SparseUnsortedList *[n];
    if ( !_sparseColumnsOfA )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::sparseColumnsOfA" );

    for ( unsigned i = 0; i < n; ++i )
    {
        _sparseColumnsOfA[i] = new SparseUnsortedList( _m );
        if ( !_sparseColumnsOfA[i] )
            throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::sparseColumnsOfA[i]" );
    }

    _sparseRowsOfA = new SparseUnsortedList *[m];
    if ( !_sparseRowsOfA )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::sparseRowOfA" );

    for ( unsigned i = 0; i < m; ++i )
    {
        _sparseRowsOfA[i] = new SparseUnsortedList( _n );
        if ( !_sparseRowsOfA[i] )
            throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::sparseRowOfA[i]" );
    }

    _denseA = new double[m*n];
    if ( !_denseA )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::denseA" );

    _changeColumn = new double[m];
    if ( !_changeColumn )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::changeColumn" );

    _pivotRow = new TableauRow( n-m );
    if ( !_pivotRow )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::pivotRow" );

    _b = new double[m];
    if ( !_b )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::b" );

    _unitVector = new double[m];
    if ( !_unitVector )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::unitVector" );

    _multipliers = new double[m];
    if ( !_multipliers )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::multipliers" );

    _basicIndexToVariable = new unsigned[m];
    if ( !_basicIndexToVariable )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::basicIndexToVariable" );

    _variableToIndex = new unsigned[n];
    if ( !_variableToIndex )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::variableToIndex" );

    _nonBasicIndexToVariable = new unsigned[n-m];
    if ( !_nonBasicIndexToVariable )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::nonBasicIndexToVariable" );

    _nonBasicAssignment = new double[n-m];
    if ( !_nonBasicAssignment )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::nonBasicAssignment" );

    _lowerBounds = new double[n];
    if ( !_lowerBounds )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::lowerBounds" );
    std::fill_n( _lowerBounds, n, FloatUtils::negativeInfinity() );

    _upperBounds = new double[n];
    if ( !_upperBounds )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::upperBounds" );
    std::fill_n( _upperBounds, n, FloatUtils::infinity() );

    _basicAssignment = new double[m];
    if ( !_basicAssignment )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::assignment" );

    _basicStatus = new unsigned[m];
    if ( !_basicStatus )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::basicStatus" );

    _basisFactorization = BasisFactorizationFactory::createBasisFactorization( _m, *this );
    if ( !_basisFactorization )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::basisFactorization" );
    _basisFactorization->setStatistics( _statistics );

    _workM = new double[m];
    if ( !_workM )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::work" );

    _workN = new double[n];
    if ( !_workN )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::work" );

    if ( _statistics )
        _statistics->setCurrentTableauDimension( _m, _n );

    if( GlobalConfiguration::PROOF_CERTIFICATE )
	{
    	if( _boundsExplanator )
			delete _boundsExplanator;

		_boundsExplanator = new BoundsExplanator(_n, _m);  // Reset whenever new dimensions are set.
		if ( !_boundsExplanator )
			throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::work" );
	}

	// Guy: the tableau dimensions can also change in addRow(). The
	// most elegant way to be informed when this happens is to
	// register the bound explanator to be a resizeWatcher, via the
	// registerResizeWatcher mechanism. Then it will be notified
	// whenever there's a change.
}

void Tableau::setConstraintMatrix( const double *A )
{
    _A->initialize( A, _m, _n );

    for ( unsigned column = 0; column < _n; ++column )
    {
        for ( unsigned row = 0; row < _m; ++row )
            _denseA[column*_m + row] = A[row*_n + column];

        _sparseColumnsOfA[column]->initialize( _denseA + ( column * _m ), _m );
    }

    for ( unsigned row = 0; row < _m; ++row )
        _sparseRowsOfA[row]->initialize( A + ( row * _n ), _n );
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

void Tableau::initializeTableau( const List<unsigned> &initialBasicVariables )
{
    _basicVariables.clear();

    // Assign the basic indices
    unsigned basicIndex = 0;
    for( unsigned basicVar : initialBasicVariables )
    {
        markAsBasic( basicVar );
        assignIndexToBasicVariable( basicVar, basicIndex );
        ++basicIndex;
    }

    // Assign the non-basic indices
    unsigned nonBasicIndex = 0;
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

    // Factorize the basis
    _basisFactorization->obtainFreshBasis();

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

    memcpy( _workM, _b, sizeof(double) * _m );

    // Compute a linear combination of the columns of AN
    for ( unsigned i = 0; i < _n - _m; ++i )
    {
        unsigned var = _nonBasicIndexToVariable[i];
        double value = _nonBasicAssignment[i];

        for ( const auto &entry : *_sparseColumnsOfA[var] )
            _workM[entry._index] -= entry._value * value;
    }

    // Solve B*xB = y by performing a forward transformation
    _basisFactorization->forwardTransformation( _workM, _basicAssignment );

    computeBasicStatus();

    _basicAssignmentStatus = ITableau::BASIC_ASSIGNMENT_JUST_COMPUTED;

    // Inform the watchers
    for ( unsigned i = 0; i < _m; ++i )
        notifyVariableValue( _basicIndexToVariable[i], _basicAssignment[i] );
}

bool Tableau::checkValueWithinBounds( unsigned variable, double value )
{
    return
        FloatUtils::gte( value, getLowerBound( variable ) ) &&
        FloatUtils::lte( value, getUpperBound( variable ) );
}

void Tableau::computeBasicStatus()
{
    for ( unsigned i = 0; i < _m; ++i )
        computeBasicStatus( i );
}

void Tableau::computeBasicStatus( unsigned basicIndex )
{
    unsigned variable = _basicIndexToVariable[basicIndex];
    double value = _basicAssignment[basicIndex];

    double lb = _lowerBounds[variable];
    double relaxedLb =
        lb -
        ( GlobalConfiguration::BOUND_COMPARISON_ADDITIVE_TOLERANCE +
          GlobalConfiguration::BOUND_COMPARISON_MULTIPLICATIVE_TOLERANCE * FloatUtils::abs( lb ) );

    double ub = _upperBounds[variable];
    double relaxedUb =
        ub +
        ( GlobalConfiguration::BOUND_COMPARISON_ADDITIVE_TOLERANCE +
          GlobalConfiguration::BOUND_COMPARISON_MULTIPLICATIVE_TOLERANCE * FloatUtils::abs( ub ) );

    if ( value > relaxedUb )
    {
        _basicStatus[basicIndex] = Tableau::ABOVE_UB;
    }
    else if ( value < relaxedLb )
    {
        _basicStatus[basicIndex] = Tableau::BELOW_LB;
    }
    else
    {
        _basicStatus[basicIndex] = Tableau::BETWEEN;
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
    /*
      If this variable has been merged into another,
      we need to be reading the other variable's value
    */
    if ( _mergedVariables.exists( variable ) )
        variable = _mergedVariables[variable];

    // The values of non-basics can be extracted even if the
    // assignment is invalid
    if ( !_basicVariables.exists( variable ) )
    {
        unsigned index = _variableToIndex[variable];
        return _nonBasicAssignment[index];
    }

    ASSERT( _basicAssignmentStatus != ITableau::BASIC_ASSIGNMENT_INVALID );

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

    for ( unsigned i = 0; i < _m; ++i )
    {
        if ( !FloatUtils::isZero( _b[i] ) )
            _rhsIsAllZeros = false;
    }
}

void Tableau::setRightHandSide( unsigned index, double value )
{
    _b[index] = value;

    if ( !FloatUtils::isZero( value ) )
        _rhsIsAllZeros = false;
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
    // A non-basic variable is eligible for entry if one of the
    // two conditions holds:
    //
    //   1. It has a negative coefficient in the cost function and it
    //      can increase
    //   2. It has a positive coefficient in the cost function and it
    //      can decrease

    double reducedCost = costFunction[nonBasic];

    if ( reducedCost <= -GlobalConfiguration::ENTRY_ELIGIBILITY_TOLERANCE )
    {
        // Variable needs to increase
        return nonBasicCanIncrease( nonBasic );
    }
    else if ( reducedCost >= +GlobalConfiguration::ENTRY_ELIGIBILITY_TOLERANCE )
    {
        // Variable needs to decrease
        return nonBasicCanDecrease( nonBasic );
    }

    // Cost is zero
    return false;
}

bool Tableau::nonBasicCanIncrease( unsigned nonBasic ) const
{
    unsigned variable = _nonBasicIndexToVariable[nonBasic];

    double ub = _upperBounds[variable];
    double tighterUb =
        ub -
        ( GlobalConfiguration::BOUND_COMPARISON_ADDITIVE_TOLERANCE +
          GlobalConfiguration::BOUND_COMPARISON_MULTIPLICATIVE_TOLERANCE * FloatUtils::abs( ub ) );

    return _nonBasicAssignment[nonBasic] < tighterUb;
}

bool Tableau::nonBasicCanDecrease( unsigned nonBasic ) const
{
    unsigned variable = _nonBasicIndexToVariable[nonBasic];

    double lb = _lowerBounds[variable];
    double tighterLb =
        lb +
        ( GlobalConfiguration::BOUND_COMPARISON_ADDITIVE_TOLERANCE +
          GlobalConfiguration::BOUND_COMPARISON_MULTIPLICATIVE_TOLERANCE * FloatUtils::abs( lb ) );

    return _nonBasicAssignment[nonBasic] > tighterLb;
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

    bool decrease;
    unsigned  nonBasic;
    (void) decrease;
    (void) nonBasic;
    if ( _leavingVariable == _m )
    {
        if ( _statistics )
            _statistics->incNumTableauBoundHopping();

        double enteringReducedCost = _costFunctionManager->getCostFunction()[_enteringVariable];

        ASSERT( ( enteringReducedCost <= -GlobalConfiguration::ENTRY_ELIGIBILITY_TOLERANCE ) ||
                ( enteringReducedCost >= +GlobalConfiguration::ENTRY_ELIGIBILITY_TOLERANCE ) );

        decrease = ( enteringReducedCost >= +GlobalConfiguration::ENTRY_ELIGIBILITY_TOLERANCE );
        nonBasic = _nonBasicIndexToVariable[_enteringVariable];

        TABLEAU_LOG( Stringf( "Performing 'fake' pivot. Variable x%u jumping to %s bound",
                      _nonBasicIndexToVariable[_enteringVariable],
                      decrease ? "LOWER" : "UPPER" ).ascii() );
        TABLEAU_LOG( Stringf( "Current value: %.3lf. Range: [%.3lf, %.3lf]\n",
                      _nonBasicAssignment[_enteringVariable],
                      _lowerBounds[nonBasic], _upperBounds[nonBasic] ).ascii() );

        updateAssignmentForPivot();

        return;
    }

    struct timespec pivotStart;

    if ( _statistics )
    {
        pivotStart = TimeUtils::sampleMicro();
        _statistics->incNumTableauPivots();
    }

    unsigned currentBasic = _basicIndexToVariable[_leavingVariable];
    unsigned currentNonBasic = _nonBasicIndexToVariable[_enteringVariable];

    TABLEAU_LOG( Stringf( "Tableau performing pivot. Entering: %u, Leaving: %u",
                  _nonBasicIndexToVariable[_enteringVariable],
                  _basicIndexToVariable[_leavingVariable] ).ascii() );
    TABLEAU_LOG( Stringf( "Leaving variable %s. Current value: %.15lf. Range: [%.15lf, %.15lf]",
                  _leavingVariableIncreases ? "increases" : "decreases",
                  _basicAssignment[_leavingVariable],
                  _lowerBounds[currentBasic], _upperBounds[currentBasic] ).ascii() );
    TABLEAU_LOG( Stringf( "Entering variable %s. Current value: %.15lf. Range: [%.15lf, %.15lf]",
                  FloatUtils::isNegative( _costFunctionManager->getCostFunction()[_enteringVariable] ) ?
                  "increases" : "decreases",
                  _nonBasicAssignment[_enteringVariable],
                  _lowerBounds[currentNonBasic], _upperBounds[currentNonBasic] ).ascii() );
    TABLEAU_LOG( Stringf( "Change ratio is: %.15lf\n", _changeRatio ).ascii() );

    // As part of the pivot operation we use both the pivot row and
    // pivot column. If they don't agree on the intersection, there's some
    // numerical degradation issue.
    double pivotEntryByColumn = -_changeColumn[_leavingVariable];
    double pivotEntryByRow = _pivotRow->_row[_enteringVariable]._coefficient;
    if ( !FloatUtils::isZero( pivotEntryByRow - pivotEntryByColumn, GlobalConfiguration::PIVOT_ROW_AND_COLUMN_TOLERANCE ) )
        throw MalformedBasisException();

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
    _basisFactorization->updateToAdjacentBasis( _leavingVariable,
                                                _changeColumn,
                                                getAColumn( currentNonBasic ) );

    if ( _statistics )
    {
        struct timespec pivotEnd = TimeUtils::sampleMicro();
        _statistics->addTimePivots( TimeUtils::timePassed( pivotStart, pivotEnd ) );
    }
}

void Tableau::performDegeneratePivot()
{
    struct timespec pivotStart;
    if ( _statistics )
    {
        pivotStart = TimeUtils::sampleMicro();
        _statistics->incNumTableauPivots();
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
    _basisFactorization->updateToAdjacentBasis( _leavingVariable,
                                                _changeColumn,
                                                getAColumn( currentNonBasic ) );

    // Switch assignment values. No call to notify is required,
    // because values haven't changed.
    double temp = _basicAssignment[_leavingVariable];
    _basicAssignment[_leavingVariable] = _nonBasicAssignment[_enteringVariable];
    _nonBasicAssignment[_enteringVariable] = temp;
    computeBasicStatus( _leavingVariable );

    if ( _statistics )
    {
        struct timespec pivotEnd = TimeUtils::sampleMicro();
        _statistics->addTimePivots( TimeUtils::timePassed( pivotStart, pivotEnd ) );
    }
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

    double basicCost = _costFunctionManager->getBasicCost( basicIndex );

    if ( ( FloatUtils::isPositive( coefficient ) && decrease ) ||
         ( FloatUtils::isNegative( coefficient ) && !decrease ) )
    {
        // Basic variable is decreasing
        double actualLowerBound;
        if ( basicCost > 0 )
        {
            actualLowerBound = _upperBounds[basic];
        }
        else if ( basicCost < 0 )
        {
            actualLowerBound = FloatUtils::negativeInfinity();
        }
        else
        {
            actualLowerBound = _lowerBounds[basic];
        }

        double tolerance = GlobalConfiguration::RATIO_CONSTRAINT_ADDITIVE_TOLERANCE +
            FloatUtils::abs( actualLowerBound ) * GlobalConfiguration::RATIO_CONSTRAINT_MULTIPLICATIVE_TOLERANCE;

        if ( _basicAssignment[basicIndex] <= actualLowerBound + tolerance )
        {
            ratio = 0;
        }
        else
        {
            double maxChange = actualLowerBound - _basicAssignment[basicIndex];
            ASSERT( !FloatUtils::isPositive( maxChange ) );
            ratio = maxChange / coefficient;
        }
    }
    else if ( ( FloatUtils::isPositive( coefficient ) && !decrease ) ||
              ( FloatUtils::isNegative( coefficient ) && decrease ) )

    {
        // Basic variable is increasing
        double actualUpperBound;
        if ( basicCost < 0 )
        {
            actualUpperBound = _lowerBounds[basic];
        }
        else if ( basicCost > 0 )
        {
            actualUpperBound = FloatUtils::infinity();
        }
        else
        {
            actualUpperBound = _upperBounds[basic];
        }

        double tolerance = GlobalConfiguration::RATIO_CONSTRAINT_ADDITIVE_TOLERANCE +
            FloatUtils::abs( actualUpperBound ) * GlobalConfiguration::RATIO_CONSTRAINT_MULTIPLICATIVE_TOLERANCE;

        if ( _basicAssignment[basicIndex] >= actualUpperBound - tolerance )
        {
            ratio = 0;
        }
        else
        {
            double maxChange = actualUpperBound - _basicAssignment[basicIndex];
            ASSERT( !FloatUtils::isNegative( maxChange ) );
            ratio = maxChange / coefficient;
        }
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
    if ( GlobalConfiguration::USE_HARRIS_RATIO_TEST )
        harrisRatioTest( changeColumn );
    else
        standardRatioTest( changeColumn );
}

void Tableau::standardRatioTest( double *changeColumn )
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

    double largestPivot = 0;
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
            if ( changeColumn[i] >= +GlobalConfiguration::PIVOT_CHANGE_COLUMN_TOLERANCE ||
                 changeColumn[i] <= -GlobalConfiguration::PIVOT_CHANGE_COLUMN_TOLERANCE )
            {
                double ratio = ratioConstraintPerBasic( i, changeColumn[i], decrease );

                if ( ( ratio > _changeRatio ) ||
                     ( ( ratio == _changeRatio ) && ( FloatUtils::abs( changeColumn[i] ) > largestPivot ) ) )
                {
                    _changeRatio = ratio;
                    _leavingVariable = i;
                    largestPivot = FloatUtils::abs( changeColumn[i] );
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
            if ( changeColumn[i] >= +GlobalConfiguration::PIVOT_CHANGE_COLUMN_TOLERANCE ||
                 changeColumn[i] <= -GlobalConfiguration::PIVOT_CHANGE_COLUMN_TOLERANCE )
            {
                double ratio = ratioConstraintPerBasic( i, changeColumn[i], decrease );

                if ( ( ratio < _changeRatio ) ||
                     ( ( ratio == _changeRatio ) && ( FloatUtils::abs( changeColumn[i] ) > largestPivot ) ) )
                {
                    _changeRatio = ratio;
                    _leavingVariable = i;
                    largestPivot = FloatUtils::abs( changeColumn[i] );
                }
            }
        }

        // Only perform this check if pivot isn't fake
        if ( _leavingVariable != _m )
            _leavingVariableIncreases = FloatUtils::isNegative( changeColumn[_leavingVariable] );
    }
}

void Tableau::harrisRatioTest( double *changeColumn )
{
    /*
      The Harris ratio test is performed in two steps:

      1. Find the minimal change ratio, according to the constraints imposed by the
         basic variables
      2. For the discovered change ratio, pick the basic variable leading to the largest
         pivot element.

      Observe that the constraints imposed by the basic variables are slightly relaxed
      when compared to the traditional, text-book ratio test
    */

    ASSERT( !FloatUtils::isZero( _costFunctionManager->getCostFunction()[_enteringVariable] ) );

    // Is the entering variable decreasing?
    bool enteringDecreases = FloatUtils::isPositive( _costFunctionManager->getCostFunction()[_enteringVariable] );

    DEBUG({
            if ( enteringDecreases )
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

    /*
      Alfa:

      nb increases -->   pivot ( = -changeColumn )
      nb decreases --> - pivot ( =  changeColumn )
    */

    // *** First pass: determine optimal change ratio *** //
    double optimalChangeRatio;
    if ( enteringDecreases )
    {
        // The maximum amount by which the entering variable can
        // decrease, as determined by its bounds. This is a negative
        // value.
        optimalChangeRatio = FloatUtils::negativeInfinity();

        // Iterate over the basics that depend on the entering
        // variable and see if any of them imposes a tighter
        // constraint.
        double ratioConstraintPerBasic;
        for ( unsigned i = 0; i < _m; ++i )
        {
            unsigned basic = _basicIndexToVariable[i];
            double basicCost = _costFunctionManager->getBasicCost( i );
            if ( changeColumn[i] >= +GlobalConfiguration::PIVOT_CHANGE_COLUMN_TOLERANCE )
            {
                // Nonbasic decreases, basic increases
                double actualUpperBound;
                if ( basicCost > 0 )
                    continue;
                else if ( basicCost < 0 )
                    actualUpperBound = _lowerBounds[basic];
                else
                    actualUpperBound = _upperBounds[basic];

                // Determine the constraint imposed by this basic
                double delta = GlobalConfiguration::HARRIS_RATIO_CONSTRAINT_ADDITIVE_TOLERANCE +
                    FloatUtils::abs( actualUpperBound ) * GlobalConfiguration::HARRIS_RATIO_CONSTRAINT_MULTIPLICATIVE_TOLERANCE;
                if ( _basicAssignment[i] > actualUpperBound )
                    ratioConstraintPerBasic = - delta / changeColumn[i];
                else
                    ratioConstraintPerBasic = - ( ( actualUpperBound + delta ) - _basicAssignment[i] ) / changeColumn[i];
            }
            else if ( changeColumn[i] <= -GlobalConfiguration::PIVOT_CHANGE_COLUMN_TOLERANCE )
            {
                // Nonbasic decreases, basic decreases
                double actualLowerBound;
                if ( basicCost < 0 )
                    continue;
                else if ( basicCost > 0 )
                    actualLowerBound = _upperBounds[basic];
                else
                    actualLowerBound = _lowerBounds[basic];

                // Determine the constraint imposed by this basic
                double delta = GlobalConfiguration::HARRIS_RATIO_CONSTRAINT_ADDITIVE_TOLERANCE +
                    FloatUtils::abs( actualLowerBound ) * GlobalConfiguration::HARRIS_RATIO_CONSTRAINT_MULTIPLICATIVE_TOLERANCE;
                if ( _basicAssignment[i] < actualLowerBound )
                    ratioConstraintPerBasic = delta / changeColumn[i];
                else
                    ratioConstraintPerBasic = - ( ( actualLowerBound - delta ) - _basicAssignment[i] ) / changeColumn[i];
            }
            else
            {
                // This basic does not depend on the entering variable
                continue;
            }

            ASSERT( !FloatUtils::isPositive( ratioConstraintPerBasic ) );
            if ( ratioConstraintPerBasic > optimalChangeRatio )
                optimalChangeRatio = ratioConstraintPerBasic;
        }
    }
    else
    {
        // The maximum amount by which the entering variable can
        // increase, as determined by its bounds. This is a positive
        // value.
        optimalChangeRatio = FloatUtils::infinity();

        // Iterate over the basics that depend on the entering
        // variable and see if any of them imposes a tighter
        // constraint.
        double ratioConstraintPerBasic;
        for ( unsigned i = 0; i < _m; ++i )
        {
            unsigned basic = _basicIndexToVariable[i];
            double basicCost = _costFunctionManager->getBasicCost( i );
            if ( changeColumn[i] >= +GlobalConfiguration::PIVOT_CHANGE_COLUMN_TOLERANCE )
            {
                // Nonbasic increases, basic decreases
                double actualLowerBound;
                if ( basicCost < 0 )
                    continue;
                else if ( basicCost > 0 )
                    actualLowerBound = _upperBounds[basic];
                else
                    actualLowerBound = _lowerBounds[basic];

                // Determine the constraint imposed by this basic
                double delta = GlobalConfiguration::HARRIS_RATIO_CONSTRAINT_ADDITIVE_TOLERANCE +
                    FloatUtils::abs( actualLowerBound ) * GlobalConfiguration::HARRIS_RATIO_CONSTRAINT_MULTIPLICATIVE_TOLERANCE;
                if ( _basicAssignment[i] < actualLowerBound )
                    ratioConstraintPerBasic = delta / changeColumn[i];
                else
                    ratioConstraintPerBasic = - ( ( actualLowerBound - delta ) - _basicAssignment[i] ) / changeColumn[i];
            }
            else if ( changeColumn[i] <= -GlobalConfiguration::PIVOT_CHANGE_COLUMN_TOLERANCE )
            {
                // Nonbasic increases, basic increases
                double actualUpperBound;
                if ( basicCost > 0 )
                    continue;
                else if ( basicCost < 0 )
                    actualUpperBound = _lowerBounds[basic];
                else
                    actualUpperBound = _upperBounds[basic];

                // Determine the constraint imposed by this basic
                double delta = GlobalConfiguration::HARRIS_RATIO_CONSTRAINT_ADDITIVE_TOLERANCE +
                    FloatUtils::abs( actualUpperBound ) * GlobalConfiguration::HARRIS_RATIO_CONSTRAINT_MULTIPLICATIVE_TOLERANCE;
                if ( _basicAssignment[i] > actualUpperBound )
                    ratioConstraintPerBasic = - delta / changeColumn[i];
                else
                    ratioConstraintPerBasic = - ( ( actualUpperBound + delta ) - _basicAssignment[i] ) / changeColumn[i];
            }
            else
            {
                // This basic does not depend on the entering variable
                continue;
            }

            ASSERT( !FloatUtils::isNegative( ratioConstraintPerBasic ) );
            if ( ratioConstraintPerBasic < optimalChangeRatio )
                optimalChangeRatio = ratioConstraintPerBasic;
        }
    }

    // *** Second pass: choose leaving variable *** //

    // Check if we need to perform a fake pivot
    double enteringLb = _lowerBounds[_nonBasicIndexToVariable[_enteringVariable]];
    double enteringUb = _upperBounds[_nonBasicIndexToVariable[_enteringVariable]];
    double enteringCurrentValue = _nonBasicAssignment[_enteringVariable];

    _leavingVariable = _m;
    if ( enteringDecreases )
    {
        ASSERT( !FloatUtils::isPositive( optimalChangeRatio ) );
        if ( enteringLb - enteringCurrentValue >= optimalChangeRatio )
        {
            _changeRatio = enteringLb - enteringCurrentValue;
            return;
        }
    }
    else
    {
        ASSERT( !FloatUtils::isNegative( optimalChangeRatio ) );
        if ( enteringUb - enteringCurrentValue <= optimalChangeRatio )
        {
            _changeRatio = enteringUb - enteringCurrentValue;
            return;
        }
    }

    // Pivot isn't fake, choose the leaving variable
    double largestPivot = 0;
    if ( enteringDecreases )
    {
        // Change ratios are negative
        double ratioConstraintPerBasic;
        for ( unsigned i = 0; i < _m; ++i )
        {
            unsigned basic = _basicIndexToVariable[i];
            double basicCost = _costFunctionManager->getBasicCost( i );
            if ( changeColumn[i] >= +GlobalConfiguration::PIVOT_CHANGE_COLUMN_TOLERANCE )
            {
                // Nonbasic decreases, basic increases
                double actualUpperBound;
                if ( basicCost > 0 )
                    continue;
                else if ( basicCost < 0 )
                    actualUpperBound = _lowerBounds[basic];
                else
                    actualUpperBound = _upperBounds[basic];

                ratioConstraintPerBasic = - ( actualUpperBound - _basicAssignment[i] ) / changeColumn[i];
            }
            else if ( changeColumn[i] <= -GlobalConfiguration::PIVOT_CHANGE_COLUMN_TOLERANCE )
            {
                // Nonbasic decreases, basic decreases
                double actualLowerBound;
                if ( basicCost < 0 )
                    continue;
                else if ( basicCost > 0 )
                    actualLowerBound = _upperBounds[basic];
                else
                    actualLowerBound = _lowerBounds[basic];

                ratioConstraintPerBasic = - ( actualLowerBound - _basicAssignment[i] ) / changeColumn[i];
            }
            else
            {
                // This basic does not depend on the entering variable
                continue;
            }

            if ( FloatUtils::isPositive( ratioConstraintPerBasic ) )
            {
                /*
                  This happens when the basic cost does not properly reflect the relation
                  between assignment and bound, due to some minor degeneracy. Assume that
                  assignment = bound, and set the cosntraint to 0.
                */
                ratioConstraintPerBasic = 0;
            }

            ASSERT( !FloatUtils::isPositive( ratioConstraintPerBasic ) );
            double pivot = FloatUtils::abs( changeColumn[i] );
            if ( ( ratioConstraintPerBasic >= optimalChangeRatio ) && ( pivot > largestPivot ) )
            {
                largestPivot = pivot;
                _leavingVariable = i;
                _changeRatio = ratioConstraintPerBasic;
                _leavingVariableIncreases = FloatUtils::isPositive( changeColumn[_leavingVariable] );
            }
        }
    }
    else
    {
        // Change ratios are positive
        double ratioConstraintPerBasic;
        for ( unsigned i = 0; i < _m; ++i )
        {
            unsigned basic = _basicIndexToVariable[i];
            double basicCost = _costFunctionManager->getBasicCost( i );
            if ( changeColumn[i] >= +GlobalConfiguration::PIVOT_CHANGE_COLUMN_TOLERANCE )
            {
                // Nonbasic increases, basic decreases
                double actualLowerBound;
                if ( basicCost < 0 )
                    continue;
                else if ( basicCost > 0 )
                    actualLowerBound = _upperBounds[basic];
                else
                    actualLowerBound = _lowerBounds[basic];

                ratioConstraintPerBasic = - ( actualLowerBound - _basicAssignment[i] ) / changeColumn[i];
            }
            else if ( changeColumn[i] <= -GlobalConfiguration::PIVOT_CHANGE_COLUMN_TOLERANCE )
            {
                // Nonbasic decreases, basic decreases
                double actualUpperBound;
                if ( basicCost > 0 )
                    continue;
                else if ( basicCost < 0 )
                    actualUpperBound = _lowerBounds[basic];
                else
                    actualUpperBound = _upperBounds[basic];

                ratioConstraintPerBasic = - ( actualUpperBound - _basicAssignment[i] ) / changeColumn[i];

            }
            else
            {
                // This basic does not depend on the entering variable
                continue;
            }

            if ( FloatUtils::isNegative( ratioConstraintPerBasic ) )
            {
                /*
                  This happens when the basic cost does not properly reflect the relation
                  between assignment and bound, due to some minor degeneracy. Assume that
                  assignment = bound, and set the cosntraint to 0.
                */
                ratioConstraintPerBasic = 0;
            }

            ASSERT( !FloatUtils::isNegative( ratioConstraintPerBasic ) );
            double pivot = FloatUtils::abs( changeColumn[i] );
            if ( ( ratioConstraintPerBasic <= optimalChangeRatio ) && ( pivot > largestPivot ) )
            {
                largestPivot = pivot;
                _leavingVariable = i;
                _changeRatio = ratioConstraintPerBasic;
                _leavingVariableIncreases = FloatUtils::isNegative( changeColumn[_leavingVariable] );
            }
        }
    }

    // Something must have been chosen
    ASSERT( _leavingVariable != _m );
}

double Tableau::getChangeRatio() const
{
    return _changeRatio;
}

void Tableau::setChangeRatio( double changeRatio )
{
    _changeRatio = changeRatio;
}

void Tableau::computeChangeColumn()
{
    // Compute d = inv(B) * a using the basis factorization
    const double *a = getAColumn( _nonBasicIndexToVariable[_enteringVariable] );
    _basisFactorization->forwardTransformation( a, _changeColumn );
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
        printf( "\tx%u (index: %u)  -->  %.5lf [%s]. ", i, _variableToIndex[i],
                getValue( i ), basic ? "B" : "NB" );
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
            printf( "%5.1lf ", _A->get( i, j ) );
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

    for ( unsigned i = 0; i < _n - _m; ++i )
    {
        row->_row[i]._var = _nonBasicIndexToVariable[i];
        row->_row[i]._coefficient = 0;

        SparseUnsortedList *column = _sparseColumnsOfA[_nonBasicIndexToVariable[i]];

        for ( const auto &entry : *column )
            row->_row[i]._coefficient -= ( _multipliers[entry._index] * entry._value );
    }

    /*
      If the rhs vector is all zeros, the row's scalar will be 0. This is
      the common case. If the rhs vector is not zero, we need to compute
      the scalar directly.
    */
    if ( _rhsIsAllZeros )
        row->_scalar = 0;
    else
    {
        _basisFactorization->forwardTransformation( _b, _workM );
        row->_scalar = _workM[index];
    }

    row->_lhs = _basicIndexToVariable[index];
}

const SparseMatrix *Tableau::getSparseA() const
{
    return _A;
}

const double *Tableau::getAColumn( unsigned variable ) const
{
    return _denseA + ( variable * _m );
}

void Tableau::getSparseAColumn( unsigned variable, SparseUnsortedList *result ) const
{
    _sparseColumnsOfA[variable]->storeIntoOther( result );
}

const SparseUnsortedList *Tableau::getSparseAColumn( unsigned variable ) const
{
    return _sparseColumnsOfA[variable];
}

const SparseUnsortedList *Tableau::getSparseARow( unsigned row ) const
{
    return _sparseRowsOfA[row];
}

void Tableau::getSparseARow( unsigned row, SparseUnsortedList *result ) const
{
    _sparseRowsOfA[row]->storeIntoOther( result );
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
    state.setDimensions( _m, _n, *this );

    // Store matrix A
    _A->storeIntoOther( state._A );
    for ( unsigned i = 0; i < _n; ++i )
        _sparseColumnsOfA[i]->storeIntoOther( state._sparseColumnsOfA[i] );
    for ( unsigned i = 0; i < _m; ++i )
        _sparseRowsOfA[i]->storeIntoOther( state._sparseRowsOfA[i] );
    memcpy( state._denseA, _denseA, sizeof(double) * _m * _n );

    // Store right hand side vector _b
    memcpy( state._b, _b, sizeof(double) * _m );

    // Store the bounds
    memcpy( state._lowerBounds, _lowerBounds, sizeof(double) *_n );
    memcpy( state._upperBounds, _upperBounds, sizeof(double) *_n );

    // Basic variables
    state._basicVariables = _basicVariables;

    // Store the assignments
    memcpy( state._basicAssignment, _basicAssignment, sizeof(double) *_m );
    memcpy( state._nonBasicAssignment, _nonBasicAssignment, sizeof(double) * ( _n - _m ) );
    state._basicAssignmentStatus = _basicAssignmentStatus;

    // Store the indices
    memcpy( state._basicIndexToVariable, _basicIndexToVariable, sizeof(unsigned) * _m );
    memcpy( state._nonBasicIndexToVariable, _nonBasicIndexToVariable, sizeof(unsigned) * ( _n - _m ) );
    memcpy( state._variableToIndex, _variableToIndex, sizeof(unsigned) * _n );

    // Store the basis factorization
    _basisFactorization->storeFactorization( state._basisFactorization );

    // Store the _boundsValid indicator
    state._boundsValid = _boundsValid;

    // Store the merged variables
    state._mergedVariables = _mergedVariables;

    // Store bounds explanations
    if ( GlobalConfiguration::PROOF_CERTIFICATE )
		*state._boundsExplanator = *_boundsExplanator;
}

void Tableau::restoreState( const TableauState &state )
{
    freeMemoryIfNeeded();
    setDimensions( state._m, state._n );

    // Restore matrix A
    state._A->storeIntoOther( _A );
    for ( unsigned i = 0; i < _n; ++i )
        state._sparseColumnsOfA[i]->storeIntoOther( _sparseColumnsOfA[i] );
    for ( unsigned i = 0; i < _m; ++i )
        state._sparseRowsOfA[i]->storeIntoOther( _sparseRowsOfA[i] );
    memcpy( _denseA, state._denseA, sizeof(double) * _m * _n );

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

    // Restore the merged variables
    _mergedVariables = state._mergedVariables;

    // Restore bounds explanations
    if( GlobalConfiguration::PROOF_CERTIFICATE )
		*_boundsExplanator = *state._boundsExplanator;


    computeAssignment();
    _costFunctionManager->initialize();
    computeCostFunction();

    if ( _statistics )
        _statistics->setCurrentTableauDimension( _m, _n );
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
    /* TODO erase - since explanations are already updated via rowBoundTightener
    // Update only for a basic var
    if ( GlobalConfiguration::PROOF_CERTIFICATE && _basicVariables.exists(variable) )
    {
        TableauRow row = TableauRow( _n );
        getTableauRow( _variableToIndex[variable], &row );
        _boundsExplanator->updateBoundExplanation( row, false );
    }
    */
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
    /* TODO erase - since explanations are already updated via rowBoundTightener
    // Update only for a basic var
    if ( GlobalConfiguration::PROOF_CERTIFICATE && _basicVariables.exists(variable) )
    {
        TableauRow row = TableauRow( _n );
        getTableauRow( _variableToIndex[variable], &row );
        _boundsExplanator->updateBoundExplanation( row, true );
    }
    */
}

unsigned Tableau::addEquation( const Equation &equation )
{
    // The fresh auxiliary variable assigned to the equation is _n.
    // This variable is implicitly added to the equation, with
    // coefficient 1.
    unsigned auxVariable = _n;

    // Adjust the data structures
    addRow();

    // Adjust the constraint matrix
    _A->addEmptyColumn();
    std::fill_n( _workN, _n, 0.0 );
    for ( const auto &addend : equation._addends )
    {
        _workN[addend._variable] = addend._coefficient;
        _sparseColumnsOfA[addend._variable]->set( _m - 1, addend._coefficient );
        _sparseRowsOfA[_m - 1]->set( addend._variable, addend._coefficient );
        _denseA[(addend._variable * _m) + _m - 1] = addend._coefficient;
    }

    _workN[auxVariable] = 1;
    _sparseColumnsOfA[auxVariable]->set( _m - 1, 1 );
    _sparseRowsOfA[_m - 1]->set( auxVariable, 1 );
    _denseA[(auxVariable * _m) + _m - 1] = 1;
    _A->addLastRow( _workN );

    // Invalidate the cost function, so that it is recomputed in the next iteration.
    _costFunctionManager->invalidateCostFunction();

    // All variables except the new one have finite bounds. Use this to compute
    // finite bounds for the new variable.
    double lb = equation._scalar;
    double ub = equation._scalar;

    for ( const auto &addend : equation._addends )
    {
        double coefficient = addend._coefficient;
        unsigned variable = addend._variable;

        if ( FloatUtils::isPositive( coefficient ) )
        {
            lb -= coefficient * _upperBounds[variable];
            ub -= coefficient * _lowerBounds[variable];
        }
        else
        {
            lb -= coefficient * _lowerBounds[variable];
            ub -= coefficient * _upperBounds[variable];
        }
    }

    setLowerBound( auxVariable, lb );
    setUpperBound( auxVariable, ub );

    // Populate the new row of b
    _b[_m - 1] = equation._scalar;

    if ( !FloatUtils::isZero( _b[_m - 1] ) )
        _rhsIsAllZeros = false;

    /*
      Attempt to make the auxiliary variable the new basic variable.
      This usually works.
      If it doesn't, compute a new set of basic variables and re-initialize
      the tableau (which is more computationally expensive)
    */
    _basicIndexToVariable[_m - 1] = auxVariable;
    _variableToIndex[auxVariable] = _m - 1;
    _basicVariables.insert( auxVariable );

    // Attempt to refactorize the basis
    bool factorizationSuccessful = true;
    try
    {
        _basisFactorization->obtainFreshBasis();
    }
    catch ( MalformedBasisException & )
    {
        factorizationSuccessful = false;
    }

    if ( factorizationSuccessful )
    {
        // Compute the assignment for the new basic variable
        _basicAssignment[_m - 1] = equation._scalar;
        for ( const auto &addend : equation._addends )
        {
            _basicAssignment[_m - 1] -= addend._coefficient * getValue( addend._variable );
        }

        ASSERT( FloatUtils::wellFormed( _basicAssignment[_m - 1] ) );

        if ( FloatUtils::isZero( _basicAssignment[_m - 1] ) )
            _basicAssignment[_m - 1] = 0.0;

        // Notify about the new variable's assignment and compute its status
        notifyVariableValue( _basicIndexToVariable[_m - 1], _basicAssignment[_m - 1] );
        computeBasicStatus( _m - 1 );
    }
    else
    {
        ConstraintMatrixAnalyzer analyzer;
        analyzer.analyze( (const SparseUnsortedList **)_sparseRowsOfA, _m, _n );
        List<unsigned> independentColumns = analyzer.getIndependentColumns();

        try
        {
            initializeTableau( independentColumns );
        }
        catch ( MalformedBasisException & )
        {
            TABLEAU_LOG( "addEquation failed - could not refactorize basis" );
            throw MarabouError( MarabouError::FAILURE_TO_ADD_NEW_EQUATION );
        }

        computeCostFunction();
    }

    return auxVariable;
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

    // Allocate a larger _sparseColumnsOfA, keep old ones
    SparseUnsortedList **newSparseColumnsOfA = new SparseUnsortedList *[newN];
    if ( !newSparseColumnsOfA )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::newSparseColumnsOfA" );

    for ( unsigned i = 0; i < _n; ++i )
    {
        newSparseColumnsOfA[i] = _sparseColumnsOfA[i];
        newSparseColumnsOfA[i]->incrementSize();
    }

    newSparseColumnsOfA[newN - 1] = new SparseUnsortedList( newM );
    if ( !newSparseColumnsOfA[newN - 1] )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::newSparseColumnsOfA[newN-1]" );

    delete[] _sparseColumnsOfA;
    _sparseColumnsOfA = newSparseColumnsOfA;

    // Allocate a larger _sparseRowsOfA, keep old ones
    SparseUnsortedList **newSparseRowsOfA = new SparseUnsortedList *[newM];
    if ( !newSparseRowsOfA )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::newSparseRowsOfA" );

    for ( unsigned i = 0; i < _m; ++i )
    {
        newSparseRowsOfA[i] = _sparseRowsOfA[i];
        newSparseRowsOfA[i]->incrementSize();
    }

    newSparseRowsOfA[newM - 1] = new SparseUnsortedList( newN );
    if ( !newSparseRowsOfA[newM - 1] )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::newSparseRowsOfA[newN-1]" );

    delete[] _sparseRowsOfA;
    _sparseRowsOfA = newSparseRowsOfA;

    // Allocate a larger _denseA, keep old entries
    double *newDenseA = new double[newM * newN];
    if ( !newDenseA )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::newDenseA" );

    for ( unsigned column = 0; column < _n; ++column )
    {
        memcpy( newDenseA + ( column * newM ), _denseA + ( column * _m ), sizeof(double) * _m );
        newDenseA[column*newM + newM - 1] = 0.0;
    }
    std::fill_n( newDenseA + ( newN - 1 ) * newM, newM, 0.0 );

    delete[] _denseA;
    _denseA = newDenseA;

    // Allocate a new changeColumn. Don't need to initialize
    double *newChangeColumn = new double[newM];
    if ( !newChangeColumn )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::newChangeColumn" );
    delete[] _changeColumn;
    _changeColumn = newChangeColumn;

    // Allocate a new b and copy the old values
    double *newB = new double[newM];
    if ( !newB )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::newB" );
    std::fill( newB + _m, newB + newM, 0.0 );
    memcpy( newB, _b, _m * sizeof(double) );
    delete[] _b;
    _b = newB;

    // Allocate a new unit vector. Don't need to initialize
    double *newUnitVector = new double[newM];
    if ( !newUnitVector )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::newUnitVector" );
    delete[] _unitVector;
    _unitVector = newUnitVector;

    // Allocate new multipliers. Don't need to initialize
    double *newMultipliers = new double[newM];
    if ( !newMultipliers )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::newMultipliers" );
    delete[] _multipliers;
    _multipliers = newMultipliers;

    // Allocate new index arrays. Copy old indices, but don't assign indices to new variables yet.
    unsigned *newBasicIndexToVariable = new unsigned[newM];
    if ( !newBasicIndexToVariable )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::newBasicIndexToVariable" );
    memcpy( newBasicIndexToVariable, _basicIndexToVariable, _m * sizeof(unsigned) );
    delete[] _basicIndexToVariable;
    _basicIndexToVariable = newBasicIndexToVariable;

    unsigned *newVariableToIndex = new unsigned[newN];
    if ( !newVariableToIndex )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::newVariableToIndex" );
    memcpy( newVariableToIndex, _variableToIndex, _n * sizeof(unsigned) );
    delete[] _variableToIndex;
    _variableToIndex = newVariableToIndex;

    // Allocate a new basic assignment vector, copy old values
    double *newBasicAssignment = new double[newM];
    if ( !newBasicAssignment )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::newAssignment" );
    memcpy( newBasicAssignment, _basicAssignment, sizeof(double) * _m );
    delete[] _basicAssignment;
    _basicAssignment = newBasicAssignment;

    unsigned *newBasicStatus = new unsigned[newM];
    if ( !newBasicStatus )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::newBasicStatus" );
    memcpy( newBasicStatus, _basicStatus, sizeof(unsigned) * _m );
    delete[] _basicStatus;
    _basicStatus = newBasicStatus;

    // Allocate new lower and upper bound arrays, and copy old values
    double *newLowerBounds = new double[newN];
    if ( !newLowerBounds )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::newLowerBounds" );
    memcpy( newLowerBounds, _lowerBounds, _n * sizeof(double) );
    delete[] _lowerBounds;
    _lowerBounds = newLowerBounds;

    double *newUpperBounds = new double[newN];
    if ( !newUpperBounds )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::newUpperBounds" );
    memcpy( newUpperBounds, _upperBounds, _n * sizeof(double) );
    delete[] _upperBounds;
    _upperBounds = newUpperBounds;

    // Mark the new variable as unbounded
    _lowerBounds[_n] = FloatUtils::negativeInfinity();
    _upperBounds[_n] = FloatUtils::infinity();

    // Allocate a larger basis factorization
    IBasisFactorization *newBasisFactorization =
        BasisFactorizationFactory::createBasisFactorization( newM, *this );
    if ( !newBasisFactorization )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::newBasisFactorization" );
    delete _basisFactorization;
    _basisFactorization = newBasisFactorization;
    _basisFactorization->setStatistics( _statistics );

    // Allocate a larger _workM and _workN. Don't need to initialize.
    double *newWorkM = new double[newM];
    if ( !newWorkM )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::newWorkM" );
    delete[] _workM;
    _workM = newWorkM;

    double *newWorkN = new double[newN];
    if ( !newWorkN )
        throw MarabouError( MarabouError::ALLOCATION_FAILED, "Tableau::newWorkN" );
    delete[] _workN;
    _workN = newWorkN;

    _m = newM;
    _n = newN;
    _costFunctionManager->initialize();

    for ( const auto &watcher : _resizeWatchers )
        watcher->notifyDimensionChange( _m, _n );

    if ( _statistics )
    {
        _statistics->incNumAddedRows();
        _statistics->setCurrentTableauDimension( _m, _n );
    }

    if (GlobalConfiguration::PROOF_CERTIFICATE)
        _boundsExplanator = new BoundsExplanator(_n, _m); //TODO erase after registration as resizeWatcher
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

void Tableau::registerResizeWatcher( ResizeWatcher *watcher )
{
    _resizeWatchers.append( watcher );
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

void Tableau::verifyInvariants()
{
    // All merged variables are non-basic
    for ( const auto &merged : _mergedVariables )
    {
        if ( _basicVariables.exists( merged.first ) )
        {
            printf( "Error! Merged variable x%u is basic!\n", merged.first );
            exit( 1 );
        }
    }

    // All assignments are well formed
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
        unsigned variable = _nonBasicIndexToVariable[i];
        double value = _nonBasicAssignment[i];

        double lb = _lowerBounds[variable];
        double relaxedLb =
            lb -
            ( GlobalConfiguration::BOUND_COMPARISON_ADDITIVE_TOLERANCE +
              GlobalConfiguration::BOUND_COMPARISON_MULTIPLICATIVE_TOLERANCE * FloatUtils::abs( lb ) );

        double ub = _upperBounds[variable];
        double relaxedUb =
            ub +
            ( GlobalConfiguration::BOUND_COMPARISON_ADDITIVE_TOLERANCE +
              GlobalConfiguration::BOUND_COMPARISON_MULTIPLICATIVE_TOLERANCE * FloatUtils::abs( ub ) );

        if ( !( ( relaxedLb <= value ) && ( value <= relaxedUb ) ) )
        {
            // This behavior is okay iff lb > ub, and this is going to be caught
            // soon anyway

            if ( FloatUtils::gt( _lowerBounds[variable], _upperBounds[variable] ) )
                continue;

            printf( "Tableau test invariants: bound violation!\n" );
            printf( "Variable %u (non-basic #%u). Assignment: %.15lf. Range: [%.15lf, %.15lf]\n",
                    variable, i, _nonBasicAssignment[i], _lowerBounds[variable], _upperBounds[variable] );
            printf( "RelaxedLB = %.15lf. RelaxedUB = %.15lf\n", relaxedLb, relaxedUb );

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

    case BETWEEN:
        return "BETWEEN";

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

        double enteringReducedCost = _costFunctionManager->getCostFunction()[_enteringVariable];
        bool nonBasicDecreases = ( enteringReducedCost >= +GlobalConfiguration::ENTRY_ELIGIBILITY_TOLERANCE );

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
    double normalizedError = _costFunctionManager->updateCostFunctionForPivot( _enteringVariable,
                                                                               _leavingVariable,
                                                                               pivotElement,
                                                                               _pivotRow,
                                                                               _changeColumn );

    if ( FloatUtils::gt( normalizedError, GlobalConfiguration::COST_FUNCTION_ERROR_THRESHOLD ) )
        _costFunctionManager->invalidateCostFunction();
}

ITableau::BasicAssignmentStatus Tableau::getBasicAssignmentStatus() const
{
    return _basicAssignmentStatus;
}

double Tableau::getBasicAssignment( unsigned basicIndex ) const
{
    return _basicAssignment[basicIndex];
}

void Tableau::setBasicAssignmentStatus( ITableau::BasicAssignmentStatus status )
{
    _basicAssignmentStatus = status;
}

void Tableau::makeBasisMatrixAvailable()
{
    return _basisFactorization->makeExplicitBasisAvailable();
}

bool Tableau::basisMatrixAvailable() const
{
    return _basisFactorization->explicitBasisAvailable();
}

double *Tableau::getInverseBasisMatrix() const
{
    ASSERT( basisMatrixAvailable() );

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

void Tableau::getColumnOfBasis( unsigned column, double *result ) const
{
    ASSERT( column < _m );
    ASSERT( !_mergedVariables.exists( _basicIndexToVariable[column] ) );

    _sparseColumnsOfA[_basicIndexToVariable[column]]->toDense( result );
}

void Tableau::getSparseBasis( SparseColumnsOfBasis &basis ) const
{
    for ( unsigned i = 0; i < _m; ++i )
        basis._columns[i] = _sparseColumnsOfA[_basicIndexToVariable[i]];
}

void Tableau::getColumnOfBasis( unsigned column, SparseUnsortedList *result ) const
{
    ASSERT( column < _m );
    ASSERT( !_mergedVariables.exists( _basicIndexToVariable[column] ) );

    _sparseColumnsOfA[_basicIndexToVariable[column]]->storeIntoOther( result );
}

void Tableau::refreshBasisFactorization()
{
    _basisFactorization->obtainFreshBasis();
}

unsigned Tableau::getVariableAfterMerging( unsigned variable ) const
{
    unsigned answer = variable;

    while ( _mergedVariables.exists( answer ) )
        answer = _mergedVariables[answer];

    return answer;
}

void Tableau::mergeColumns( unsigned x1, unsigned x2 )
{
    ASSERT( !isBasic( x1 ) );
    ASSERT( !isBasic( x2 ) );

    /*
      If x2 has tighter bounds than x1, adjust the bounds
      for x1.
    */
    if ( FloatUtils::lt( _upperBounds[x2], _upperBounds[x1] ) )
        tightenUpperBound( x1, _upperBounds[x2] );
    if ( FloatUtils::gt( _lowerBounds[x2], _lowerBounds[x1] ) )
        tightenLowerBound( x1, _lowerBounds[x2] );

    /*
      Merge column x2 of the constraint matrix into x1
      and zero-out column x2
    */
    _A->mergeColumns( x1, x2 );
    _mergedVariables[x2] = x1;

    // Adjust sparse columns and rows, also
    _sparseColumnsOfA[x2]->clear();
    _A->getColumn( x1, _sparseColumnsOfA[x1] );

    for ( unsigned i = 0; i < _m; ++i )
        _sparseRowsOfA[i]->mergeEntries( x2, x1 );

    // And the dense ones, too
    for ( unsigned i = 0; i < _m; ++i )
        _denseA[x1*_m + i] += _denseA[x2*_m + i];
    std::fill_n( _denseA + x2 * _m, _m, 0 );

    computeAssignment();
    computeCostFunction();

    if ( _statistics )
        _statistics->incNumMergedColumns();
}

bool Tableau::areLinearlyDependent( unsigned x1, unsigned x2, double &coefficient, double &inverseCoefficient )
{
    bool oneIsBasic = isBasic( x1 );
    bool twoIsBasic = isBasic( x2 );

    // If both are basic or both or non-basic, they are
    // independent
    if ( oneIsBasic == twoIsBasic )
        return false;

    // One is basic and one isnt
    unsigned basic = oneIsBasic ? x1 : x2;
    unsigned nonBasic = oneIsBasic ? x2 : x1;

    // Find the column of the non-basic
    const double *a = getAColumn( nonBasic );
    _basisFactorization->forwardTransformation( a, _workM );

    // Find the correct entry in the column
    unsigned basicIndex = _variableToIndex[basic];

    coefficient = -_workM[basicIndex];

    _basisFactorization->forwardTransformation( _b, _workM );

    // Coefficient is zero - independent!
    if ( FloatUtils::isZero( coefficient ) )
        return false;

    /*
      The variable are linearly dependent. We want the coefficient c such that

        x2 = ... + c * x1 + ...

      right now we know that

        basic = ... + c * nonBasic + ...
    */

    if ( basic != x2 )
    {
        inverseCoefficient = coefficient;
        coefficient = 1 / coefficient;
    }
    else
        inverseCoefficient = 1 / coefficient;

    return true;
}

int Tableau::getInfeasibleRow( TableauRow& row )
{
    for ( unsigned i = 0; i < _m; ++i )
    {
        if ( basicOutOfBounds( i )  )
        {
            Tableau::getTableauRow( i, &row );
            if ( ( computeRowBound( row, true ) < _lowerBounds[row._lhs] || computeRowBound( row, false ) > _upperBounds[row._lhs] ) )
                return i;
        }
    }
    return -1;
}

int Tableau::getInfeasibleVar() const
{
    for (unsigned i = 0; i < _n; ++i)
        if (_lowerBounds[i] > _upperBounds[i])
            return i;  
    return -1;
}

SingleVarBoundsExplanator* Tableau::ExplainBound( const unsigned variable ) const
{
    ASSERT( GlobalConfiguration::PROOF_CERTIFICATE && variable < _n );
    return &_boundsExplanator->returnWholeVarExplanation( variable );
}

void Tableau::updateExplanation( const TableauRow& row, const bool isUpper ) const
{
    if ( GlobalConfiguration::PROOF_CERTIFICATE )
        _boundsExplanator->updateBoundExplanation( row, isUpper );
}

void Tableau::updateExplanation( const TableauRow& row, const bool isUpper, unsigned varIndex ) const
{
    if ( GlobalConfiguration::PROOF_CERTIFICATE )
        _boundsExplanator->updateBoundExplanation( row, isUpper, varIndex );
}

void Tableau::updateExplanation( const SparseUnsortedList& row, const bool isUpper, unsigned var ) const
{
    if ( GlobalConfiguration::PROOF_CERTIFICATE )
        _boundsExplanator->updateBoundExplanationSparse( row, isUpper, var );
}

double Tableau::computeRowBound( const TableauRow& row, const bool isUpper ) const
{
    double bound = 0, multiplier;
    unsigned var;
    for ( unsigned i = 0; i < row._size; ++i )
    {
        var = row._row[i]._var;
        if( FloatUtils::isZero( row[i] ) )
            continue;

        multiplier = (isUpper && row[i] > 0) || (!isUpper && row[i] < 0)? _upperBounds[var] : _lowerBounds[var];
        multiplier *= row[i];
        bound += multiplier;
    }

    return bound;
}

void Tableau::multiplyExplanationCoefficients (const unsigned var, const double alpha, const bool isUpper)
{
	_boundsExplanator->multiplyExplanationCoefficients( var, alpha, isUpper );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
