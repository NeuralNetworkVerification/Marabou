/*********************                                                        */
/*! \file UnsatCertificateNode.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Omri Isac, Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2022 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]
 **/

#include <Options.h>
#include "UnsatCertificateNode.h"


UnsatCertificateNode::UnsatCertificateNode( Vector<Vector<double>> *initialTableau, Vector<double> &groundUpperBounds, Vector<double> &groundLowerBounds )
    : _parent( NULL )
    , _contradiction( NULL )
    , _hasSATSolution( false )
    , _wasVisited( false )
    , _delegationStatus( DelegationStatus::DONT_DELEGATE )
    , _delegationNumber( 0 )
    , _initialTableau( initialTableau )
    , _groundUpperBounds( groundUpperBounds )
    , _groundLowerBounds( groundLowerBounds )
{
}

UnsatCertificateNode::UnsatCertificateNode( UnsatCertificateNode *parent, PiecewiseLinearCaseSplit split )
    : _parent( parent )
    , _contradiction( NULL )
    , _headSplit( std::move( split ) )
    , _hasSATSolution( false )
    , _wasVisited( false )
    , _delegationStatus( DelegationStatus::DONT_DELEGATE )
    , _delegationNumber( 0 )
    , _initialTableau( NULL )
    , _groundUpperBounds( 0 )
    , _groundLowerBounds( 0 )
{
    parent->_children.append( this );
}

UnsatCertificateNode::~UnsatCertificateNode()
{
    for ( auto *child : _children )
    {
        if ( child )
        {
            delete child;
            child = NULL;
        }
    }

    _children.clear();
    deletePLCExplanations();

    if ( _contradiction )
    {
        delete _contradiction;
        _contradiction = NULL;
    }

    _parent = NULL;
}

void UnsatCertificateNode::setContradiction( Contradiction *contradiction )
{
    _contradiction = contradiction;
}

Contradiction *UnsatCertificateNode::getContradiction() const
{
    return _contradiction;
}

UnsatCertificateNode *UnsatCertificateNode::getParent() const
{
    return _parent;
}

const PiecewiseLinearCaseSplit &UnsatCertificateNode::getSplit() const
{
    return _headSplit;
}

const List<std::shared_ptr<PLCExplanation>> &UnsatCertificateNode::getPLCExplanations() const
{
    return _PLCExplanations;
}

void UnsatCertificateNode::makeLeaf()
{
    for ( UnsatCertificateNode *child : _children )
    {
        if ( child )
        {
            // Clear reference to root tableau so it will not be deleted
            child->_initialTableau = NULL;
            delete child;
            child = NULL;
        }
    }

    _children.clear();
}

void UnsatCertificateNode::passChangesToChildren( UnsatCertificateProblemConstraint *childrenSplitConstraint )
{
    for ( auto *child : _children )
    {
        child->copyGroundBounds( _groundUpperBounds, _groundLowerBounds );
        child->_initialTableau = _initialTableau;

        for ( auto &constraint : _problemConstraints )
        {
            if ( &constraint != childrenSplitConstraint )
                child->addProblemConstraint( constraint._type, constraint._constraintVars, constraint._status );
        }

        // Add the constraint corresponding to head split with correct phase
        if ( childrenSplitConstraint && childrenSplitConstraint->_type == PiecewiseLinearFunctionType::RELU )
        {
            if ( child->_headSplit.getBoundTightenings().front()._type == Tightening::LB || child->_headSplit.getBoundTightenings().back()._type == Tightening::LB  )
                child->addProblemConstraint( childrenSplitConstraint->_type, childrenSplitConstraint->_constraintVars, PhaseStatus::RELU_PHASE_ACTIVE );
            else
                child->addProblemConstraint( childrenSplitConstraint->_type, childrenSplitConstraint->_constraintVars, PhaseStatus::RELU_PHASE_INACTIVE );
        }
    }
}

bool UnsatCertificateNode::certify()
{
    // Update ground bounds according to head split
    for ( auto &tightening : _headSplit.getBoundTightenings() )
    {
        auto &temp = tightening._type == Tightening::UB ? _groundUpperBounds : _groundLowerBounds;
        temp[tightening._variable] = tightening._value;
    }

    // Certify all PLC bound propagations
    if ( !certifyAllPLCExplanations( UNSATCertificateUtils::CERTIFICATION_TOLERANCE ) )
        return false;

    // Save to file if marked
    if ( _delegationStatus == DelegationStatus::DELEGATE_SAVE )
        writeLeafToFile();

    // Skip if laf has the SAT solution, or if was marked to delegate
    if ( _hasSATSolution || _delegationStatus != DelegationStatus::DONT_DELEGATE )
        return true;

    // Check if it is a leaf, and if so use contradiction to certify
    // return true iff it is certified
    if ( isValidLeaf() )
        return certifyContradiction();

    // If not a valid leaf, skip only if it is leaf that was not visited
    if ( !_wasVisited && !_contradiction && _children.empty() )
        return true;

    // Otherwise, should be a valid non-leaf node
    if ( !isValidNonLeaf() )
        return false;

    // If so, certify all children and return true iff all children are certified
    // Also make sure that they are split correctly (i.e by ReLU constraint or by a single var)
    bool answer = true;
    List<PiecewiseLinearCaseSplit> childrenSplits;

    for ( auto &child : _children )
        childrenSplits.append( child->_headSplit );

    auto *childrenSplitConstraint = getCorrespondingReLUConstraint( childrenSplits );
    if ( !certifySingleVarSplits( childrenSplits ) && !childrenSplitConstraint )
        return false;

    passChangesToChildren( childrenSplitConstraint );

    for ( auto child : _children )
        if ( !child->certify() )
            answer = false;

    // After all subtree is checked, no use in these vectors.
    _groundUpperBounds.clear();
    _groundLowerBounds.clear();
    return answer;
}

bool UnsatCertificateNode::certifyContradiction()
{
    ASSERT( isValidLeaf() && !_hasSATSolution );
    unsigned var = _contradiction->_var;
    unsigned length = _initialTableau->size();

    auto upperBoundExplanation = Vector<double>( 0, 0 );
    auto lowerBoundExplanation = Vector<double>( 0, 0 );

    if ( _contradiction->_upperBoundExplanation )
    {
        upperBoundExplanation = Vector<double>( length, 0 );
        std::copy( _contradiction->_upperBoundExplanation, _contradiction->_upperBoundExplanation + length, upperBoundExplanation.begin() );
    }

    if ( _contradiction->_lowerBoundExplanation )
    {
        lowerBoundExplanation = Vector<double>( length, 0 );
        std::copy( _contradiction->_lowerBoundExplanation, _contradiction->_lowerBoundExplanation + length, lowerBoundExplanation.begin() );
    }

    double computedUpper = explainBound( var, true, upperBoundExplanation );
    double computedLower = explainBound( var, false, lowerBoundExplanation );

    return computedUpper < computedLower;
}

double UnsatCertificateNode::explainBound( unsigned var, bool isUpper, Vector<double> &explanation )
{
    return UNSATCertificateUtils::computeBound( var, isUpper, explanation, *_initialTableau, _groundUpperBounds, _groundLowerBounds );
}

void UnsatCertificateNode::copyGroundBounds( Vector<double> &groundUpperBounds, Vector<double> &groundLowerBounds )
{
    _groundUpperBounds = Vector<double>( groundUpperBounds );
    _groundLowerBounds = Vector<double>( groundLowerBounds );
}

bool UnsatCertificateNode::isValidLeaf() const
{
    return _contradiction && _children.empty();
}

bool UnsatCertificateNode::isValidNonLeaf() const
{
    return !_contradiction && !_children.empty();
}

void UnsatCertificateNode::addPLCExplanation( std::shared_ptr<PLCExplanation> &explanation )
{
    _PLCExplanations.append( explanation );
}

void UnsatCertificateNode::addProblemConstraint( PiecewiseLinearFunctionType type, List<unsigned> constraintVars, PhaseStatus status )
{
    _problemConstraints.append( { type, constraintVars, status } );
}

UnsatCertificateProblemConstraint *UnsatCertificateNode::getCorrespondingReLUConstraint( const List<PiecewiseLinearCaseSplit> &splits )
{
    if ( splits.size() != 2 )
        return NULL;

    auto firstSplitTightenings = splits.front().getBoundTightenings();
    auto secondSplitTightenings = splits.back().getBoundTightenings();

    // Find the LB tightening, its var is b
    auto &activeSplit = firstSplitTightenings.front()._type == Tightening::LB ? firstSplitTightenings : secondSplitTightenings;
    auto &inactiveSplit = firstSplitTightenings.front()._type == Tightening::LB ? secondSplitTightenings : firstSplitTightenings;

    unsigned b = activeSplit.front()._variable;
    unsigned aux = activeSplit.back()._variable;
    unsigned f = inactiveSplit.back()._variable;

    // Aux var may or may not be used
    if ( ( activeSplit.size() != 2 && activeSplit.size() != 1 ) || inactiveSplit.size() != 2 )
        return NULL;

    if ( FloatUtils::areDisequal( inactiveSplit.back()._value, 0.0 ) || FloatUtils::areDisequal( inactiveSplit.front()._value, 0.0 ) || FloatUtils::areDisequal( activeSplit.back()._value, 0.0 ) || FloatUtils::areDisequal( activeSplit.front()._value, 0.0 ) )
        return NULL;

    // Certify that f = b + aux corresponds to a problem constraints
    UnsatCertificateProblemConstraint *correspondingConstraint = NULL;
    for ( UnsatCertificateProblemConstraint &constraint : _problemConstraints )
    {
        if ( constraint._type == PiecewiseLinearFunctionType::RELU && constraint._constraintVars.front() == b && constraint._constraintVars.exists( f ) && ( activeSplit.size() == 1 || constraint._constraintVars.back() == aux ) )
            correspondingConstraint = &constraint;
    }

    // Return the constraint for which f=relu(b)
    return correspondingConstraint;
}

bool UnsatCertificateNode::certifyAllPLCExplanations( double epsilon )
{
    // Create copies of the gb, check for their validity, and pass these changes to all the children
    // Assuming the splits of the children are ok.
    // NOTE, this will change as PLCExplanation will
    for ( const auto &explanation : _PLCExplanations )
    {
        bool constraintMatched = false;
        bool tighteningMatched = false;
        unsigned length = _initialTableau->size();
        auto explanationVector = Vector<double>( 0, 0 );

        if ( explanation->_explanation )
        {
            explanationVector = Vector<double>( length, 0 );
            std::copy( explanation->_explanation, explanation->_explanation + length, explanationVector.begin() );
        }

        double explainedBound = UNSATCertificateUtils::computeBound( explanation->_causingVar, explanation->_causingVarBound == UPPER, explanationVector, *_initialTableau, _groundUpperBounds, _groundLowerBounds );
        unsigned b = 0;
        unsigned f = 0;
        unsigned aux = 0;

        // Make sure propagation was by a problem constraint
        for ( UnsatCertificateProblemConstraint &constraint : _problemConstraints )
        {
            if ( explanation->_constraintType == PiecewiseLinearFunctionType::RELU && constraint._constraintVars.exists(explanation->_affectedVar ) && constraint._constraintVars.exists(explanation->_causingVar ) )
            {
                Vector<unsigned> conVec( constraint._constraintVars.begin(), constraint._constraintVars.end() );
                b = conVec[0];
                f = conVec[1];
                aux = conVec[2];
                constraintMatched = true;

                // If explanation is phase fixing, mark it
                if ( ( explanation->_affectedVarBound == LOWER && explanation->_affectedVar == f && FloatUtils::isPositive(explanation->_bound ) ) || ( explanation->_affectedVarBound == UPPER && explanation->_affectedVar == aux && FloatUtils::isZero(explanation->_bound ) ) )
                    constraint._status = PhaseStatus::RELU_PHASE_ACTIVE;
                else if ( ( explanation->_affectedVarBound == LOWER && explanation->_affectedVar == aux && FloatUtils::isPositive(explanation->_bound ) ) || ( explanation->_affectedVarBound == UPPER && explanation->_affectedVar == f && FloatUtils::isZero(explanation->_bound ) ) )
                    constraint._status = PhaseStatus::RELU_PHASE_INACTIVE;
            }
        }

        if ( !constraintMatched )
            return false;

        if ( explanation->_causingVar != b && explanation->_causingVar != f && explanation->_causingVar != aux )
            return false;

        // Make sure the explanation is explained using a ReLU bound tightening. Cases are matching each rule in ReluConstraint.cpp
        // We allow explained bound to be tighter than the ones recorded (since an explanation can explain tighter bounds), and an epsilon sized error is tolerated.

        // If lb of b is non negative, then ub of aux is 0
        if ( explanation->_causingVar == b && explanation->_causingVarBound == LOWER && explanation->_affectedVar == aux && explanation->_affectedVarBound == UPPER && FloatUtils::isZero(explanation->_bound ) && !FloatUtils::isNegative(explainedBound + epsilon ) )
            tighteningMatched = true;

        // If lb of f is positive, then ub if aux is 0
        else if ( explanation->_causingVar == f && explanation->_causingVarBound == LOWER && explanation->_affectedVar == aux && explanation->_affectedVarBound == UPPER && FloatUtils::isZero(explanation->_bound ) && FloatUtils::isPositive(explainedBound + epsilon ) )
            tighteningMatched = true;

        // If lb of b is positive x, then lb of aux is -x
        else if ( explanation->_causingVar == b && explanation->_causingVarBound == LOWER && explanation->_affectedVar == aux && explanation->_affectedVarBound == UPPER && FloatUtils::gte(explainedBound, - explanation->_bound - epsilon ) && explanation->_bound > 0 )
            tighteningMatched = true;

        // If lb of aux is positive, then ub of f is 0
        else if ( explanation->_causingVar == aux && explanation->_causingVarBound ==LOWER && explanation->_affectedVar == f && explanation->_affectedVarBound == UPPER && FloatUtils::isZero(explanation->_bound ) && FloatUtils::isPositive(explainedBound + epsilon ) )
            tighteningMatched = true;

        // If lb of f is negative, then it is 0
        else if ( explanation->_causingVar == f && explanation->_causingVarBound == LOWER && explanation->_affectedVar == f && explanation->_affectedVarBound == LOWER && FloatUtils::isZero(explanation->_bound ) && FloatUtils::isNegative(explainedBound - epsilon ) )
            tighteningMatched = true;

        // Propagate ub from f to b
        else if ( explanation->_causingVar == f && explanation->_causingVarBound == UPPER && explanation->_affectedVar == b && explanation->_affectedVarBound == UPPER && FloatUtils::lte(explainedBound, explanation->_bound + epsilon ) )
            tighteningMatched = true;

        // If ub of b is non positive, then ub of f is 0
        else if ( explanation->_causingVar == b && explanation->_causingVarBound == UPPER && explanation->_affectedVar == f && explanation->_affectedVarBound == UPPER && FloatUtils::isZero(explanation->_bound ) && !FloatUtils::isPositive(explainedBound - epsilon ) )
            tighteningMatched = true;

        // If ub of b is non positive x, then lb of aux is -x
        else if ( explanation->_causingVar == b && explanation->_causingVarBound == UPPER && explanation->_affectedVar == aux && explanation->_affectedVarBound == LOWER && explanation->_bound > 0 && !FloatUtils::isPositive(explainedBound - epsilon ) && FloatUtils::lte(explainedBound, -explanation->_bound + epsilon) )
            tighteningMatched = true;

        // If ub of b is positive, then propagate to f ( positivity of explained bound is not checked since negative explained ub can always explain positive bound )
        else if ( explanation->_causingVar == b && explanation->_causingVarBound == UPPER && explanation->_affectedVar == f && explanation->_affectedVarBound == UPPER && FloatUtils::isPositive(explanation->_bound ) && FloatUtils::lte(explainedBound, explanation->_bound + epsilon ) )
            tighteningMatched = true;

        // If ub of aux is x, then lb of b is -x
        else if ( explanation->_causingVar == aux && explanation->_causingVarBound == UPPER && explanation->_affectedVar == b && explanation->_affectedVarBound == LOWER && FloatUtils::lte(explainedBound, -explanation->_bound + epsilon ) )
            tighteningMatched = true;

        if ( !tighteningMatched )
            return false;

        // If so, update the ground bounds and continue
        Vector<double> &temp = explanation->_affectedVarBound ? _groundUpperBounds : _groundLowerBounds;
        bool isTighter = explanation->_affectedVarBound ? FloatUtils::lt( explanation->_bound, temp[explanation->_affectedVar] ) : FloatUtils::gt( explanation->_bound, temp[explanation->_affectedVar] );
        if ( isTighter )
            temp[explanation->_affectedVar] = explanation->_bound;
    }
    return true;
}

/*
 * Get a pointer to a child by a head split, or NULL if not found
 */
UnsatCertificateNode *UnsatCertificateNode::getChildBySplit( const PiecewiseLinearCaseSplit &split ) const
{
    for ( UnsatCertificateNode *child : _children )
    {
        if ( child->_headSplit == split )
            return child;
    }

    return NULL;
}

void UnsatCertificateNode::setSATSolution()
{
    _hasSATSolution = true;
}

void UnsatCertificateNode::setVisited()
{
    _wasVisited = true;
}

void UnsatCertificateNode::shouldDelegate( unsigned delegationNumber, DelegationStatus delegationStatus )
{
    ASSERT( delegationStatus != DelegationStatus::DONT_DELEGATE );
    _delegationStatus = delegationStatus;
    _delegationNumber = delegationNumber;
}

bool UnsatCertificateNode::certifySingleVarSplits( const List<PiecewiseLinearCaseSplit> &splits ) const
{
    if ( splits.size() != 2 )
        return false;

    // These are singletons to tightenings
    auto &frontSplitTightenings = splits.front().getBoundTightenings();
    auto &backSplitTightenings = splits.back().getBoundTightenings();

    if ( frontSplitTightenings.size() != 1 || backSplitTightenings.size() != 1 )
        return false;

    // These are the elements in the singletons
    auto &frontSplitOnlyTightening = frontSplitTightenings.front();
    auto &backSplitOnlyTightening = backSplitTightenings.front();

    // Check that cases are of the same var and bound, where the for one the bound is UB, and for the other is LB
    if ( frontSplitOnlyTightening._variable != backSplitOnlyTightening._variable )
        return false;

    if ( FloatUtils::areDisequal( frontSplitOnlyTightening._value, backSplitOnlyTightening._value ) )
        return false;

    if ( frontSplitOnlyTightening._type == backSplitOnlyTightening._type )
        return false;

    return true;
}

void UnsatCertificateNode::deletePLCExplanations()
{
    _PLCExplanations.clear();
}

/*
 * Removes all PLC explanations from a certain point
 */
void UnsatCertificateNode::setPLCExplanations( const List<std::shared_ptr<PLCExplanation>> &explanations )
{
    _PLCExplanations.clear();
    _PLCExplanations = explanations;
}

void UnsatCertificateNode::writeLeafToFile()
{
    ASSERT( _children.empty() && _delegationStatus == DelegationStatus::DELEGATE_SAVE );
    List<String> leafInstance;

    // Write with SmtLibWriter
    unsigned b, f;
    unsigned m = _initialTableau->size();
    unsigned n = _groundUpperBounds.size();

    SmtLibWriter::addHeader( n, leafInstance );
    SmtLibWriter::addGroundUpperBounds( _groundUpperBounds, leafInstance );
    SmtLibWriter::addGroundLowerBounds( _groundLowerBounds, leafInstance );

    for ( unsigned i = 0; i < m; ++i )
        SmtLibWriter::addTableauRow( ( *_initialTableau )[i], leafInstance );

    for ( auto &constraint : _problemConstraints )
        if ( constraint._type == PiecewiseLinearFunctionType::RELU )
        {
            auto vars = constraint._constraintVars;
            b = vars.front();
            vars.popBack();
            f = vars.back();
            SmtLibWriter::addReLUConstraint( b, f, constraint._status, leafInstance );
        }

    SmtLibWriter::addFooter( leafInstance );
    File file ( "delegated" + std::to_string( _delegationNumber ) + ".smtlib" );
    SmtLibWriter::writeInstanceToFile( file, leafInstance );
}

void UnsatCertificateNode::removePLCExplanationsBelowDecisionLevel( unsigned decisionLevel )
{
    _PLCExplanations.removeIf( [decisionLevel] ( std::shared_ptr<PLCExplanation> &explanation ){ return explanation->_decisionLevel <= decisionLevel; } );
}