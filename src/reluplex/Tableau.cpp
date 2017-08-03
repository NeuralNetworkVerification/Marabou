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

#include "BasisFactorization.h"
#include "Debug.h"
#include "EntrySelectionStrategy.h"
#include "Equation.h"
#include "FloatUtils.h"
#include "MStringf.h"
#include "PiecewiseLinearCaseSplit.h"
#include "ReluplexError.h"
#include "Tableau.h"
#include "TableauRow.h"
#include "TableauState.h"

#include <cfloat>
#include <string.h>

Tableau::Tableau()
    : _A( NULL )
    , _a( NULL )
    , _d( NULL )
    , _b( NULL )
    , _rhs( NULL )
    , _basisFactorization( NULL )
    , _costFunction( NULL )
    , _basicCosts( NULL )
    , _multipliers( NULL )
    , _basicIndexToVariable( NULL )
    , _nonBasicIndexToVariable( NULL )
    , _variableToIndex( NULL )
    , _nonBasicAssignment( NULL )
    , _lowerBounds( NULL )
    , _upperBounds( NULL )
    , _boundsValid( true )
    , _basicAssignment( NULL )
    , _basicAssignmentStatus( ASSIGNMENT_INVALID )
    , _basicStatus( NULL )
    , _statistics( NULL )
    , _usingSteepestEdge( true )
    , _steepestEdgeGamma( NULL )
    , _alpha( NULL )
    , _nu( NULL )
    , _work( NULL )
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

    if ( _d )
    {
        delete[] _d;
        _d = NULL;
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

    if ( _costFunction )
    {
        delete[] _costFunction;
        _costFunction = NULL;
    }

    if ( _basicCosts )
    {
        delete[] _basicCosts;
        _basicCosts = NULL;
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

    if ( _steepestEdgeGamma )
    {
        delete[] _steepestEdgeGamma;
        _steepestEdgeGamma = NULL;
    }

    if ( _alpha )
    {
        delete[] _alpha;
        _alpha = NULL;
    }

    if ( _nu )
    {
        delete[] _nu;
        _nu = NULL;
    }

    if ( _work )
    {
        delete[] _work;
        _work = NULL;
    }

    if ( _rhs )
    {
        delete[] _rhs;
        _rhs = NULL;
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

    _d = new double[m];
    if ( !_d )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::d" );

    _b = new double[m];
    if ( !_b )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::b" );

    _unitVector = new double[m];
    if ( !_unitVector )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::unitVector" );

    _costFunction = new double[n-m];
    if ( !_costFunction )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::costFunction" );

    _basicCosts = new double[m];
    if ( !_basicCosts )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::basicCosts" );

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

    _basisFactorization = new BasisFactorization( _m );
    if ( !_basisFactorization )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::basisFactorization" );

    _steepestEdgeGamma = new double[n-m];
    if ( !_steepestEdgeGamma )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::steepestEdgeGamma" );

    _alpha = new double[n-m];
    if ( !_alpha )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::alpha" );

    _nu = new double[n-m];
    if ( !_nu )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::nu" );

    _work = new double[m];
    if ( !_work )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::work" );

    _rhs = new double[m];
    if ( !_rhs )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::rhs" );
}

void Tableau::setEntryValue( unsigned row, unsigned column, double value )
{
    _A[(column * _m) + row] = value;
}

void Tableau::markAsBasic( unsigned variable )
{
    _basicVariables.insert( variable );
}

void Tableau::initializeTableau()
{
    unsigned basicIndex = 0;
    unsigned nonBasicIndex = 0;

    // Assign variable indices
    for ( unsigned i = 0; i < _n; ++i )
    {
        if ( _basicVariables.exists( i ) )
        {
            _basicIndexToVariable[basicIndex] = i;
            _variableToIndex[i] = basicIndex;
            ++basicIndex;
        }
        else
        {
            _nonBasicIndexToVariable[nonBasicIndex] = i;
            _variableToIndex[i] = nonBasicIndex;
            ++nonBasicIndex;
        }
    }
    ASSERT( basicIndex + nonBasicIndex == _n );

    // Set non-basics to lower bounds
    for ( unsigned i = 0; i < _n - _m; ++i )
    {
        unsigned nonBasic = _nonBasicIndexToVariable[i];
        setNonBasicAssignment( nonBasic, _lowerBounds[nonBasic] );
    }

    // Recompute assignment
    computeAssignment();

    // Initialize gamma values for steepest edge
    if ( _usingSteepestEdge )
        initializeGamma();
}

const double *Tableau::getSteepestEdgeGamma() const
{
    return _steepestEdgeGamma;
}

void Tableau::initializeGamma()
{
    /*
     * Initialize gamma value for each nonbasic variable.
     *
     * gamma[i] = ||p[i]||^2
     *     p[i] = [ -inv(B)*AN*e[i]; e[i] ]
     *          = [ AN[i]; e[i] ]
     * where AN[i] is the ith column of the matrix AN
     */
    // Initialize gamma value for each nonbasic variable
    for ( unsigned i = 0; i < _n - _m; ++i )
    {
        // Find corresponding column in matrix B
        unsigned var = _nonBasicIndexToVariable[i];
        double *ANColumn = _A + ( var * _m ); // AN[i]

        // Euclidean norm of vector is sum of square of each element
        _steepestEdgeGamma[i] = 1.0; // To account for (m+i)th element of p[i]
        for ( unsigned j = 0; j < _m; ++j )
        {
            // Account for AN[i]
            _steepestEdgeGamma[i] += ANColumn[j] * ANColumn[j];
        }
    }
}

void Tableau::updateGamma()
{
    /*
     * Update gamma values used in steepest edge pivot selection when performing a pivot.
     *
     * Update rule based on Forrest and Goldfarb
     * Given entering q, leaving p,
     *
     * gammaNew[p] = gamma[q] / alpha[q]**2
     * gammaNew[j] = gamma[j] - 2*alpha[j]/alpha[q]*nu[j] + (alpha[j]/alpha[q])**2*gamma[q]
     *   for all other non-basic vars j =/= q
     *
     * where
     *   alpha[q] = sigma'*A[q]
     *   nu[j] = w'*A[j]
     *   A[i] = column of A corresponding to variable i
     *   B'sigma = e[p]
     *   BB'w = A[q]
     *
     * Additionally, we notice that alpha[i] = i-th component of p-th row of inv(B)*A =
     * p-th component of the vector inv(B)*A[i], and we never have to compute sigma
     * explicitly. Also,
     *   w = inv(B')*inv(B)*A[q], so
     *   nu[j] = w'*A[j] = (inv(B)*A[q])'*inv(B)*A[j]
     *
     * Assuming LU factorization is cached and optimized, this should be just as fast
     * as computing sigma via forward transform w transposed B matrix.
     *
     * TODO: Test the comparative efficiency of the following alternative procedure..
     * There seems to be no way currently in BasisFactorization to solve B'*x = y easily
     * (you can only solve B*x = y). We should see if computing sigma and w explicitly speeds
     * this up.
     */
    //    printf("\nPerforming update on gamma...\n");

    unsigned p = _leavingVariable; // basic -> nonbasic
    unsigned q = _enteringVariable; // nonbasic -> basic
    double *gamma = _steepestEdgeGamma;
    //    printf("Initial gamma: ");
    //    printVector(gamma, _n - _m);
    //    printf("\n");
    //    printf("numBasic: %d, numNonBasic: %d, p: %d, q: %d\n", _m, _n - _m, p, q);

    ASSERT( p < _m );
    ASSERT( q < _n - _m );

    double *ANColumnQ = _A + ( _nonBasicIndexToVariable[q] * _m );

    // inv(B)*A[q] vector (size m)
    double *invB_Aq = new double[_m];
    _basisFactorization->forwardTransformation( ANColumnQ, invB_Aq );

    // Compute alphas and nus. These should be different every update.
    double *alpha = _alpha;
    double *nu = _nu;
    double *work = _work; // to store inv(B)*A[j]
    double *ANColumn;

    // Store alpha[q]. Compute gamma for entering var separately
    alpha[q] = invB_Aq[p];

    for ( unsigned j = 0; j < _n - _m; ++j )
    {
        // j == q
        if ( j == q )
            continue;

        // j != q
        unsigned var = _nonBasicIndexToVariable[j];
        ANColumn = _A + ( var * _m );

        // Compute inv(B)*A[j]
        _basisFactorization->forwardTransformation( ANColumn, work );
        alpha[j] = work[p];
        nu[j] = dotProduct(invB_Aq, work, _m); 	// w'*A[j] = (inv(B)*A[q])'*inv(B)*A[j]

        double alphaBarJ = alpha[j] / alpha[q];
        gamma[j] = gamma[j] - 2*alphaBarJ*nu[j] + alphaBarJ*alphaBarJ*gamma[q];
    }

    gamma[q] = gamma[q] / ( alpha[q] * alpha[q] );
    // this assumes p replaces q directly and there's no strange sorting of basic/nonbasic vars

    //    printf("New gamma: ");
    //    printVector(gamma, _n - _m);
    //    printf("\n");
}

void Tableau::printVector( const double *v, unsigned m )
{
    // For debugging
    for ( unsigned i = 0; i < m; i++ )
        printf( "%f ", v[i] );
}

double Tableau::dotProduct(const double *a, const double *b, unsigned m)
{
    // Helper function to multiply two vectors of size m
    double result = 0;
    for ( unsigned i = 0; i < m; ++i )
        result += a[i] * b[i];
    return result;
}

void Tableau::useSteepestEdge( bool flag )
{
    _usingSteepestEdge = flag;
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

      We first compute y, and then do an FTRAN pass to solve B*xB = y
    */

    double *y = new double[_m];
    memcpy( y, _b, sizeof(double) * _m );

    // Compute a linear combination of the columns of AN
    const double *ANColumn;
    for ( unsigned i = 0; i < _n - _m; ++i )
    {
        unsigned var = _nonBasicIndexToVariable[i];
        double value = _nonBasicAssignment[i];

        ANColumn = _A + ( var * _m );
        for ( unsigned j = 0; j < _m; ++j )
            y[j] -= ANColumn[j] * value;
    }

    // Solve B*xB = y by performing a forward transformation
    _basisFactorization->forwardTransformation( y, _basicAssignment );

    delete[] y;

    computeBasicStatus();
    _basicAssignmentStatus = ASSIGNMENT_VALID;

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

    if ( FloatUtils::gt( value , ub ) )
        _basicStatus[basic] = Tableau::ABOVE_UB;
    else if ( FloatUtils::lt( value , lb ) )
        _basicStatus[basic] = Tableau::BELOW_LB;
    else if ( FloatUtils::areEqual( ub, value ) )
        _basicStatus[basic] = Tableau::AT_UB;
    else if ( FloatUtils::areEqual( lb, value ) )
        _basicStatus[basic] = Tableau::AT_LB;
    else
        _basicStatus[basic] = Tableau::BETWEEN;
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

double Tableau::getValue( unsigned variable )
{
    if ( !_basicVariables.exists( variable ) )
    {
        // The values of non-basics can be extracted even if the
        // assignment is invalid
        unsigned index = _variableToIndex[variable];
        return _nonBasicAssignment[index];
    }

    // Values of basic variabels require valid assignments
    // TODO: maybe we should compute just that one variable?
    if ( _basicAssignmentStatus != ASSIGNMENT_VALID )
        computeAssignment();

    return _basicAssignment[_variableToIndex[variable]];
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
    return _costFunction;
}

void Tableau::dumpCostFunction() const
{
    // printf( "Cost function:\n\t" );

    for ( unsigned i = 0; i < _n - _m; ++i )
    {
        double coefficient = _costFunction[i];
        if ( FloatUtils::isZero( coefficient ) )
            continue;

        // if ( FloatUtils::isPositive( coefficient ) )
        //     printf( "+" );
        // printf( "%lfx%u ", coefficient, _nonBasicIndexToVariable[i] );
    }

    // printf( "\n" );
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
    /*
      The cost function is computed in three steps:

      1. Compute the basic costs c.
         These costs indicate whether a basic variable's row in
         the tableau should be added as is (variable too great;
         cost = 1), should be added negatively (variable is too
         small; cost = -1), or should be ignored (variable
         within bounds; cost = 0).

      2. Compute the multipliers p.
         p = c' * inv(B)
         This is solved by invoking BTRAN for pB = c'

      3. Compute the non-basic (reduced) costs.
         These are given by -p * AN

      Comment: the correctness follows from the fact that

      xB = inv(B)(b - AN xN)

      we ignore b because the constants don't matter for the cost
      function, and we omit xN because we want the function and not an
      evaluation thereof on a specific point.
    */

    // Step 1: compute basic costs
    computeBasicCosts();

    // Step 2: compute the multipliers
    computeMultipliers();

    // Step 3: compute reduced costs
    computeReducedCosts();
}

void Tableau::computeMultipliers()
{
    computeMultipliers( _basicCosts );
}

void Tableau::computeBasicCosts()
{
    for ( unsigned i = 0; i < _m; ++i )
    {
        if ( basicTooLow( i ) )
            _basicCosts[i] = -1;
        else if ( basicTooHigh( i ) )
            _basicCosts[i] = 1;
        else
            _basicCosts[i] = 0;
    }

    // printf( "Dumping basic costs:\n\t" );
    // for ( unsigned i = 0; i < _m; ++i )
    // {
    //     if ( FloatUtils::isZero( _basicCosts[i] ) )
    //         continue;

    //     if ( FloatUtils::isPositive( _basicCosts[i] ) )
    //         printf( "+" );
    //     printf( "%lfx%u ", _basicCosts[i], _basicIndexToVariable[i] );
    // }
    // printf( "\n" );
}

void Tableau::computeMultipliers( double *rowCoefficients )
{
    _basisFactorization->backwardTransformation( rowCoefficients, _multipliers );

    // printf( "Dumping multipliers:\n\t" );
    // for ( unsigned i = 0; i < _m; ++i )
    //     printf( "%lf ", _multipliers[i] );

    // printf( "\n" );
}

void Tableau::computeReducedCost( unsigned nonBasic )
{
    double *ANColumn = _A + ( _nonBasicIndexToVariable[nonBasic] * _m );
    _costFunction[nonBasic] = 0;
    for ( unsigned j = 0; j < _m; ++j )
    {
        _costFunction[nonBasic] -= ( _multipliers[j] * ANColumn[j] );
    }
}

void Tableau::computeReducedCosts()
{
    for ( unsigned i = 0; i < _n - _m; ++i )
    {
        computeReducedCost(i);
    }
}

unsigned Tableau::getBasicStatus( unsigned basic )
{
    return _basicStatus[_variableToIndex[basic]];
}

void Tableau::getEntryCandidates( List<unsigned> &candidates ) const
{
    candidates.clear();
    for ( unsigned i = 0; i < _n - _m; ++i )
    {
        if ( eligibleForEntry( i ) )
            candidates.append( i );
    }
}

void Tableau::setEnteringVariable( unsigned nonBasic )
{
    _enteringVariable = nonBasic;
}

void Tableau::setLeavingVariable( unsigned basic )
{
    // ONLY FOR TESTING STEEPEST EDGE. NEVER USE THIS! Use pickLeavingVariable instead.
    _leavingVariable = basic;
}

bool Tableau::eligibleForEntry( unsigned nonBasic ) const
{
    // A non-basic variable is eligible for entry if one of the two
    //   conditions holds:
    //
    //   1. It has a negative coefficient in the cost function and it
    //      can increase
    //   2. It has a positive coefficient in the cost function and it
    //      can decrease

    if ( FloatUtils::isZero( _costFunction[nonBasic] ) )
        return false;

    bool positive = FloatUtils::isPositive( _costFunction[nonBasic] );

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

void Tableau::performPivot()
{
    // Any kind of pivot invalidates the assignment
    // TODO: we don't really need to invalidate, can update the basis
    // vars based on _d
    _basicAssignmentStatus = ASSIGNMENT_INVALID;

    if ( _leavingVariable == _m )
    {
        if ( _statistics )
            _statistics->incNumTableauBoundHopping();

        // printf( "\n\t\tTableau performing fake pivot. Varibale jumping to opposite bound: %u\n\n",
        //         _nonBasicIndexToVariable[_enteringVariable] );

        // The entering variable is going to be pressed against its bound.
        bool decrease = FloatUtils::isPositive( _costFunction[_enteringVariable] );
        unsigned nonBasic = _nonBasicIndexToVariable[_enteringVariable];
        setNonBasicAssignment( nonBasic, decrease ? _lowerBounds[nonBasic] : _upperBounds[nonBasic] );
        return;
    }

    if ( _statistics )
        _statistics->incNumTableauPivots();

    // Before pivoting, update gamma according to old basis
    if ( _usingSteepestEdge )
        updateGamma();

    // printf( "\n\t\tTableau performing pivot. Entering: %u, Leaving: %u\n\n",
    //         _nonBasicIndexToVariable[_enteringVariable],
    //         _basicIndexToVariable[_leavingVariable] );

    unsigned currentBasic = _basicIndexToVariable[_leavingVariable];
    unsigned currentNonBasic = _nonBasicIndexToVariable[_enteringVariable];

    // Update the database
    _basicVariables.insert( currentNonBasic );
    _basicVariables.erase( currentBasic );

    // Adjust the tableau indexing
    _basicIndexToVariable[_leavingVariable] = currentNonBasic;
    _nonBasicIndexToVariable[_enteringVariable] = currentBasic;
    _variableToIndex[currentBasic] = _enteringVariable;
    _variableToIndex[currentNonBasic] = _leavingVariable;

    // Update value of the old basic (now non-basic) variable
    double nonBasicAssignment;
    if ( _leavingVariableIncreases )
    {
        if ( _basicStatus[_leavingVariable] == Tableau::BELOW_LB )
            nonBasicAssignment = _lowerBounds[currentBasic];
        else
            nonBasicAssignment = _upperBounds[currentBasic];
    }
    else
    {
        if ( _basicStatus[_leavingVariable] == Tableau::ABOVE_UB )
            nonBasicAssignment = _upperBounds[currentBasic];
        else
            nonBasicAssignment = _lowerBounds[currentBasic];
    }

    // Check if the pivot is degenerate and update statistics
    if ( _statistics && ( nonBasicAssignment == _basicAssignment[currentBasic] ) )
        _statistics->incNumTableauDegeneratePivots();

    setNonBasicAssignment( _nonBasicIndexToVariable[_enteringVariable], nonBasicAssignment );

    // Update the basis factorization. The column corresponding to the
    // leaving variable is the one that has changed
    _basisFactorization->pushEtaMatrix( _leavingVariable, _d );
}

void Tableau::performDegeneratePivot( unsigned entering, unsigned leaving )
{
    if ( _statistics )
    {
        _statistics->incNumTableauDegeneratePivots();
        _statistics->incNumTableauDegeneratePivotsByRequest();
    }

    _enteringVariable = entering;
    _leavingVariable = leaving;

    ASSERT( entering < _n - _m );
    ASSERT( leaving < _m );
    ASSERT( !basicOutOfBounds( leaving ) );

    // Before pivoting, update gamma according to old basis
    if ( _usingSteepestEdge )
        updateGamma();

    // Compute d
    computeD();

    unsigned currentBasic = _basicIndexToVariable[leaving];
    unsigned currentNonBasic = _nonBasicIndexToVariable[entering];

    // Update the database
    _basicVariables.insert( currentNonBasic );
    _basicVariables.erase( currentBasic );

    // Adjust the tableau indexing
    _basicIndexToVariable[_leavingVariable] = currentNonBasic;
    _nonBasicIndexToVariable[_enteringVariable] = currentBasic;
    _variableToIndex[currentBasic] = _enteringVariable;
    _variableToIndex[currentNonBasic] = _leavingVariable;

    // Update the basis factorization
    _basisFactorization->pushEtaMatrix( leaving, _d );

    // Switch assignment values
    double temp = _basicAssignment[leaving];
    _basicAssignment[leaving] = _nonBasicAssignment[entering];
    setNonBasicAssignment( currentBasic, temp );
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
        }
        else if ( _basicStatus[basicIndex] == BasicStatus::AT_LB )
        {
            // Variable is pressed against a bound - can't change!
            maxChange = 0;
        }
        else
        {
            // Variable is below its lower bound, no constraint here
            maxChange = - DBL_MAX - _basicAssignment[basicIndex];
        }

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
        }
        else if ( _basicStatus[basicIndex] == BasicStatus::AT_UB )
        {
            // Variable is pressed against a bound - can't change!
            maxChange = 0;
        }
        else
        {
            // Variable is above its upper bound, no constraint here
            maxChange = DBL_MAX - _basicAssignment[basicIndex];
        }

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
    pickLeavingVariable( _d );
}

void Tableau::pickLeavingVariable( double *d )
{
    ASSERT( !FloatUtils::isZero( _costFunction[_enteringVariable] ) );
    bool decrease = FloatUtils::isPositive( _costFunction[_enteringVariable] );

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

    // TODO: assuming all coefficient in B are 1

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
            if ( !FloatUtils::isZero( d[i] ) )
            {
                double ratio = ratioConstraintPerBasic( i, d[i], decrease );
                if ( ratio > _changeRatio )
                {
                    _changeRatio = ratio;
                    _leavingVariable = i;
                }
            }
        }

        _leavingVariableIncreases = FloatUtils::isPositive( d[_enteringVariable] );
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
            if ( !FloatUtils::isZero( d[i] ) )
            {
                double ratio = ratioConstraintPerBasic( i, d[i], decrease );
                if ( ratio < _changeRatio )
                {
                    _changeRatio = ratio;
                    _leavingVariable = i;
                }
            }
        }

        _leavingVariableIncreases = FloatUtils::isNegative( d[_enteringVariable] );
    }
}

unsigned Tableau::getLeavingVariable() const
{
    if ( _leavingVariable == _m )
        return _nonBasicIndexToVariable[_enteringVariable];

    return _basicIndexToVariable[_leavingVariable];
}

double Tableau::getChangeRatio() const
{
    return _changeRatio;
}

void Tableau::computeD()
{
    // _a gets the entering variable's column in A
    _a = _A + ( _nonBasicIndexToVariable[_enteringVariable] * _m );

    // Compute d = inv(B) * a using the basis factorization
    _basisFactorization->forwardTransformation( _a, _d );

    // printf( "Leaving variable selection: dumping a\n\t" );
    // for ( unsigned i = 0; i < _m; ++i )
    //     printf( "%lf ", _a[i] );
    // printf( "\n" );

    // printf( "Leaving variable selection: dumping d\n\t" );
    // for ( unsigned i = 0; i < _m; ++i )
    //     printf( "%lf ", _d[i] );

    // printf( "\n" );
}

bool Tableau::isBasic( unsigned variable ) const
{
    return _basicVariables.exists( variable );
}

void Tableau::setNonBasicAssignment( unsigned variable, double value )
{
    ASSERT( !_basicVariables.exists( variable ) );

    unsigned nonBasic = _variableToIndex[variable];
    _nonBasicAssignment[nonBasic] = value;
    _basicAssignmentStatus = ASSIGNMENT_INVALID;

    // Inform watchers
    notifyVariableValue( variable, value );
}

void Tableau::dumpAssignment()
{
    printf( "Dumping assignment\n" );
    for ( unsigned i = 0; i < _n; ++i )
    {
        bool basic = _basicVariables.exists( i );
        printf( "\tx%u  -->  %.5lf [%s]. ", i, getValue( i ), basic ? "B" : "NB" );
        if ( _lowerBounds[i] != -DBL_MAX )
            printf( "Range: [ %.5lf, ", _lowerBounds[i] );
        else
            printf( "Range: [ -INFTY, " );

        if ( _upperBounds[i] != DBL_MAX )
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

    _basisFactorization->forwardTransformation( _b, _rhs );
    row->_rightHandSide = _rhs[index];
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
    ASSERT( _basicAssignmentStatus == ASSIGNMENT_VALID );

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

    // Store the indices
    memcpy( state._basicIndexToVariable, _basicIndexToVariable, sizeof(unsigned) * _m );
    memcpy( state._nonBasicIndexToVariable, _nonBasicIndexToVariable, sizeof(unsigned) * ( _n - _m ) );
    memcpy( state._variableToIndex, _variableToIndex, sizeof(unsigned) * _n );

    // Store the basis factorization
    _basisFactorization->storeFactorization( state._basisFactorization );

    // Store the steepest-edge gamma function
    memcpy( state._steepestEdgeGamma, _steepestEdgeGamma, sizeof(double) * ( _n - _m ) );
}

void Tableau::restoreState( const TableauState &state )
{
    freeMemoryIfNeeded();
    setDimensions( state._m, state._n );

    // Restore matrix A
    memcpy( _A, state._A, sizeof(double) * _n * _m );

    // Restore right hand side vector _b
    memcpy( _b, state._b, sizeof(double) * _m );

    // ReStore the bounds and valid status
    // TODO: I think valid status can just be set to true
    // TODO: should notify all the constraints.
    memcpy( _lowerBounds, state._lowerBounds, sizeof(double) *_n );
    memcpy( _upperBounds, state._upperBounds, sizeof(double) *_n );
    checkBoundsValid();

    // Basic variables
    _basicVariables = state._basicVariables;

    // Restore the assignments
    memcpy( _basicAssignment, state._basicAssignment, sizeof(double) *_m );
    memcpy( _nonBasicAssignment, state._nonBasicAssignment, sizeof(double) * ( _n - _m  ) );

    // Restore the indices
    memcpy( _basicIndexToVariable, state._basicIndexToVariable, sizeof(unsigned) * _m );
    memcpy( _nonBasicIndexToVariable, state._nonBasicIndexToVariable, sizeof(unsigned) * ( _n - _m ) );
    memcpy( _variableToIndex, state._variableToIndex, sizeof(unsigned) * _n );

    // Restore the basis factorization
    _basisFactorization->restoreFactorization( state._basisFactorization );

    // Restore the steepest-edge gamma function
    memcpy( _steepestEdgeGamma, state._steepestEdgeGamma, sizeof(double) * ( _n - _m ) );

    // After a restoration, the assignment is valid
    computeBasicStatus();
    _basicAssignmentStatus = ASSIGNMENT_VALID;
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
        throw ReluplexError( ReluplexError::INVALID_BOUND_TIGHTENING,
                             Stringf( "tightenLowerBound. Variable: %u. "
                                      "Old lower bound: %.2lf. "
                                      "New lower bound: %.2lf",
                                      variable, _lowerBounds[variable], value ).ascii() );
    setLowerBound( variable, value );

    // Ensure that non-basic variables are within bounds
    if ( !_basicVariables.exists( variable ) )
    {
        unsigned index = _variableToIndex[variable];
        if ( FloatUtils::gt( value, _nonBasicAssignment[index] ) )
            setNonBasicAssignment( variable, value );
    }
}

void Tableau::tightenUpperBound( unsigned variable, double value )
{
    ASSERT( variable < _n );

    if ( !FloatUtils::lt( value, _upperBounds[variable] ) )
        throw ReluplexError( ReluplexError::INVALID_BOUND_TIGHTENING,
                             Stringf( "tightenUpperBound. Variable: %u. "
                                      "Old upper bound: %.2lf. "
                                      "New upper bound: %.2lf",
                                      variable, _upperBounds[variable], value ).ascii() );
    setUpperBound( variable, value );

    // Ensure that non-basic variables are within bounds
    if ( !_basicVariables.exists( variable ) )
    {
        unsigned index = _variableToIndex[variable];
        if ( FloatUtils::lt( value, _nonBasicAssignment[index] ) )
            setNonBasicAssignment( variable, value );
    }
}

void Tableau::addEquation( const Equation &equation )
{
    // The aux variable in the equation has to be a new variable
    if ( equation._auxVariable != _n )
        throw ReluplexError( ReluplexError::INVALID_EQUATION_ADDED_TO_TABLEAU );

    // Prepare to update the basis factorization.
    // First condense the Etas so that we can access B0 explicitly
    _basisFactorization->condenseEtas();
    const double *oldB0 = _basisFactorization->getB0();

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

    // Mark the auxiliary variable as basic, add to indices
    _basicVariables.insert( equation._auxVariable );
    _basicIndexToVariable[_m - 1] = equation._auxVariable;
    _variableToIndex[equation._auxVariable] = _m - 1;

    // Populate the new row of A
    _b[_m - 1] = equation._scalar;
    for ( const auto &addend : equation._addends )
    {
        setEntryValue( _m - 1, addend._variable, addend._coefficient );

        // The new equation is given over the original non-basic variables.
        // However, some of them may have become basic in previous iterations.
        // Consequently, the last row of B0 may need to be adjusted.
        if ( _basicVariables.exists( addend._variable ) )
        {
            unsigned index = _variableToIndex[addend._variable];
            newB0[( newM - 1 ) * newM + index] = addend._coefficient;
        }
    }

    // Finally, give the extended B0 matrix to the basis factorization
    _basisFactorization->setB0( newB0 );

    delete[] newB0;
}

void Tableau::addRow()
{
    unsigned newM = _m + 1;
    unsigned newN = _n + 1;

    /*
      This function increases the sizes of the data structures used by
      the tableau to match newM and newN. Notice that newM = _m + 1 and
      newN = _n + 1, and so newN - newM = _m - _n. Consequently, structures
      that are of size _m - _n are left as is.
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

    // Allocate a new d. Don't need to initialize
    double *newD = new double[newM];
    if ( !newD )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::newD" );
    delete[] _d;
    _d = newD;

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

    // Allocate new basic costs. Don't need to initialize
    double *newBasicCosts = new double[newM];
    if ( !newBasicCosts )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::newBasicCosts" );
    delete[] _basicCosts;
    _basicCosts = newBasicCosts;

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

    // Allocate a new basic assignment vector, invalidate the assignment
    double *newBasicAssignment = new double[newM];
    if ( !newBasicAssignment )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::newAssignment" );
    _basicAssignmentStatus = ASSIGNMENT_INVALID;
    delete[] _basicAssignment;
    _basicAssignment = newBasicAssignment;

    unsigned *newBasicStatus = new unsigned[newM];
    if ( !newBasicStatus )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::newBasicStatus" );
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
    BasisFactorization *newBasisFactorization = new BasisFactorization( newM );
    if ( !newBasisFactorization )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::newBasisFactorization" );
    delete _basisFactorization;
    _basisFactorization = newBasisFactorization;

    // Steepest-edge data structures. Don't need to initialize.
    // TODO: Don't need to reallocate _steepestEdgeGamma, but may need to re-compute it!
    // Also don't need to resize _alpha and _nu.
    double *newWork = new double[newM];
    if ( !newWork )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::newWork" );
    delete[] _work;
    _work = newWork;

    // Allocate a larger _rhs. Don't need to initialize.
    double *newRhs = new double[newM];
    if ( !newRhs )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "Tableau::newRhs" );
    delete[] _rhs;
    _rhs = newRhs;

    _m = newM;
    _n = newN;
}

void Tableau::registerToWatchVariable( VariableWatcher *watcher, unsigned variable )
{
    _variableToWatchers[variable].append( watcher );
}

void Tableau::unregisterToWatchVariable( VariableWatcher *watcher, unsigned variable )
{
    _variableToWatchers[variable].erase( watcher );
}

void Tableau::notifyVariableValue( unsigned variable, double value )
{
    if ( _variableToWatchers.exists( variable ) )
    {
        for ( auto &watcher : _variableToWatchers[variable] )
            watcher->notifyVariableValue( variable, value );
    }
}

void Tableau::notifyLowerBound( unsigned variable, double bound )
{
    if ( _variableToWatchers.exists( variable ) )
    {
        for ( auto &watcher : _variableToWatchers[variable] )
            watcher->notifyLowerBound( variable, bound );
    }
}

void Tableau::notifyUpperBound( unsigned variable, double bound )
{
    if ( _variableToWatchers.exists( variable ) )
    {
        for ( auto &watcher : _variableToWatchers[variable] )
            watcher->notifyUpperBound( variable, bound );
    }
}

void Tableau::applySplit( const PiecewiseLinearCaseSplit &split )
{
    for ( const auto &equation : split.getEquations() )
        addEquation( equation );

    List<Tightening> bounds = split.getBoundTightenings();
    for ( const auto &bound : bounds )
    {
        if ( bound._type == Tightening::LB )
            tightenLowerBound( bound._variable, bound._value );
        else
            tightenUpperBound( bound._variable, bound._value );
    }
}

void Tableau::setStatistics( Statistics *statistics )
{
    _statistics = statistics;
}

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
