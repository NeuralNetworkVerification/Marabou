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

        tightening._type == Tightening::UB
            ? _upperBoundChanges.top().insert( tightening._variable )
            : _lowerBoundChanges.top().insert( tightening._variable );
    }

    // Check all PLC bound propagations
    if ( !checkAllPLCExplanations( node, GlobalConfiguration::LEMMA_CERTIFICATION_TOLERANCE ) )
        return false;

    // Save to file if marked
    if ( node->getDelegationStatus() == DelegationStatus::DELEGATE_SAVE )
        writeToFile();

    // Skip if leaf has the SAT solution, or if was marked to delegate
    if ( node->getSATSolutionFlag() ||
         node->getDelegationStatus() != DelegationStatus::DONT_DELEGATE )
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

    PiecewiseLinearConstraint *childrenSplitConstraint =
        getCorrespondingConstraint( childrenSplits );

    if ( !checkSingleVarSplits( childrenSplits ) && !childrenSplitConstraint )
        return false;

    // Fix the constraints phase according to the child, and check each child
    for ( const auto &child : node->getChildren() )
    {
        fixChildSplitPhase( child, childrenSplitConstraint );
        if ( !checkNode( child ) )
            answer = false;
    }

    // Revert all changes
    if ( childrenSplitConstraint )
        childrenSplitConstraint->setPhaseStatus( PHASE_NOT_FIXED );

    if ( childrenSplitConstraint && childrenSplitConstraint->getType() == DISJUNCTION )
    {
        for ( const auto &child : node->getChildren() )
            ( (DisjunctionConstraint *)childrenSplitConstraint )
                ->addFeasibleDisjunct( child->getSplit() );
    }

    // Revert only bounds that where changed during checking the current node
    for ( const auto &i : _upperBoundChanges.top() )
        _groundUpperBounds[i] = groundUpperBoundsBackup[i];

    for ( const auto &i : _lowerBoundChanges.top() )
        _groundLowerBounds[i] = groundLowerBoundsBackup[i];

    _upperBoundChanges.pop();
    _lowerBoundChanges.pop();

    return answer;
}

void Checker::fixChildSplitPhase( UnsatCertificateNode *child,
                                  PiecewiseLinearConstraint *childrenSplitConstraint )
{
    if ( childrenSplitConstraint && childrenSplitConstraint->getType() == RELU )
    {
        List<Tightening> tightenings = child->getSplit().getBoundTightenings();
        if ( tightenings.front()._type == Tightening::LB ||
             tightenings.back()._type == Tightening::LB )
            childrenSplitConstraint->setPhaseStatus( RELU_PHASE_ACTIVE );
        else
            childrenSplitConstraint->setPhaseStatus( RELU_PHASE_INACTIVE );
    }
    else if ( childrenSplitConstraint && childrenSplitConstraint->getType() == SIGN )
    {
        List<Tightening> tightenings = child->getSplit().getBoundTightenings();
        if ( tightenings.front()._type == Tightening::LB )
            childrenSplitConstraint->setPhaseStatus( SIGN_PHASE_POSITIVE );
        else
            childrenSplitConstraint->setPhaseStatus( SIGN_PHASE_NEGATIVE );
    }
    else if ( childrenSplitConstraint && childrenSplitConstraint->getType() == ABSOLUTE_VALUE )
    {
        List<Tightening> tightenings = child->getSplit().getBoundTightenings();
        if ( tightenings.front()._type == Tightening::LB )
            childrenSplitConstraint->setPhaseStatus( ABS_PHASE_POSITIVE );
        else
            childrenSplitConstraint->setPhaseStatus( ABS_PHASE_NEGATIVE );
    }
    else if ( childrenSplitConstraint && childrenSplitConstraint->getType() == MAX )
    {
        List<Tightening> tightenings = child->getSplit().getBoundTightenings();
        if ( tightenings.size() == 2 )
            childrenSplitConstraint->setPhaseStatus( MAX_PHASE_ELIMINATED );
        else
        {
            PhaseStatus phase = ( (MaxConstraint *)childrenSplitConstraint )
                                    ->variableToPhase( tightenings.back()._variable );
            childrenSplitConstraint->setPhaseStatus( phase );
        }
    }
    else if ( childrenSplitConstraint && childrenSplitConstraint->getType() == DISJUNCTION )
        ( (DisjunctionConstraint *)childrenSplitConstraint )
            ->removeFeasibleDisjunct( child->getSplit() );
    else if ( childrenSplitConstraint && childrenSplitConstraint->getType() == LEAKY_RELU )
    {
        List<Tightening> tightenings = child->getSplit().getBoundTightenings();
        if ( tightenings.front()._type == Tightening::LB &&
             tightenings.back()._type == Tightening::LB )
            childrenSplitConstraint->setPhaseStatus( RELU_PHASE_ACTIVE );
        else
            childrenSplitConstraint->setPhaseStatus( RELU_PHASE_INACTIVE );
    }
}

bool Checker::checkContradiction( const UnsatCertificateNode *node ) const
{
    ASSERT( node->isValidLeaf() && !node->getSATSolutionFlag() );
    const SparseUnsortedList &contradiction = node->getContradiction()->getContradiction();

    if ( contradiction.empty() )
    {
        unsigned infeasibleVar = node->getContradiction()->getVar();
        return FloatUtils::isNegative( _groundUpperBounds[infeasibleVar] -
                                       _groundLowerBounds[infeasibleVar] );
    }

    double contradictionUpperBound =
        UNSATCertificateUtils::computeCombinationUpperBound( contradiction,
                                                             _initialTableau,
                                                             _groundUpperBounds.data(),
                                                             _groundLowerBounds.data(),
                                                             _groundUpperBounds.size() );

    return FloatUtils::isNegative( contradictionUpperBound );
}

bool Checker::checkAllPLCExplanations( const UnsatCertificateNode *node, double epsilon )
{
    // Create copies of the gb, check for their validity, and pass these changes to all the children
    // Assuming the splits of the children are ok.
    // NOTE, this will change as PLCLemma will
    for ( const auto &plcLemma : node->getPLCLemmas() )
    {
        PiecewiseLinearConstraint *matchedConstraint = NULL;
        Tightening::BoundType affectedVarBound = plcLemma->getAffectedVarBound();
        double explainedBound = affectedVarBound == Tightening::UB ? FloatUtils::infinity()
                                                                   : FloatUtils::negativeInfinity();
        const List<unsigned> causingVars = plcLemma->getCausingVars();
        unsigned affectedVar = plcLemma->getAffectedVar();
        List<unsigned> participatingVars;

        // Make sure propagation was made by a problem constraint
        for ( const auto &constraint : _problemConstraints )
        {
            if ( constraint->getType() != plcLemma->getConstraintType() )
                continue;

            participatingVars = constraint->getParticipatingVariables();

            // If the constraint is max, then lemma variables might have been eliminated
            if ( constraint->getType() == MAX )
            {
                MaxConstraint *maxConstraint = (MaxConstraint *)constraint;
                for ( auto const &element : maxConstraint->getEliminatedElements() )
                {
                    participatingVars.append( element );
                    participatingVars.append( maxConstraint->getAuxToElement( element ) );
                }
            }

            if ( participatingVars.exists( affectedVar ) )
                matchedConstraint = constraint;
        }

        if ( !matchedConstraint )
            return false;

        PiecewiseLinearFunctionType constraintType = matchedConstraint->getType();

        if ( constraintType == RELU )
            explainedBound = checkReluLemma( *plcLemma, *matchedConstraint, epsilon );
        else if ( constraintType == SIGN )
            explainedBound = checkSignLemma( *plcLemma, *matchedConstraint, epsilon );
        else if ( constraintType == ABSOLUTE_VALUE )
            explainedBound = checkAbsLemma( *plcLemma, *matchedConstraint, epsilon );
        else if ( constraintType == MAX )
            explainedBound = checkMaxLemma( *plcLemma, *matchedConstraint );
        else if ( constraintType == LEAKY_RELU )
            explainedBound = checkLeakyReluLemma( *plcLemma, *matchedConstraint, epsilon );
        else
            return false;

        // If so, update the ground bounds and continue

        auto &temp = affectedVarBound == Tightening::UB ? _groundUpperBounds : _groundLowerBounds;
        bool isTighter = affectedVarBound == Tightening::UB
                           ? FloatUtils::lt( explainedBound, temp[affectedVar] )
                           : FloatUtils::gt( explainedBound, temp[affectedVar] );
        if ( isTighter )
        {
            temp[affectedVar] = explainedBound;
            ASSERT( !_upperBoundChanges.empty() && !_lowerBoundChanges.empty() );
            affectedVarBound == Tightening::UB ? _upperBoundChanges.top().insert( affectedVar )
                                               : _lowerBoundChanges.top().insert( affectedVar );
        }
    }
    return true;
}

double
Checker::explainBound( unsigned var, bool isUpper, const SparseUnsortedList &explanation ) const
{
    return UNSATCertificateUtils::computeBound( var,
                                                isUpper,
                                                explanation,
                                                _initialTableau,
                                                _groundUpperBounds.data(),
                                                _groundLowerBounds.data(),
                                                _groundUpperBounds.size() );
}

PiecewiseLinearConstraint *
Checker::getCorrespondingConstraint( const List<PiecewiseLinearCaseSplit> &splits )
{
    PiecewiseLinearConstraint *constraint = getCorrespondingReluConstraint( splits );
    if ( !constraint )
        constraint = getCorrespondingSignConstraint( splits );
    if ( !constraint )
        constraint = getCorrespondingAbsConstraint( splits );
    if ( !constraint )
        constraint = getCorrespondingMaxConstraint( splits );
    if ( !constraint )
        constraint = getCorrespondingDisjunctionConstraint( splits );
    if ( !constraint )
        constraint = getCorrespondingLeakyReluConstraint( splits );

    return constraint;
}

void Checker::writeToFile()
{
    String filename = "delegated" + std::to_string( _delegationCounter ) + ".smtlib";

    SmtLibWriter::writeToSmtLibFile( filename,
                                     _proofSize,
                                     _groundUpperBounds.size(),
                                     _groundUpperBounds,
                                     _groundLowerBounds,
                                     _initialTableau,
                                     List<Equation>(),
                                     _problemConstraints );

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

    // Check that cases are of the same var and bound, where the for one the bound is UB, and for
    // the other is LB
    if ( frontSplitOnlyTightening._variable != backSplitOnlyTightening._variable )
        return false;

    if ( FloatUtils::areDisequal( frontSplitOnlyTightening._value,
                                  backSplitOnlyTightening._value ) )
        return false;

    if ( frontSplitOnlyTightening._type == backSplitOnlyTightening._type )
        return false;

    return true;
}

PiecewiseLinearConstraint *
Checker::getCorrespondingReluConstraint( const List<PiecewiseLinearCaseSplit> &splits )
{
    if ( splits.size() != 2 )
        return NULL;

    auto firstSplitTightenings = splits.front().getBoundTightenings();
    auto secondSplitTightenings = splits.back().getBoundTightenings();

    // Find the LB tightening, its var is b
    auto &activeSplit = firstSplitTightenings.front()._type == Tightening::LB
                          ? firstSplitTightenings
                          : secondSplitTightenings;
    auto &inactiveSplit = firstSplitTightenings.front()._type == Tightening::LB
                            ? secondSplitTightenings
                            : firstSplitTightenings;

    // Aux var may or may not be used
    if ( ( activeSplit.size() != 2 && activeSplit.size() != 1 ) || inactiveSplit.size() != 2 )
        return NULL;

    // Check that f = b + aux corresponds to a problem constraints
    for ( auto &constraint : _problemConstraints )
    {
        if ( constraint->getType() == RELU )
        {
            auto constraintVars = constraint->getParticipatingVariables();
            unsigned b = constraintVars.front();
            unsigned aux = constraintVars.back();
            constraintVars.popBack();
            unsigned f = constraintVars.back();
            if ( inactiveSplit.exists( Tightening( b, 0.0, Tightening::UB ) ) &&
                 inactiveSplit.exists( Tightening( f, 0.0, Tightening::UB ) ) &&
                 activeSplit.exists( Tightening( b, 0.0, Tightening::LB ) ) &&
                 ( activeSplit.size() == 1 ||
                   activeSplit.exists( Tightening( aux, 0.0, Tightening::UB ) ) ) )
                // Return the constraint for which f=relu(b)
                return constraint;
        }
    }

    return NULL;
}

PiecewiseLinearConstraint *
Checker::getCorrespondingSignConstraint( const List<PiecewiseLinearCaseSplit> &splits )
{
    if ( splits.size() != 2 )
        return NULL;

    auto firstSplitTightenings = splits.front().getBoundTightenings();
    auto secondSplitTightenings = splits.back().getBoundTightenings();

    // Find an LB tightening, it marks the positive split
    auto &positiveSplit = firstSplitTightenings.front()._type == Tightening::LB
                            ? firstSplitTightenings
                            : secondSplitTightenings;
    auto &negativeSplit = firstSplitTightenings.front()._type == Tightening::LB
                            ? secondSplitTightenings
                            : firstSplitTightenings;

    unsigned b = positiveSplit.back()._variable;
    unsigned f = positiveSplit.front()._variable;

    // Check details of both constraints - value and types
    if ( positiveSplit.size() != 2 || negativeSplit.size() != 2 ||
         positiveSplit.back()._type != Tightening::LB ||
         positiveSplit.front()._type != Tightening::LB ||
         negativeSplit.back()._type != Tightening::UB ||
         negativeSplit.front()._type != Tightening::UB )
        return NULL;

    if ( FloatUtils::areDisequal( negativeSplit.back()._value, -1.0 ) ||
         FloatUtils::areDisequal( negativeSplit.front()._value, 0.0 ) ||
         FloatUtils::areDisequal( positiveSplit.back()._value, 1.0 ) ||
         FloatUtils::areDisequal( positiveSplit.front()._value, 0.0 ) )
        return NULL;

    // Check that f=sign(b) corresponds to a problem constraints
    for ( auto &constraint : _problemConstraints )
    {
        auto constraintVars = constraint->getParticipatingVariables();
        if ( constraint->getType() == SIGN && constraintVars.back() == b &&
             constraintVars.front() == f )
            // Return the constraint for which f=sign(b)
            return constraint;
    }

    return NULL;
}

PiecewiseLinearConstraint *
Checker::getCorrespondingAbsConstraint( const List<PiecewiseLinearCaseSplit> &splits )
{
    if ( splits.size() != 2 )
        return NULL;

    auto firstSplitTightenings = splits.front().getBoundTightenings();
    auto secondSplitTightenings = splits.back().getBoundTightenings();

    // Find an LB tightening, it marks the positive split
    auto &positiveSplit = firstSplitTightenings.front()._type == Tightening::LB
                            ? firstSplitTightenings
                            : secondSplitTightenings;
    auto &negativeSplit = firstSplitTightenings.front()._type == Tightening::LB
                            ? secondSplitTightenings
                            : firstSplitTightenings;

    unsigned b = positiveSplit.front()._variable;
    unsigned posAux = positiveSplit.back()._variable;

    unsigned negAux = negativeSplit.back()._variable;
    // Check details of both constraints - value and types
    if ( positiveSplit.size() != 2 || negativeSplit.size() != 2 ||
         positiveSplit.back()._type != Tightening::UB ||
         positiveSplit.front()._type != Tightening::LB ||
         negativeSplit.back()._type != Tightening::UB ||
         negativeSplit.front()._type != Tightening::UB )
        return NULL;

    if ( FloatUtils::areDisequal( negativeSplit.back()._value, 0.0 ) ||
         FloatUtils::areDisequal( negativeSplit.front()._value, 0.0 ) ||
         FloatUtils::areDisequal( positiveSplit.back()._value, 0.0 ) ||
         FloatUtils::areDisequal( positiveSplit.front()._value, 0.0 ) )
        return NULL;

    // Check that f=sign(b) corresponds to a problem constraints
    for ( auto &constraint : _problemConstraints )
    {
        auto constraintVars = constraint->getParticipatingVariables();
        if ( constraint->getType() == ABSOLUTE_VALUE && constraintVars.front() == b &&
             constraintVars.back() == negAux && constraintVars.exists( posAux ) )
            // Return the constraint for which f=abs(b)
            return constraint;
    }

    return NULL;
}

PiecewiseLinearConstraint *
Checker::getCorrespondingMaxConstraint( const List<PiecewiseLinearCaseSplit> &splits )
{
    for ( auto &constraint : _problemConstraints )
    {
        if ( constraint->getType() != MAX )
            continue;

        bool constraintMatched = true;

        // When checking, it is possible that the problem constraints already eliminated elements
        // that appear in the proof
        List<PiecewiseLinearCaseSplit> constraintSplits = constraint->getCaseSplits();
        MaxConstraint *maxConstraint = (MaxConstraint *)constraint;
        for ( auto const &element : maxConstraint->getEliminatedElements() )
        {
            PiecewiseLinearCaseSplit eliminatedElementCaseSplit;
            eliminatedElementCaseSplit.storeBoundTightening(
                Tightening( element, 0, Tightening::UB ) );
            constraintSplits.append( eliminatedElementCaseSplit );
        }

        for ( const auto &split : splits )
            if ( !constraintSplits.exists( split ) )
                constraintMatched = false;

        // Case corresponding to MAX_PHASE_ELIMINATED
        if ( splits.size() == 2 && splits.back().getBoundTightenings().size() == 1 &&
             splits.front().getBoundTightenings().size() == 1 &&
             splits.back().getBoundTightenings().back()._variable ==
                 splits.front().getBoundTightenings().back()._variable &&
             splits.back().getBoundTightenings().back()._variable == maxConstraint->getF() &&
             splits.back().getBoundTightenings().back()._type !=
                 splits.front().getBoundTightenings().back()._type )
            return constraint;

        if ( constraintMatched )
            return constraint;
    }
    return NULL;
}

PiecewiseLinearConstraint *
Checker::getCorrespondingDisjunctionConstraint( const List<PiecewiseLinearCaseSplit> &splits )
{
    for ( auto &constraint : _problemConstraints )
    {
        if ( constraint->getType() != DISJUNCTION )
            continue;

        bool constraintMatched = true;

        // constraintMatched will remain true iff the splits list is the same as the list of
        // disjuncts (up to order)
        for ( const auto &split : constraint->getCaseSplits() )
            if ( !splits.exists( split ) )
                constraintMatched = false;

        for ( const auto &split : splits )
            if ( !constraint->getCaseSplits().exists( split ) )
                constraintMatched = false;

        if ( constraintMatched )
            return constraint;
    }
    return NULL;
}

PiecewiseLinearConstraint *
Checker::getCorrespondingLeakyReluConstraint( const List<PiecewiseLinearCaseSplit> &splits )
{
    if ( splits.size() != 2 )
        return NULL;

    auto firstSplitTightenings = splits.front().getBoundTightenings();
    auto secondSplitTightenings = splits.back().getBoundTightenings();

    // Find the LB tightening, its var is b
    auto &activeSplit = firstSplitTightenings.front()._type == Tightening::LB
                          ? firstSplitTightenings
                          : secondSplitTightenings;
    auto &inactiveSplit = firstSplitTightenings.front()._type == Tightening::LB
                            ? secondSplitTightenings
                            : firstSplitTightenings;

    // Aux var may or may not be used in either splits, but sizes are always the same
    if ( !( activeSplit.size() == 3 && inactiveSplit.size() == 3 ) &&
         !( activeSplit.size() == 2 && inactiveSplit.size() == 2 ) )
        return NULL;

    // Check that f=LeakyRelu(b) corresponds to a problem constraints
    for ( auto &constraint : _problemConstraints )
    {
        if ( constraint->getType() == LEAKY_RELU )
        {
            auto constraintVars = constraint->getParticipatingVariables();
            unsigned b = constraintVars.front();
            unsigned inactiveAux = constraintVars.back();
            constraintVars.popBack();
            unsigned activeAux = constraintVars.back();
            constraintVars.popBack();
            unsigned f = constraintVars.back();
            if ( inactiveSplit.exists( Tightening( b, 0.0, Tightening::UB ) ) &&
                 inactiveSplit.exists( Tightening( f, 0.0, Tightening::UB ) ) &&
                 ( inactiveSplit.size() == 2 ||
                   inactiveSplit.exists( Tightening( inactiveAux, 0.0, Tightening::UB ) ) ) &&
                 activeSplit.exists( Tightening( b, 0.0, Tightening::LB ) ) &&
                 activeSplit.exists( Tightening( f, 0.0, Tightening::LB ) ) &&
                 ( activeSplit.size() == 2 ||
                   activeSplit.exists( Tightening( activeAux, 0.0, Tightening::UB ) ) ) )

                // Return the constraint for which f = LeakyRelu(b)
                return constraint;
        }
    }

    return NULL;
}

double Checker::checkReluLemma( const PLCLemma &expl,
                                PiecewiseLinearConstraint &constraint,
                                double epsilon )
{
    ASSERT( constraint.getType() == RELU && expl.getConstraintType() == RELU && epsilon > 0 );
    ASSERT( expl.getCausingVars().size() == 1 );

    unsigned causingVar = expl.getCausingVars().front();
    unsigned affectedVar = expl.getAffectedVar();
    double bound = expl.getBound();
    const List<SparseUnsortedList> &explanations = expl.getExplanations();
    Tightening::BoundType causingVarBound = expl.getCausingVarBound();
    Tightening::BoundType affectedVarBound = expl.getAffectedVarBound();
    ASSERT( explanations.size() == 1 );

    double explainedBound = UNSATCertificateUtils::computeBound( causingVar,
                                                                 causingVarBound == Tightening::UB,
                                                                 explanations.back(),
                                                                 _initialTableau,
                                                                 _groundUpperBounds.data(),
                                                                 _groundLowerBounds.data(),
                                                                 _groundUpperBounds.size() );

    List<unsigned> constraintVars = constraint.getParticipatingVariables();
    ASSERT( constraintVars.size() == 3 );

    Vector<unsigned> conVec( constraintVars.begin(), constraintVars.end() );
    unsigned b = conVec[0];
    unsigned f = conVec[1];
    unsigned aux = conVec[2];

    // If explanation is phase fixing, mark it
    if ( ( affectedVarBound == Tightening::LB && affectedVar == f &&
           FloatUtils::isPositive( bound ) ) ||
         ( affectedVarBound == Tightening::UB && affectedVar == aux &&
           FloatUtils::isZero( bound ) ) )
        constraint.setPhaseStatus( RELU_PHASE_ACTIVE );
    else if ( ( affectedVarBound == Tightening::LB && affectedVar == aux &&
                FloatUtils::isPositive( bound ) ) ||
              ( affectedVarBound == Tightening::UB && affectedVar == f &&
                FloatUtils::isZero( bound ) ) )
        constraint.setPhaseStatus( RELU_PHASE_INACTIVE );

    // Make sure the explanation is explained using a ReLU bound tightening. Cases are matching each
    // rule in ReluConstraint.cpp We allow explained bound to be tighter than the ones recorded
    // (since an explanation can explain tighter bounds), and an epsilon sized error is tolerated.

    // If lb of b is non negative, then ub of aux is 0
    if ( causingVar == b && causingVarBound == Tightening::LB && affectedVar == aux &&
         affectedVarBound == Tightening::UB && !FloatUtils::isNegative( explainedBound + epsilon ) )
        return 0;

    // If lb of f is positive, then ub if aux is 0
    else if ( causingVar == f && causingVarBound == Tightening::LB && affectedVar == aux &&
              affectedVarBound == Tightening::UB &&
              FloatUtils::isPositive( explainedBound + epsilon ) )
        return 0;

    // If lb of b is negative x, then ub of aux is -x
    else if ( causingVar == b && causingVarBound == Tightening::LB && affectedVar == aux &&
              affectedVarBound == Tightening::UB &&
              FloatUtils::isNegative( explainedBound - epsilon ) )
        return -explainedBound;

    // If lb of aux is positive, then ub of f is 0
    else if ( causingVar == aux && causingVarBound == Tightening::LB && affectedVar == f &&
              affectedVarBound == Tightening::UB &&
              FloatUtils::isPositive( explainedBound + epsilon ) )
        return 0;

    // If lb of f is negative, then it is 0
    else if ( causingVar == f && causingVarBound == Tightening::LB && affectedVar == f &&
              affectedVarBound == Tightening::LB &&
              FloatUtils::isNegative( explainedBound - epsilon ) )
        return 0;

    // Propagate ub from f to b
    else if ( causingVar == f && causingVarBound == Tightening::UB && affectedVar == b &&
              affectedVarBound == Tightening::UB )
        return explainedBound;

    // If ub of b is non positive, then ub of f is 0
    else if ( causingVar == b && causingVarBound == Tightening::UB && affectedVar == f &&
              affectedVarBound == Tightening::UB &&
              !FloatUtils::isPositive( explainedBound - epsilon ) )
        return 0;

    // If ub of b is non positive x, then lb of aux is -x
    else if ( causingVar == b && causingVarBound == Tightening::UB && affectedVar == aux &&
              affectedVarBound == Tightening::LB &&
              !FloatUtils::isPositive( explainedBound - epsilon ) )
        return -explainedBound;

    // If ub of b is positive, then propagate to f ( positivity of explained bound is not checked
    // since negative explained ub can always explain positive bound )
    else if ( causingVar == b && causingVarBound == Tightening::UB && affectedVar == f &&
              affectedVarBound == Tightening::UB &&
              FloatUtils::isPositive( explainedBound + epsilon ) )
        return explainedBound;

    // If ub of aux is x, then lb of b is -x
    else if ( causingVar == aux && causingVarBound == Tightening::UB && affectedVar == b &&
              affectedVarBound == Tightening::LB )
        return -explainedBound;

    return affectedVarBound == Tightening::UB ? FloatUtils::infinity()
                                              : FloatUtils::negativeInfinity();
}

double Checker::checkSignLemma( const PLCLemma &expl,
                                PiecewiseLinearConstraint &constraint,
                                double epsilon )
{
    ASSERT( constraint.getType() == SIGN && expl.getConstraintType() == SIGN && epsilon > 0 );
    ASSERT( expl.getCausingVars().size() == 1 );

    unsigned causingVar = expl.getCausingVars().front();
    unsigned affectedVar = expl.getAffectedVar();
    double bound = expl.getBound();
    const List<SparseUnsortedList> &explanations = expl.getExplanations();
    Tightening::BoundType causingVarBound = expl.getCausingVarBound();
    Tightening::BoundType affectedVarBound = expl.getAffectedVarBound();
    ASSERT( explanations.size() == 1 );

    double explainedBound = UNSATCertificateUtils::computeBound( causingVar,
                                                                 causingVarBound == Tightening::UB,
                                                                 explanations.front(),
                                                                 _initialTableau,
                                                                 _groundUpperBounds.data(),
                                                                 _groundLowerBounds.data(),
                                                                 _groundUpperBounds.size() );
    List<unsigned> constraintVars = constraint.getParticipatingVariables();
    ASSERT( constraintVars.size() == 2 );

    unsigned b = constraintVars.front();
    unsigned f = constraintVars.back();

    // Any explanation is phase fixing
    if ( ( affectedVarBound == Tightening::LB && affectedVar == f &&
           FloatUtils::gt( bound, -1 ) ) ||
         ( affectedVarBound == Tightening::LB && affectedVar == b &&
           !FloatUtils::isNegative( bound ) ) )
        constraint.setPhaseStatus( SIGN_PHASE_POSITIVE );
    else if ( ( affectedVarBound == Tightening::UB && affectedVar == f &&
                FloatUtils::gt( bound, 1 ) ) ||
              ( affectedVarBound == Tightening::UB && affectedVar == b &&
                FloatUtils::isNegative( bound ) ) )
        constraint.setPhaseStatus( SIGN_PHASE_NEGATIVE );

    // Make sure the explanation is explained using a sign bound tightening. Cases are matching each
    // rule in SignConstraint.cpp We allow explained bound to be tighter than the ones recorded
    // (since an explanation can explain tighter bounds), and an epsilon sized error is tolerated.

    // If lb of f is > -1, then lb of f is 1
    if ( causingVar == f && causingVarBound == Tightening::LB && affectedVar == f &&
         affectedVarBound == Tightening::LB && FloatUtils::areEqual( bound, 1 ) &&
         FloatUtils::gte( explainedBound + epsilon, -1 ) )
        return 1;

    // If lb of f is > -1, then lb of b is 0
    else if ( causingVar == f && causingVarBound == Tightening::LB && affectedVar == b &&
              affectedVarBound == Tightening::LB && FloatUtils::isZero( bound ) &&
              FloatUtils::gte( explainedBound + epsilon, -1 ) )
        return 0;

    // If lb of b is non negative, then lb of f is 1
    else if ( causingVar == b && causingVarBound == Tightening::LB && affectedVar == f &&
              affectedVarBound == Tightening::LB && FloatUtils::areEqual( bound, 1 ) &&
              !FloatUtils::isNegative( explainedBound + epsilon ) )
        return 1;

    // If ub of f is < 1, then ub of f is -1
    else if ( causingVar == f && causingVarBound == Tightening::UB && affectedVar == f &&
              affectedVarBound == Tightening::UB && FloatUtils::areEqual( bound, -1 ) &&
              FloatUtils::lte( explainedBound - epsilon, 1 ) )
        return -1;

    // If ub of f is < 1, then ub of b is 0
    else if ( causingVar == f && causingVarBound == Tightening::UB && affectedVar == b &&
              affectedVarBound == Tightening::UB && FloatUtils::isZero( bound ) &&
              FloatUtils::lte( explainedBound - epsilon, 1 ) )
        return 0;

    // If ub of b is negative, then ub of f is -1
    else if ( causingVar == b && causingVarBound == Tightening::UB && affectedVar == f &&
              affectedVarBound == Tightening::UB && FloatUtils::areEqual( bound, -1 ) &&
              FloatUtils::isNegative( explainedBound - epsilon ) )
        return -1;

    return affectedVarBound == Tightening::UB ? FloatUtils::infinity()
                                              : FloatUtils::negativeInfinity();
}

double Checker::checkAbsLemma( const PLCLemma &expl,
                               PiecewiseLinearConstraint &constraint,
                               double epsilon )
{
    ASSERT( constraint.getType() == ABSOLUTE_VALUE && expl.getConstraintType() == ABSOLUTE_VALUE );
    ASSERT( expl.getCausingVars().size() == 2 || expl.getCausingVars().size() == 1 );

    Tightening::BoundType causingVarBound = expl.getCausingVarBound();
    Tightening::BoundType affectedVarBound = expl.getAffectedVarBound();
    unsigned affectedVar = expl.getAffectedVar();
    double bound = expl.getBound();
    List<unsigned> constraintVars = constraint.getParticipatingVariables();
    ASSERT( constraintVars.size() == 4 );

    Vector<unsigned> conVec( constraintVars.begin(), constraintVars.end() );
    unsigned b = conVec[0];
    unsigned f = conVec[1];
    unsigned pos = conVec[2];
    unsigned neg = conVec[3];

    // Make sure the explanation is explained using an absolute value bound tightening. Cases are
    // matching each rule in AbsolutValueConstraint.cpp We allow explained bound to be tighter than
    // the ones recorded (since an explanation can explain tighter bounds)

    if ( expl.getCausingVars().size() == 2 )
    {
        unsigned firstCausingVar = expl.getCausingVars().front();
        unsigned secondCausingVar = expl.getCausingVars().back();

        const SparseUnsortedList &firstExplanation = expl.getExplanations().front();
        const SparseUnsortedList &secondExplanation = expl.getExplanations().back();

        // Case of a non phase-fixing lemma
        if ( firstCausingVar == secondCausingVar )
        {
            double explainedUpperBound =
                UNSATCertificateUtils::computeBound( firstCausingVar,
                                                     Tightening::UB,
                                                     firstExplanation,
                                                     _initialTableau,
                                                     _groundUpperBounds.data(),
                                                     _groundLowerBounds.data(),
                                                     _groundUpperBounds.size() );
            double explainedLowerBound =
                UNSATCertificateUtils::computeBound( secondCausingVar,
                                                     Tightening::LB,
                                                     secondExplanation,
                                                     _initialTableau,
                                                     _groundUpperBounds.data(),
                                                     _groundLowerBounds.data(),
                                                     _groundUpperBounds.size() );

            // b is always the causing var, affecting the ub of f
            if ( affectedVar == f && firstCausingVar == b && affectedVarBound == Tightening::UB &&
                 bound > 0 )
                return FloatUtils::max( explainedUpperBound, -explainedLowerBound );

            return affectedVarBound == Tightening::UB ? FloatUtils::infinity()
                                                      : FloatUtils::negativeInfinity();
        }
        // Cases of a phase-fixing lemma
        else
        {
            ASSERT( firstCausingVar == b && secondCausingVar == f );
            double explainedBBound =
                UNSATCertificateUtils::computeBound( firstCausingVar,
                                                     causingVarBound == Tightening::UB,
                                                     firstExplanation,
                                                     _initialTableau,
                                                     _groundUpperBounds.data(),
                                                     _groundLowerBounds.data(),
                                                     _groundUpperBounds.size() );
            double explainedFBound =
                UNSATCertificateUtils::computeBound( secondCausingVar,
                                                     Tightening::LB,
                                                     secondExplanation,
                                                     _initialTableau,
                                                     _groundUpperBounds.data(),
                                                     _groundLowerBounds.data(),
                                                     _groundUpperBounds.size() );

            if ( affectedVar == neg && causingVarBound == Tightening::UB &&
                 explainedFBound > explainedBBound - epsilon && bound == 0 )
            {
                constraint.setPhaseStatus( ABS_PHASE_NEGATIVE );
                return 0;
            }
            else if ( affectedVar == pos && causingVarBound == Tightening::LB &&
                      explainedFBound > -explainedBBound - epsilon && bound == 0 )
            {
                constraint.setPhaseStatus( ABS_PHASE_POSITIVE );
                return 0;
            }

            return affectedVarBound == Tightening::UB ? FloatUtils::infinity()
                                                      : FloatUtils::negativeInfinity();
        }
    }

    // Cases of a phase-fixing lemma
    const List<SparseUnsortedList> &explanation = expl.getExplanations();
    unsigned causingVar = expl.getCausingVars().front();

    double explainedBound = UNSATCertificateUtils::computeBound( causingVar,
                                                                 causingVarBound == Tightening::UB,
                                                                 explanation.front(),
                                                                 _initialTableau,
                                                                 _groundUpperBounds.data(),
                                                                 _groundLowerBounds.data(),
                                                                 _groundUpperBounds.size() );

    if ( affectedVar == pos && causingVar == b && causingVarBound == Tightening::LB &&
         !FloatUtils::isNegative( explainedBound + epsilon ) && bound == 0 )
    {
        constraint.setPhaseStatus( ABS_PHASE_POSITIVE );
        return 0;
    }
    else if ( affectedVar == neg && causingVar == b && causingVarBound == Tightening::UB &&
              !FloatUtils::isPositive( explainedBound - epsilon ) && bound == 0 )
    {
        constraint.setPhaseStatus( ABS_PHASE_NEGATIVE );
        return 0;
    }
    else if ( affectedVar == neg && causingVar == pos && causingVarBound == Tightening::LB &&
              FloatUtils::isPositive( explainedBound + epsilon ) && bound == 0 )
    {
        constraint.setPhaseStatus( ABS_PHASE_NEGATIVE );
        return 0;
    }
    else if ( affectedVar == pos && causingVar == neg && causingVarBound == Tightening::LB &&
              FloatUtils::isPositive( explainedBound + epsilon ) && bound == 0 )
    {
        constraint.setPhaseStatus( ABS_PHASE_POSITIVE );
        return 0;
    }

    return affectedVarBound == Tightening::UB ? FloatUtils::infinity()
                                              : FloatUtils::negativeInfinity();
}

double Checker::checkMaxLemma( const PLCLemma &expl, PiecewiseLinearConstraint &constraint )
{
    ASSERT( constraint.getType() == MAX && expl.getConstraintType() == MAX );
    MaxConstraint *maxConstraint = (MaxConstraint *)&constraint;

    double maxBound = maxConstraint->getMaxValueOfEliminatedPhases();
    const List<unsigned> causingVars = expl.getCausingVars();
    unsigned affectedVar = expl.getAffectedVar();
    const List<SparseUnsortedList> &allExplanations = expl.getExplanations();
    Tightening::BoundType causingVarBound = expl.getCausingVarBound();
    Tightening::BoundType affectedVarBound = expl.getAffectedVarBound();
    double explainedBound = affectedVarBound == Tightening::UB ? FloatUtils::infinity()
                                                               : FloatUtils::negativeInfinity();

    unsigned f = maxConstraint->getF();

    Set<unsigned> constraintElements = maxConstraint->getParticipatingElements();
    for ( const auto &var : causingVars )
        if ( !constraintElements.exists( var ) )
            return explainedBound;


    // Only tightening type is of the form f = element, for some element with the maximal upper
    // bound
    unsigned counter = 0;

    Vector<unsigned> causingVarsVec = Vector<unsigned>( 0 );
    for ( const auto &var : causingVars )
        causingVarsVec.append( var );

    for ( const auto &explanation : allExplanations )
    {
        explainedBound = UNSATCertificateUtils::computeBound( causingVarsVec[counter],
                                                              Tightening::UB,
                                                              explanation,
                                                              _initialTableau,
                                                              _groundUpperBounds.data(),
                                                              _groundLowerBounds.data(),
                                                              _groundUpperBounds.size() );
        maxBound = FloatUtils::max( maxBound, explainedBound );
        ++counter;
    }

    if ( causingVarBound == Tightening::UB && affectedVar == f &&
         affectedVarBound == Tightening::UB )
        return maxBound;

    return affectedVarBound == Tightening::UB ? FloatUtils::infinity()
                                              : FloatUtils::negativeInfinity();
}


double Checker::checkLeakyReluLemma( const PLCLemma &expl,
                                     PiecewiseLinearConstraint &constraint,
                                     double epsilon )
{
    ASSERT( constraint.getType() == LEAKY_RELU && expl.getConstraintType() == LEAKY_RELU &&
            epsilon > 0 );
    ASSERT( expl.getCausingVars().size() == 1 );

    unsigned causingVar = expl.getCausingVars().front();
    unsigned affectedVar = expl.getAffectedVar();
    double bound = expl.getBound();
    const List<SparseUnsortedList> &explanations = expl.getExplanations();
    Tightening::BoundType causingVarBound = expl.getCausingVarBound();
    Tightening::BoundType affectedVarBound = expl.getAffectedVarBound();
    ASSERT( explanations.size() == 1 );

    double explainedBound = UNSATCertificateUtils::computeBound( causingVar,
                                                                 causingVarBound == Tightening::UB,
                                                                 explanations.back(),
                                                                 _initialTableau,
                                                                 _groundUpperBounds.data(),
                                                                 _groundLowerBounds.data(),
                                                                 _groundUpperBounds.size() );

    List<unsigned> constraintVars = constraint.getParticipatingVariables();
    ASSERT( constraintVars.size() == 4 );

    Vector<unsigned> conVec( constraintVars.begin(), constraintVars.end() );
    unsigned b = conVec[0];
    unsigned f = conVec[1];
    unsigned activeAux = conVec[2];
    unsigned inactiveAux = conVec[3];

    double slope = ( (LeakyReluConstraint *)&constraint )->getSlope();

    // If explanation is phase fixing, mark it
    if ( ( affectedVarBound == Tightening::LB && affectedVar == f &&
           !FloatUtils::isNegative( bound ) ) ||
         ( affectedVarBound == Tightening::LB && affectedVar == b &&
           !FloatUtils::isNegative( bound ) ) ||
         ( affectedVarBound == Tightening::LB && affectedVar == inactiveAux &&
           FloatUtils::isPositive( bound ) ) ||
         ( affectedVarBound == Tightening::UB && affectedVar == activeAux &&
           FloatUtils::isZero( bound ) ) )
        constraint.setPhaseStatus( RELU_PHASE_ACTIVE );
    else if ( ( affectedVarBound == Tightening::UB && affectedVar == f &&
                FloatUtils::isNegative( bound ) ) ||
              ( affectedVarBound == Tightening::UB && affectedVar == b &&
                FloatUtils::isNegative( bound ) ) ||
              ( affectedVarBound == Tightening::LB && affectedVar == activeAux &&
                FloatUtils::isPositive( bound ) ) ||
              ( affectedVarBound == Tightening::UB && affectedVar == inactiveAux &&
                FloatUtils::isZero( bound ) ) )
        constraint.setPhaseStatus( RELU_PHASE_INACTIVE );

    // Make sure the explanation is explained using a LeakyReLU bound tightening. Cases are matching
    // each rule in ReluConstraint.cpp We allow explained bound to be tighter than the ones recorded
    // (since an explanation can explain tighter bounds), and an epsilon sized error is tolerated.

    // If lb of b or f is positive, then ub of active aux is 0
    if ( ( causingVar == b || causingVar == f ) && causingVarBound == Tightening::LB &&
         affectedVar == activeAux && affectedVarBound == Tightening::UB &&
         FloatUtils::isPositive( explainedBound + epsilon ) )
        return 0;

    // If lb of f is negative, then lb of b is bound / slope
    if ( causingVar == f && causingVarBound == Tightening::LB && affectedVar == b &&
         affectedVarBound == Tightening::LB && FloatUtils::isNegative( bound ) &&
         FloatUtils::gte( explainedBound + epsilon, bound * slope ) )
        return explainedBound / slope;

    // If lb of active aux is positive, then ub of inactive aux is 0
    if ( causingVar == activeAux && causingVarBound == Tightening::LB &&
         affectedVar == inactiveAux && affectedVarBound == Tightening::UB &&
         FloatUtils::isPositive( explainedBound + epsilon ) )
        return 0;

    // ... and vice versa
    if ( causingVar == inactiveAux && causingVarBound == Tightening::LB &&
         affectedVar == activeAux && affectedVarBound == Tightening::UB &&
         FloatUtils::isPositive( explainedBound + epsilon ) )
        return 0;

    // If ub of f is positive, propagate to b
    if ( causingVar == f && causingVarBound == Tightening::UB && affectedVar == b &&
         affectedVarBound == Tightening::UB && FloatUtils::isPositive( bound ) &&
         FloatUtils::lte( explainedBound - epsilon, bound ) )
        return explainedBound;

    // ... and vice versa
    if ( causingVar == b && causingVarBound == Tightening::UB && affectedVar == f &&
         affectedVarBound == Tightening::UB && FloatUtils::isPositive( bound ) &&
         FloatUtils::lte( explainedBound - epsilon, bound ) )
        return explainedBound;

    // If ub of f or b is negative, then ub of inactive aux is 0
    if ( ( causingVar == b || causingVar == f ) && causingVarBound == Tightening::UB &&
         affectedVar == inactiveAux && affectedVarBound == Tightening::UB &&
         FloatUtils::isNegative( explainedBound - epsilon ) )
        return 0;

    return affectedVarBound == Tightening::UB ? FloatUtils::infinity()
                                              : FloatUtils::negativeInfinity();
}
