/*********************                                                        */
/*! \file MockEngine.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Duligur Ibeling
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#ifndef __MockEngine_h__
#define __MockEngine_h__

#include "IEngine.h"
#include "List.h"
#include "PiecewiseLinearCaseSplit.h"
#include "PiecewiseLinearConstraint.h"
#include "SearchTreeHandler.h"
#include "context/context.h"

#include <cxxtest/TestSuite.h>

class String;

class MockEngine : public IEngine
{
public:
    MockEngine()
    {
        wasCreated = false;
        wasDiscarded = false;

        lastStoredState = NULL;
    }

    ~MockEngine()
    {
    }

    bool wasCreated;
    bool wasDiscarded;

    void mockConstructor()
    {
        TS_ASSERT( !wasCreated );
        wasCreated = true;
    }

    void mockDestructor()
    {
        TS_ASSERT( wasCreated );
        TS_ASSERT( !wasDiscarded );
        wasDiscarded = true;
    }

    struct Bound
    {
        Bound( unsigned variable, double bound )
        {
            _variable = variable;
            _bound = bound;
        }

        unsigned _variable;
        double _bound;
    };

    List<Bound> lastLowerBounds;
    List<Bound> lastUpperBounds;
    List<Equation> lastEquations;
    void applySplit( const PiecewiseLinearCaseSplit &split ) override
    {
        List<Tightening> bounds = split.getBoundTightenings();
        auto equations = split.getEquations();
        for ( auto &it : equations )
        {
            lastEquations.append( it );
        }

        for ( auto &bound : bounds )
        {
            if ( bound._type == Tightening::LB )
            {
                lastLowerBounds.append( Bound( bound._variable, bound._value ) );
            }
            else
            {
                lastUpperBounds.append( Bound( bound._variable, bound._value ) );
            }
        }
    }

    void applyPlcPhaseFixingTightenings( PiecewiseLinearConstraint & /*constraint*/ ) override
    {
    }


    void postContextPopHook() override
    {
    }

    void preContextPushHook() override
    {
    }

    mutable EngineState *lastStoredState;
    void storeState( EngineState &state, TableauStateStorageLevel /*level*/ ) const override
    {
        lastStoredState = &state;
    }

    const EngineState *lastRestoredState;
    void restoreState( const EngineState &state ) override
    {
        lastRestoredState = &state;
    }

    void setNumPlConstraintsDisabledByValidSplits( unsigned /* numConstraints */ ) override
    {
    }

    unsigned _timeToSolve;
    ExitCode _exitCode;

    bool solve( double timeoutInSeconds ) override
    {
        if ( timeoutInSeconds >= _timeToSolve )
            _exitCode = ExitCode::TIMEOUT;
        return _exitCode == ExitCode::SAT;
    }

    void setTimeToSolve( unsigned timeToSolve )
    {
        _timeToSolve = timeToSolve;
    }

    ExitCode getExitCode() const override
    {
        return _exitCode;
    }

    void setExitCode( ExitCode exitCode ) override
    {
        _exitCode = exitCode;
    }

    void reset() override
    {
    }

    List<unsigned> _inputVariables;
    void setInputVariables( List<unsigned> &inputVariables )
    {
        _inputVariables = inputVariables;
    }

    List<unsigned> getInputVariables() const override
    {
        return _inputVariables;
    }

    void updateScores( DivideStrategy /**/ )
    {
    }

    mutable SearchTreeState *lastRestoredSearchTreeState;
    bool restoreSearchTreeState( SearchTreeState &searchTreeState ) override
    {
        lastRestoredSearchTreeState = &searchTreeState;
        return true;
    }

    mutable SearchTreeState *lastStoredSearchTreeState;
    void storeSearchTreeState( SearchTreeState &searchTreeState ) override
    {
        lastStoredSearchTreeState = &searchTreeState;
    }

    List<PiecewiseLinearConstraint *> _constraintsToSplit;
    void setSplitPLConstraint( PiecewiseLinearConstraint *constraint )
    {
        _constraintsToSplit.append( constraint );
    }

    PiecewiseLinearConstraint *pickSplitPLConstraint( DivideStrategy /**/ ) override
    {
        if ( !_constraintsToSplit.empty() )
        {
            PiecewiseLinearConstraint *ptr = *_constraintsToSplit.begin();
            _constraintsToSplit.erase( ptr );
            return ptr;
        }
        else
            return NULL;
    }

    PiecewiseLinearConstraint *pickSplitPLConstraintSnC( SnCDivideStrategy /**/ ) override
    {
        if ( !_constraintsToSplit.empty() )
        {
            PiecewiseLinearConstraint *ptr = *_constraintsToSplit.begin();
            _constraintsToSplit.erase( ptr );
            return ptr;
        }
        else
            return NULL;
    }

    bool _snc;
    CVC4::context::Context _context;

    void applySnCSplit( PiecewiseLinearCaseSplit /*split*/, String /*queryId*/ ) override
    {
        _snc = true;
        _context.push();
    }

    bool inSnCMode() const override
    {
        return _snc;
    }

    void applyAllBoundTightenings() override
    {
    }

    bool applyAllValidConstraintCaseSplits() override
    {
        return false;
    };

    CVC4::context::Context &getContext() override
    {
        return _context;
    }

    bool consistentBounds() const override
    {
        return true;
    }

    double explainBound( unsigned /* var */, bool /* isUpper */ ) const override
    {
        return 0.0;
    }

    void updateGroundUpperBound( unsigned /* var */, double /* value */ )
    {
    }

    void updateGroundLowerBound( unsigned /*var*/, double /*value*/ )
    {
    }

    double getGroundBound( unsigned /*var*/, bool /*isUpper*/ ) const override
    {
        return 0;
    }
    std::shared_ptr<GroundBoundManager::GroundBoundEntry>
    getGroundBoundEntry( unsigned /*var*/, bool /*isUpper*/ ) const override
    {
        return nullptr;
    }


    UnsatCertificateNode *getUNSATCertificateCurrentPointer() const override
    {
        return NULL;
    }

    void setUNSATCertificateCurrentPointer( UnsatCertificateNode * /* node*/ ) override
    {
    }

    const UnsatCertificateNode *getUNSATCertificateRoot() const override
    {
        return NULL;
    }

    bool certifyUNSATCertificate() override
    {
        return true;
    }

    void explainSimplexFailure() override
    {
    }

    const BoundExplainer *getBoundExplainer() const override
    {
        return NULL;
    }

    void setBoundExplainerContent( BoundExplainer * /*boundExplainer */ ) override
    {
    }

    bool propagateBoundManagerTightenings() override
    {
        return false;
    }

    bool shouldProduceProofs() const override
    {
        return true;
    }

    std::shared_ptr<GroundBoundManager::GroundBoundEntry>
    setGroundBoundFromLemma( const std::shared_ptr<PLCLemma> /*lemma*/,
                             bool /*isPhaseFixing*/ ) override
    {
        return nullptr;
    }

    bool shouldSolveWithMILP() const override
    {
        return false;
    }

    bool shouldSolveWithCDCL() const override
    {
        return false;
    }

    bool solveWithCDCL( double timeoutInSeconds ) override
    {
        if ( timeoutInSeconds >= _timeToSolve )
            _exitCode = ExitCode::TIMEOUT;
        return _exitCode == ExitCode::SAT;
    }

    Set<int>
    clauseFromContradictionVector( const SparseUnsortedList &, unsigned, int, bool ) override
    {
        return Set<int>();
    }

    Vector<int> explainPhase( const PiecewiseLinearConstraint * ) override
    {
        return Vector<int>();
    }

    void explainGurobiFailure() override
    {
    }

    void removeLiteralFromPropagations( int /*literal*/ ) override
    {
    }

    void initializeSolver() override
    {
    }

    void assertEngineBoundsForSplit( const PiecewiseLinearCaseSplit & /*split*/ ) override
    {
    }

    bool shouldExitDueToTimeout( double ) const override
    {
        return false;
    }

    unsigned getVerbosity() const override
    {
        return 0;
    }

    void exportQueryWithError( String ) override
    {
    }

    const List<PiecewiseLinearConstraint *> *getPiecewiseLinearConstraints() const override
    {
        return NULL;
    }

    LPSolverType getLpSolverType() const override
    {
        return LPSolverType::NATIVE;
    }

    NLR::NetworkLevelReasoner *getNetworkLevelReasoner() const override
    {
        return nullptr;
    }

    void restoreInitialEngineState() override
    {
    }

    bool checkAssignmentComplianceWithClause( const Set<int> & /*clause*/ ) const override
    {
        return true;
    }

    void addPLCLemma( std::shared_ptr<PLCLemma> & /*explanation*/ )
    {
    }

    bool solveWithMILPEncoding( double timeoutInSeconds ) override
    {
        if ( timeoutInSeconds >= _timeToSolve )
            _exitCode = ExitCode::TIMEOUT;
        return _exitCode == ExitCode::SAT;
    }

    void incNumOfLemmas() override
    {
    }
};

#endif // __MockEngine_h__

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../../TAGS"
// c-basic-offset: 4
// End:
//
