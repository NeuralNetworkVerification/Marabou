/*********************                                                        */
/*! \file Checker.cpp
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

#include "Checker.h"

Checker::Checker( const UnsatCertificateNode *root,
                  unsigned proofSize,
                  const SparseMatrix *initialTableau,
                  const Vector<double> &groundUpperBounds,
                  const Vector<double> &groundLowerBounds,
                  const List<PiecewiseLinearConstraint *> &problemConstraints )
    : _root( root )
    , _proofSize( proofSize )
    , _initialTableau( initialTableau )
    , _groundUpperBounds( groundUpperBounds )
    , _groundLowerBounds( groundLowerBounds )
    , _problemConstraints( problemConstraints )
    , _delegationCounter( 0 )
{
    for ( auto constraint : problemConstraints )
      constraint->setPhaseStatus( PHASE_NOT_FIXED );
}

bool Checker::check()
{
    return checkNode( _root );
}

bool Checker::checkNode( const UnsatCertificateNode *node )
{
    Vector<double> groundUpperBoundsBackup( _groundUpperBounds );
    Vector<double> groundLowerBoundsBackup( _groundLowerBounds );

    _upperBoundChanges.push( {} );
    _lowerBoundChanges.push( {} );

    // Update ground bounds according to head split
    for ( const auto &tightening : node->getSplit().getBoundTightenings() )
    {
        auto &temp = tightening._type == Tightening::UB ? _groundUpperBounds : _groundLowerBounds;
        temp[tightening._variable] = tightening._value;

        tightening._type == Tightening::UB ? _upperBoundChanges.top().insert( tightening._variable ) : _lowerBoundChanges.top().insert( tightening._variable );
    }

    // Check all PLC bound propagations
    if ( !checkAllPLCExplanations( node, GlobalConfiguration::LEMMA_CERTIFICATION_TOLERANCE ) )
        return false;

    // Save to file if marked
    if ( node->getDelegationStatus() == DelegationStatus::DELEGATE_SAVE )
        writeToFile();

    // Skip if leaf has the SAT solution, or if was marked to delegate
    if ( node->getSATSolutionFlag() || node->getDelegationStatus() != DelegationStatus::DONT_DELEGATE )
        return true;

    // Check if it is a leaf, and if so use contradiction to check
    // return true iff it is certified
    if ( node->isValidLeaf() )
        return checkContradiction( node );

    // If not a valid leaf, skip only if it is leaf that was not visited
    if ( !node->getVisited() && !node->getContradiction() && node->getChildren().empty() )
        return true;

    // Otherwise, should be a valid non-leaf node
    if ( !node->isValidNonLeaf() )
        return false;

    // If so, check all children and return true iff all children are certified
    // Also make sure that they are split correctly (i.e by ReLU constraint or by a single var)
    bool answer = true;
    List<PiecewiseLinearCaseSplit> childrenSplits;

    for ( const auto &child : node->getChildren() )
        childrenSplits.append( child->getSplit() );

    auto *childrenSplitConstraint = getCorrespondingReLUConstraint( childrenSplits );

    ASSERT( !childrenSplitConstraint || childrenSplitConstraint->getType() == RELU );

    if ( !checkSingleVarSplits( childrenSplits ) && !childrenSplitConstraint )
        return false;

    for ( const auto &child : node->getChildren() )
    {
        // Fix the phase of the constraint corresponding to the children
        if ( childrenSplitConstraint && childrenSplitConstraint->getType() == PiecewiseLinearFunctionType::RELU )
        {
            auto tightenings = child->getSplit().getBoundTightenings();
            if ( tightenings.front()._type == Tightening::LB || tightenings.back()._type == Tightening::LB  )
                childrenSplitConstraint->setPhaseStatus( RELU_PHASE_ACTIVE );
            else
                childrenSplitConstraint->setPhaseStatus( RELU_PHASE_INACTIVE );
        }

        if ( !checkNode( child ) )
            answer = false;
    }

    // Revert all changes
    if ( childrenSplitConstraint )
        childrenSplitConstraint->setPhaseStatus( PHASE_NOT_FIXED );

    // Revert only bounds that where changed during checking the current node
    for ( unsigned i : _upperBoundChanges.top() )
        _groundUpperBounds[i] = groundUpperBoundsBackup[i];

    for ( unsigned i : _lowerBoundChanges.top() )
        _groundLowerBounds[i] = groundLowerBoundsBackup[i];

    _upperBoundChanges.pop();
    _lowerBoundChanges.pop();

    return answer;
}

bool Checker::checkContradiction( const UnsatCertificateNode *node ) const
{
    ASSERT( node->isValidLeaf() && !node->getSATSolutionFlag() );
    const double *contradiction = node->getContradiction()->getContradiction();

    if ( contradiction == NULL )
    {
        unsigned infeasibleVar = node->getContradiction()->getVar();
        return FloatUtils::isNegative( _groundUpperBounds[infeasibleVar] - _groundLowerBounds[infeasibleVar] );
    }

    double contradictionUpperBound = UNSATCertificateUtils::computeCombinationUpperBound( contradiction, _initialTableau, _groundUpperBounds.data(), _groundLowerBounds.data(), _proofSize, _groundUpperBounds.size() );

    return FloatUtils::isNegative( contradictionUpperBound );
}

bool Checker::checkAllPLCExplanations( const UnsatCertificateNode *node, double epsilon )
{
    // Create copies of the gb, check for their validity, and pass these changes to all the children
    // Assuming the splits of the children are ok.
    // NOTE, this will change as PLCExplanation will
    for ( const auto &plcExplanation : node->getPLCExplanations() )
    {
        bool constraintMatched = false;
        bool tighteningMatched = false;
        auto explanationVector = Vector<double>( 0, 0 );
        List<unsigned> constraintVars;

        unsigned causingVar = plcExplanation->getCausingVar();
        unsigned affectedVar = plcExplanation->getAffectedVar();
        double bound = plcExplanation->getBound();
        const double *explanation = plcExplanation->getExplanation();
        BoundType causingVarBound = plcExplanation->getCausingVarBound();
        BoundType affectedVarBound = plcExplanation->getAffectedVarBound();
        PiecewiseLinearFunctionType constraintType = plcExplanation->getConstraintType();

        double explainedBound = UNSATCertificateUtils::computeBound( causingVar, causingVarBound == UPPER, explanation, _initialTableau, _groundUpperBounds.data(), _groundLowerBounds.data(), _proofSize, _groundUpperBounds.size() );
        unsigned b = 0;
        unsigned f = 0;
        unsigned aux = 0;

        // Make sure propagation was made by a problem constraint
        for ( const auto &constraint : _problemConstraints )
        {
            constraintVars = constraint->getParticipatingVariables();
            if ( constraintType == PiecewiseLinearFunctionType::RELU && constraintVars.exists( affectedVar ) && constraintVars.exists( causingVar ) )
            {
                Vector<unsigned> conVec( constraintVars.begin(), constraintVars.end() );
                b = conVec[0];
                f = conVec[1];
                aux = conVec[2];
                constraintMatched = true;

                // If explanation is phase fixing, mark it
                if ( ( affectedVarBound == LOWER && affectedVar == f && FloatUtils::isPositive( bound ) ) || ( affectedVarBound == UPPER && affectedVar == aux && FloatUtils::isZero( bound ) ) )
                    constraint->setPhaseStatus( RELU_PHASE_ACTIVE );
                else if ( ( affectedVarBound == LOWER && affectedVar == aux && FloatUtils::isPositive( bound ) ) || ( affectedVarBound == UPPER && affectedVar == f && FloatUtils::isZero( bound ) ) )
                    constraint->setPhaseStatus( RELU_PHASE_INACTIVE );
            }
        }

        if ( !constraintMatched )
            return false;

        if ( causingVar != b && causingVar != f && causingVar != aux )
            return false;

        // Make sure the explanation is explained using a ReLU bound tightening. Cases are matching each rule in ReluConstraint.cpp
        // We allow explained bound to be tighter than the ones recorded (since an explanation can explain tighter bounds), and an epsilon sized error is tolerated.

        // If lb of b is non negative, then ub of aux is 0
        if ( causingVar == b && causingVarBound == LOWER && affectedVar == aux && affectedVarBound == UPPER && FloatUtils::isZero( bound ) && !FloatUtils::isNegative( explainedBound + epsilon ) )
            tighteningMatched = true;

            // If lb of f is positive, then ub if aux is 0
        else if ( causingVar == f && causingVarBound == LOWER && affectedVar == aux && affectedVarBound == UPPER && FloatUtils::isZero( bound ) && FloatUtils::isPositive( explainedBound + epsilon ) )
            tighteningMatched = true;

            // If lb of b is positive x, then ub of aux is -x
        else if ( causingVar == b && causingVarBound == LOWER && affectedVar == aux &&affectedVarBound == UPPER && FloatUtils::gte(explainedBound, - bound - epsilon ) && bound > 0 )
            tighteningMatched = true;

            // If lb of aux is positive, then ub of f is 0
        else if ( causingVar == aux && causingVarBound ==LOWER && affectedVar == f && affectedVarBound == UPPER && FloatUtils::isZero( bound ) && FloatUtils::isPositive( explainedBound + epsilon ) )
            tighteningMatched = true;

            // If lb of f is negative, then it is 0
        else if ( causingVar == f && causingVarBound == LOWER && affectedVar == f && affectedVarBound == LOWER && FloatUtils::isZero( bound ) && FloatUtils::isNegative( explainedBound - epsilon ) )
            tighteningMatched = true;

            // Propagate ub from f to b
        else if ( causingVar == f && causingVarBound == UPPER && affectedVar == b && affectedVarBound == UPPER && FloatUtils::lte( explainedBound, bound + epsilon ) )
            tighteningMatched = true;

            // If ub of b is non positive, then ub of f is 0
        else if ( causingVar == b && causingVarBound == UPPER && affectedVar == f && affectedVarBound == UPPER && FloatUtils::isZero( bound ) && !FloatUtils::isPositive( explainedBound - epsilon ) )
            tighteningMatched = true;

            // If ub of b is non positive x, then lb of aux is -x
        else if ( causingVar == b && causingVarBound == UPPER && affectedVar == aux && affectedVarBound == LOWER && bound > 0 && !FloatUtils::isPositive( explainedBound - epsilon ) && FloatUtils::lte( explainedBound, -bound + epsilon) )
            tighteningMatched = true;

            // If ub of b is positive, then propagate to f ( positivity of explained bound is not checked since negative explained ub can always explain positive bound )
        else if ( causingVar == b && causingVarBound == UPPER && affectedVar == f && affectedVarBound == UPPER && FloatUtils::isPositive( bound ) && FloatUtils::lte( explainedBound,  bound + epsilon ) )
            tighteningMatched = true;

            // If ub of aux is x, then lb of b is -x
        else if ( causingVar == aux && causingVarBound == UPPER && affectedVar == b && affectedVarBound == LOWER && FloatUtils::lte( explainedBound, -bound + epsilon ) )
            tighteningMatched = true;

        if ( !tighteningMatched )
            return false;

        // If so, update the ground bounds and continue
        auto &temp = affectedVarBound == UPPER ? _groundUpperBounds : _groundLowerBounds;
        bool isTighter = affectedVarBound ? FloatUtils::lt( bound, temp[affectedVar] ) : FloatUtils::gt( bound, temp[affectedVar] );
        if ( isTighter )
        {
            temp[affectedVar] = bound;
            ASSERT( !_upperBoundChanges.empty() && !_lowerBoundChanges.empty() );
            affectedVarBound == UPPER ? _upperBoundChanges.top().insert( affectedVar ) : _lowerBoundChanges.top().insert( affectedVar );
        }
    }
    return true;
}

double Checker::explainBound( unsigned var, bool isUpper, const double *explanation ) const
{
    return UNSATCertificateUtils::computeBound( var, isUpper, explanation, _initialTableau, _groundUpperBounds.data(), _groundLowerBounds.data(), _proofSize, _groundUpperBounds.size() );
}

PiecewiseLinearConstraint *Checker::getCorrespondingReLUConstraint( const List<PiecewiseLinearCaseSplit> &splits )
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

    // Check that f = b + aux corresponds to a problem constraints
    PiecewiseLinearConstraint *correspondingConstraint = NULL;
    for ( auto &constraint : _problemConstraints )
    {
        auto constraintVars = constraint->getParticipatingVariables();
        if ( constraint->getType() == PiecewiseLinearFunctionType::RELU && constraintVars.front() == b && constraintVars.exists( f ) && ( activeSplit.size() == 1 || constraintVars.back() == aux ) )
            correspondingConstraint = constraint;
    }

    // Return the constraint for which f=relu(b)
    return correspondingConstraint;
}

void Checker::writeToFile()
{
    List<String> leafInstance;

    // Write with SmtLibWriter
    unsigned b, f;
    unsigned m = _proofSize;
    unsigned n = _groundUpperBounds.size();

    SmtLibWriter::addHeader( n, leafInstance );
    SmtLibWriter::addGroundUpperBounds( _groundUpperBounds, leafInstance );
    SmtLibWriter::addGroundLowerBounds( _groundLowerBounds, leafInstance );

    auto tableauRow = SparseUnsortedList();

    for ( unsigned i = 0; i < m; ++i )
    {
        _initialTableau->getRow( i, &tableauRow );
        SmtLibWriter::addTableauRow(  tableauRow , leafInstance );
    }

    for ( auto &constraint : _problemConstraints )
        if ( constraint->getType() == PiecewiseLinearFunctionType::RELU )
        {
            auto vars = constraint->getParticipatingVariables();
            b = vars.front();
            vars.popBack();
            f = vars.back();
            SmtLibWriter::addReLUConstraint( b, f, constraint->getPhaseStatus(), leafInstance );
        }

    SmtLibWriter::addFooter( leafInstance );
    File file ( "delegated" + std::to_string( _delegationCounter ) + ".smtlib" );
    SmtLibWriter::writeInstanceToFile( file, leafInstance );

    ++_delegationCounter;
}

bool Checker::checkSingleVarSplits( const List<PiecewiseLinearCaseSplit> &splits )
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
