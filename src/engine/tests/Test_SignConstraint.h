//
// Created by guyam on 6/4/20.
//

#ifndef MARABOU_TEST_SIGNCONSTRAINT_H
#define MARABOU_TEST_SIGNCONSTRAINT_H


#include <cxxtest/TestSuite.h>

#include "InputQuery.h"
#include "MockConstraintBoundTightener.h"
#include "MockTableau.h"
#include "PiecewiseLinearCaseSplit.h"
#include "ReluConstraint.h"
#include "SignConstraint.h"
#include "MarabouError.h"
#include "/cs/usr/guyam/CLionProjects/Marabou/src/common/tests/MockErrno.h"
//#include "MockErrno.h" // todo - yuval says red line ok


#include <string.h>





class MockForSignConstraint
        : public MockErrno
{
public:
};




class SignConstraintTestSuite : public CxxTest::TestSuite {
public:
    MockForSignConstraint *mock;

    void setUp() {
        TS_ASSERT(mock = new MockForSignConstraint);
    }

    void tearDown() {
        TS_ASSERT_THROWS_NOTHING(delete mock);
    }

    void test_sign_constraint() { // TODO - PASSES
        unsigned b = 1;
        unsigned f = 4;

        SignConstraint sign(b, f); // define constraint

        List<unsigned> participatingVariables; // needs to return 1 and 4 - the tableu vars
        TS_ASSERT_THROWS_NOTHING(
                participatingVariables = sign.getParticipatingVariables()); // see doesnt throw
        TS_ASSERT_EQUALS(participatingVariables.size(),
                         2U); // check that equals 2 - becausse there are 2vars
        auto it = participatingVariables.begin();
        TS_ASSERT_EQUALS(*it, b);
        ++it;
        TS_ASSERT_EQUALS(*it, f);

        TS_ASSERT(sign.participatingVariable(b));
        TS_ASSERT(sign.participatingVariable(f));
        TS_ASSERT(!sign.participatingVariable(2));
        TS_ASSERT(!sign.participatingVariable(0));
        TS_ASSERT(!sign.participatingVariable(5));

        TS_ASSERT_THROWS_EQUALS(sign.satisfied(),
                                const MarabouError &e,
                                e.getCode(),
                                MarabouError::PARTICIPATING_VARIABLES_ABSENT);

        sign.notifyVariableValue(b, -1); // change val of b to '-1'
        sign.notifyVariableValue(f, -1); // change val of f to '1

        TS_ASSERT(sign.satisfied()); // f= Relu (b) or f=sign(b)

        sign.notifyVariableValue(f, 1);

        TS_ASSERT(!sign.satisfied());

        sign.notifyVariableValue(b, 2);

        TS_ASSERT(sign.satisfied());

        sign.notifyVariableValue(b, -2);

        TS_ASSERT(!sign.satisfied()); // because f is '1'

        sign.notifyVariableValue(f, 1);

        TS_ASSERT(!sign.satisfied());

        sign.notifyVariableValue(b, 0);

        TS_ASSERT(sign.satisfied());

        sign.notifyVariableValue(b, 9);

        TS_ASSERT(sign.satisfied());

        sign.notifyVariableValue(4, -8); // is first var is 'f'

        // A relu cannot be satisfied if f is not '1' or '-1'
        TS_ASSERT(!sign.satisfied());

        sign.notifyVariableValue(4, 1.5); // is first var is 'f'

        // A relu cannot be satisfied if f is not '1' or '-1'
        TS_ASSERT(!sign.satisfied());

        sign.notifyVariableValue(f, -1);
        sign.notifyVariableValue(b, 11);

        TS_ASSERT(!sign.satisfied());

        // Changing variable indices
        sign.notifyVariableValue(b, 1);
        sign.notifyVariableValue(f, 1);
        TS_ASSERT(sign.satisfied());


        unsigned newB = 12;
        unsigned newF = 14;

        TS_ASSERT_THROWS_NOTHING(sign.updateVariableIndex(b, newB));
        TS_ASSERT_THROWS_NOTHING(sign.updateVariableIndex(f, newF));

        TS_ASSERT(sign.satisfied());

        sign.notifyVariableValue(newF, -1);

        TS_ASSERT(!sign.satisfied());

        sign.notifyVariableValue(newB, -0.1);

        TS_ASSERT(sign.satisfied());
    }


    void test_sign_fixes() {
        unsigned b = 1;
        unsigned f = 4;

        SignConstraint sign(b, f);

        List<PiecewiseLinearConstraint::Fix> fixes;
        List<PiecewiseLinearConstraint::Fix>::iterator it;

        sign.notifyVariableValue(b, -1);
        sign.notifyVariableValue(f, 1);
        fixes = sign.getPossibleFixes();
        it = fixes.begin();
        TS_ASSERT_EQUALS(it->_variable, f);
        TS_ASSERT_EQUALS(it->_value, -1); // check if fix we changed to make f '-1'

        sign.notifyVariableValue(b, 0);
        sign.notifyVariableValue(f, -1);
        fixes = sign.getPossibleFixes();
        it = fixes.begin();
        TS_ASSERT_EQUALS(it->_variable, f);
        TS_ASSERT_EQUALS(it->_value, 1); // check if fix we changed to make f '1'


        sign.notifyVariableValue(b, 3);
        sign.notifyVariableValue(f, -1);
        fixes = sign.getPossibleFixes();
        it = fixes.begin();
        TS_ASSERT_EQUALS(it->_variable, f);
        TS_ASSERT_EQUALS(it->_value, 1); // check if fix we changed to make f '1'



        sign.notifyVariableValue(b, -2);
        sign.notifyVariableValue(f, 1);

        fixes = sign.getPossibleFixes();
        it = fixes.begin();
        TS_ASSERT_EQUALS(it->_variable, f);
        TS_ASSERT_EQUALS(it->_value, -1);


        sign.notifyVariableValue(b, 11);
        sign.notifyVariableValue(f, 0);

        fixes = sign.getPossibleFixes();
        it = fixes.begin();
        TS_ASSERT_EQUALS(it->_variable, f);
        TS_ASSERT_EQUALS(it->_value, 1);
    }


    void test_sign_case_splits() {

        unsigned b = 1;
        unsigned f = 4;

        SignConstraint sign(b, f);

        List<PiecewiseLinearConstraint::Fix> fixes;
        List<PiecewiseLinearConstraint::Fix>::iterator it;

        List<PiecewiseLinearCaseSplit> splits = sign.getCaseSplits(); // returns list with 2 elemns for relu

        Equation activeEquation, inactiveEquation;

        TS_ASSERT_EQUALS(splits.size(), 2U);

        List<PiecewiseLinearCaseSplit>::iterator split1 = splits.begin();
        List<PiecewiseLinearCaseSplit>::iterator split2 = split1;
        ++split2;

        TS_ASSERT(isPositiveSplit(b, f, split1) || isPositiveSplit(b, f, split2));
        TS_ASSERT(isNegativeSplit(b, f, split1) || isNegativeSplit(b, f, split2));
    }


    bool isPositiveSplit(unsigned b, unsigned f,
                         List<PiecewiseLinearCaseSplit>::iterator &split) { // return true only if 2 matching bound and no equations
        List<Tightening> bounds = split->getBoundTightenings();

        auto bound = bounds.begin();
        Tightening bound1 = *bound;

        TS_ASSERT_EQUALS(bound1._variable, b);
        TS_ASSERT_EQUALS(bound1._value, 0.0);

        if (bound1._type != Tightening::LB)
            return false;

        TS_ASSERT_EQUALS(bounds.size(), 2U);

        ++bound;
        Tightening bound2 = *bound;

        TS_ASSERT_EQUALS(bound2._variable, f);
        TS_ASSERT_EQUALS(bound2._value, 1);
        TS_ASSERT_EQUALS(bound2._type, Tightening::LB);

        auto equations = split->getEquations();
        TS_ASSERT(equations.empty());

        return true;
    }


    bool isNegativeSplit(unsigned b, unsigned f,
                         List<PiecewiseLinearCaseSplit>::iterator &split) { // return true only if 2 matching bound and no equations
        List<Tightening> bounds = split->getBoundTightenings();

        auto bound = bounds.begin();
        Tightening bound1 = *bound;

        TS_ASSERT_EQUALS(bound1._variable, b);
        TS_ASSERT_EQUALS(bound1._value, 0.0);

        if (bound1._type != Tightening::UB)
            return false;

        TS_ASSERT_EQUALS(bounds.size(), 2U);

        ++bound;
        Tightening bound2 = *bound;

        TS_ASSERT_EQUALS(bound2._variable, f);
        TS_ASSERT_EQUALS(bound2._value, -1);
        TS_ASSERT_EQUALS(bound2._type, Tightening::UB);

        auto equations = split->getEquations();
        TS_ASSERT(equations.empty());

        return true;
    }


    void test_register_as_watcher() {
        unsigned b = 1;
        unsigned f = 4;

        MockTableau tableau;

        ReluConstraint relu(b, f);

        TS_ASSERT_THROWS_NOTHING(relu.registerAsWatcher(&tableau));

        TS_ASSERT_EQUALS(tableau.lastRegisteredVariableToWatcher.size(), 2U);
        TS_ASSERT(tableau.lastUnregisteredVariableToWatcher.empty());
        TS_ASSERT_EQUALS(tableau.lastRegisteredVariableToWatcher[b].size(), 1U);
        TS_ASSERT(tableau.lastRegisteredVariableToWatcher[b].exists(&relu));
        TS_ASSERT_EQUALS(tableau.lastRegisteredVariableToWatcher[f].size(), 1U);
        TS_ASSERT(tableau.lastRegisteredVariableToWatcher[f].exists(&relu));

        tableau.lastRegisteredVariableToWatcher.clear();

        TS_ASSERT_THROWS_NOTHING(relu.unregisterAsWatcher(&tableau));

        TS_ASSERT(tableau.lastRegisteredVariableToWatcher.empty());
        TS_ASSERT_EQUALS(tableau.lastUnregisteredVariableToWatcher.size(), 2U);
        TS_ASSERT_EQUALS(tableau.lastUnregisteredVariableToWatcher[b].size(), 1U);
        TS_ASSERT(tableau.lastUnregisteredVariableToWatcher[b].exists(&relu));
        TS_ASSERT_EQUALS(tableau.lastUnregisteredVariableToWatcher[f].size(), 1U);
        TS_ASSERT(tableau.lastUnregisteredVariableToWatcher[f].exists(&relu));
    }


    void test_fix_positive() {
        unsigned b = 1;
        unsigned f = 4;

        MockTableau tableau;

        SignConstraint sign(b, f);

        sign.registerAsWatcher(&tableau);

        List<PiecewiseLinearCaseSplit> splits = sign.getCaseSplits();
        TS_ASSERT_EQUALS(splits.size(), 2U);

        sign.notifyLowerBound(1, 0); // todo conitnue debug 1106!!!. make sure pass all tests and then last test
//        TS_ASSERT_THROWS_EQUALS(splits = sign.getCaseSplits(),
//                                const MarabouError &e,
//                                e.getCode(),
//                                MarabouError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT);
//
//        sign.unregisterAsWatcher(&tableau);
//
//        sign = SignConstraint(b, f);
//
//        sign.registerAsWatcher(&tableau);
//
//        splits = sign.getCaseSplits();
//        TS_ASSERT_EQUALS(splits.size(), 2U);
//
//        sign.notifyLowerBound(4, -0.5);
//        TS_ASSERT_THROWS_EQUALS(splits = sign.getCaseSplits(),
//                                const MarabouError &e,
//                                e.getCode(),
//                                MarabouError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT);
//
//        sign.unregisterAsWatcher(&tableau);
    }


//    void test_fix_negative() {
//        unsigned b = 1;
//        unsigned f = 4;
//
//        MockTableau tableau;
//
//        SignConstraint sign(b, f);
//
//        sign.registerAsWatcher(&tableau);
//
//        List<PiecewiseLinearCaseSplit> splits = sign.getCaseSplits();
//        TS_ASSERT_EQUALS(splits.size(), 2U);
//
//        sign.notifyUpperBound(4, 0.5);
//        TS_ASSERT_THROWS_EQUALS(splits = sign.getCaseSplits(),
//                                const MarabouError &e,
//                                e.getCode(),
//                                MarabouError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT);
//
//        sign.unregisterAsWatcher(&tableau);
//
//
//        sign = SignConstraint(b, f);
//
//        sign.registerAsWatcher(&tableau);
//
//        splits = sign.getCaseSplits();
//        TS_ASSERT_EQUALS(splits.size(), 2U);
//
//        sign.notifyUpperBound(1, -0.5);
//        TS_ASSERT_THROWS_EQUALS(splits = sign.getCaseSplits(),
//                                const MarabouError &e,
//                                e.getCode(),
//                                MarabouError::REQUESTED_CASE_SPLITS_FROM_FIXED_CONSTRAINT);
//
//        sign.unregisterAsWatcher(&tableau);
//
//    }
//
//
//    void test_constraint_phase_gets_fixed() {
//        unsigned b = 1;
//        unsigned f = 4;
//
//        MockTableau tableau;
//
//        // Upper bounds
//        {
//            SignConstraint sign(b, f);
//            TS_ASSERT(!sign.phaseFixed());
//            sign.notifyUpperBound(b, -0.1);
//            TS_ASSERT(sign.phaseFixed());
//        }
//
//        {
//            SignConstraint sign(b, f);
//            TS_ASSERT(!sign.phaseFixed());
//            sign.notifyUpperBound(b, -0.001);
//            TS_ASSERT(sign.phaseFixed());
//        }
//
//        {
//            SignConstraint sign(b, f);
//            TS_ASSERT(!sign.phaseFixed());
//            sign.notifyUpperBound(f, 1);
//            TS_ASSERT(!sign.phaseFixed());
//        }
//
//        {
//            SignConstraint sign(b, f);
//            TS_ASSERT(!sign.phaseFixed());
//            sign.notifyUpperBound(f, 0.5);
//            TS_ASSERT(sign.phaseFixed());
//        }
//
//        {
//            SignConstraint sign(b, f);
//            TS_ASSERT(!sign.phaseFixed());
//            sign.notifyUpperBound(b, 3.0);
//            TS_ASSERT(!sign.phaseFixed());
//        }
//
//        {
//            SignConstraint sign(b, f);
//            TS_ASSERT(!sign.phaseFixed());
//            sign.notifyUpperBound(b, 0);
//            TS_ASSERT(!sign.phaseFixed());
//        }
//
//        // Lower bounds
//        {
//            SignConstraint sign(b, f);
//            TS_ASSERT(!sign.phaseFixed());
//            sign.notifyLowerBound(b, -0.1);
//            TS_ASSERT(!sign.phaseFixed());
//        }
//
//        {
//            SignConstraint sign(b, f);
//            TS_ASSERT(!sign.phaseFixed());
//            sign.notifyLowerBound(b, 0);
//            TS_ASSERT(sign.phaseFixed());
//        }
//
//        {
//            SignConstraint sign(b, f);
//            TS_ASSERT(!sign.phaseFixed());
//            sign.notifyLowerBound(f, -1);
//            TS_ASSERT(!sign.phaseFixed());
//        }
//
//        {
//            SignConstraint sign(b, f);
//            TS_ASSERT(!sign.phaseFixed());
//            sign.notifyLowerBound(f, -0.6);
//            TS_ASSERT(sign.phaseFixed());
//        }
//
//        {
//            SignConstraint sign(b, f);
//            TS_ASSERT(!sign.phaseFixed());
//            sign.notifyLowerBound(b, 0.0);
//            TS_ASSERT(sign.phaseFixed());
//        }
//
//        {
//            SignConstraint sign(b, f);
//            TS_ASSERT(!sign.phaseFixed());
//            sign.notifyLowerBound(f, 6.0);
//            TS_ASSERT(sign.phaseFixed());
//        }
//
//        {
//            SignConstraint sign(b, f);
//            TS_ASSERT(!sign.phaseFixed());
//            sign.notifyLowerBound(f, 0.0);
//            TS_ASSERT(sign.phaseFixed());
//        }
//
//        {
//            SignConstraint sign(b, f);
//            TS_ASSERT(!sign.phaseFixed());
//            sign.notifyLowerBound(b, -2.0);
//            TS_ASSERT(!sign.phaseFixed());
//        }
//
//    }
//
//
//    void test_valid_split_sign_phase_fixed_to_positive() {
//        unsigned b = 1;
//        unsigned f = 4;
//
//        SignConstraint sign(b, f);
//
//        List<PiecewiseLinearConstraint::Fix> fixes;
//        List<PiecewiseLinearConstraint::Fix>::iterator it;
//
//        TS_ASSERT(!sign.phaseFixed());
//        TS_ASSERT_THROWS_NOTHING(sign.notifyLowerBound(b, -0.5));
//        TS_ASSERT(!sign.phaseFixed());
//
//        TS_ASSERT_THROWS_NOTHING(sign.notifyLowerBound(b, 0));
//        TS_ASSERT(sign.phaseFixed());
//
//        PiecewiseLinearCaseSplit split;
//        TS_ASSERT_THROWS_NOTHING(split = sign.getValidCaseSplit());
//
//        Equation activeEquation;
//
//        List<Tightening> bounds = split.getBoundTightenings();
//
//        TS_ASSERT_EQUALS(bounds.size(), 2U);
//        auto bound = bounds.begin();
//        Tightening bound1 = *bound;
//
//        TS_ASSERT_EQUALS(bound1._variable, b);
//        TS_ASSERT_EQUALS(bound1._type, Tightening::LB);
//        TS_ASSERT_EQUALS(bound1._value, 0.0);
//
//        ++bound;
//        Tightening bound2 = *bound;
//
//        TS_ASSERT_EQUALS(bound2._variable, f);
//        TS_ASSERT_EQUALS(bound2._value, 1);
//        TS_ASSERT_EQUALS(bound2._type, Tightening::LB);
//
//        auto equations = split.getEquations();
//        TS_ASSERT(equations.empty());
//    }
//
//
//    void test_valid_split_sign_phase_fixed_to_negative() {
//        unsigned b = 1;
//        unsigned f = 4;
//
//        SignConstraint sign(b, f);
//
//        List<PiecewiseLinearConstraint::Fix> fixes;
//        List<PiecewiseLinearConstraint::Fix>::iterator it;
//
//        TS_ASSERT(!sign.phaseFixed());
//
//        TS_ASSERT_THROWS_NOTHING(sign.notifyUpperBound(b, 0.5));
//        TS_ASSERT(!sign.phaseFixed());
//
//        TS_ASSERT_THROWS_NOTHING(sign.notifyUpperBound(b, -2));
//        TS_ASSERT(sign.phaseFixed());
//
//        PiecewiseLinearCaseSplit split;
//        TS_ASSERT_THROWS_NOTHING(split = sign.getValidCaseSplit());
//
//        Equation activeEquation;
//
//        List<Tightening> bounds = split.getBoundTightenings();
//
//        TS_ASSERT_EQUALS(bounds.size(), 2U);
//        auto bound = bounds.begin();
//        Tightening bound1 = *bound;
//
//        TS_ASSERT_EQUALS(bound1._variable, b);
//        TS_ASSERT_EQUALS(bound1._type, Tightening::UB);
//        TS_ASSERT_EQUALS(bound1._value, 0.0);
//
//        ++bound;
//        Tightening bound2 = *bound;
//
//        TS_ASSERT_EQUALS(bound2._variable, f);
//        TS_ASSERT_EQUALS(bound2._value, -1);
//        TS_ASSERT_EQUALS(bound2._type, Tightening::UB);
//
//        auto equations = split.getEquations();
//        TS_ASSERT(equations.empty());
//    }
//
//
//    void test_sign_duplicate_and_restore() {
//        SignConstraint *sign1 = new SignConstraint(4, 6);
//        sign1->setActiveConstraint(false);
//        sign1->notifyVariableValue(4, 1.0); // b
//        sign1->notifyVariableValue(6, 1.0); // f
//
//        sign1->notifyLowerBound(4, -8.0); // b
//        sign1->notifyUpperBound(4, 8.0);  // b
//
//        sign1->notifyLowerBound(6, -1); // f
//        sign1->notifyUpperBound(6, 1); // f
//
//        PiecewiseLinearConstraint *sign2 = sign1->duplicateConstraint();
//
//        sign1->notifyVariableValue(4, -2); // b
//        TS_ASSERT(!sign1->satisfied()); // f != sign(b)
//
//        sign1->notifyVariableValue(6, -1); // f
//        TS_ASSERT(sign1->satisfied()); // f = sign(b)
//
//
//        sign1->notifyVariableValue(6, 0.5); // f
//        TS_ASSERT(!sign1->satisfied()); // f != sign(b)
//
//
//        TS_ASSERT(!sign2->isActive());
//        TS_ASSERT(sign2->satisfied());
//
//        sign2->restoreState(sign1);
//        TS_ASSERT(!sign2->satisfied());
//
//        TS_ASSERT_THROWS_NOTHING(delete sign1);
//        TS_ASSERT_THROWS_NOTHING(delete sign2);
//    }
//
//    void test_eliminate_variable_active() {
//        unsigned b = 1;
//        unsigned f = 4;
//
//        MockTableau tableau;
//
//        SignConstraint sign(b, f);
//
//        sign.registerAsWatcher(&tableau);
//
//        TS_ASSERT(!sign.constraintObsolete());
//        TS_ASSERT_THROWS_NOTHING(sign.eliminateVariable(b, 5));
//        TS_ASSERT(sign.constraintObsolete());
//    }
//
//
//    void test_sign_entailed_tightenings() {
//        unsigned b = 1;
//        unsigned f = 4;
//
//        InputQuery dontCare;
//        dontCare.setNumberOfVariables(500);
//
//        SignConstraint sign(b, f);
//
//        sign.notifyLowerBound(b, -1);
//        sign.notifyUpperBound(b, 7);
//
//        sign.notifyLowerBound(f, -1);
//        sign.notifyUpperBound(f, 1);
//
//        List<Tightening> entailedTightenings;
//        sign.getEntailedTightenings(entailedTightenings);
//
//        // no phase fixed - only 2 trivial tightening -1<=f, f<=1
//        TS_ASSERT_EQUALS(entailedTightenings.size(), 2U);
//        TS_ASSERT(entailedTightenings.exists(Tightening(f, 1, Tightening::UB)));
//        TS_ASSERT(entailedTightenings.exists(Tightening(f, -1, Tightening::LB)));
//
//
//        entailedTightenings.clear();
//
//
//
//        sign.notifyLowerBound(b, -1);
//        sign.notifyUpperBound(b, -0.5); // the important line
//
//        sign.notifyLowerBound(f, -1);
//        sign.notifyUpperBound(f, 1);
//
//
//        sign.getEntailedTightenings(entailedTightenings);
//
//        // negative phase - because of b
//        TS_ASSERT_EQUALS(entailedTightenings.size(), 4U);
//        TS_ASSERT(entailedTightenings.exists(Tightening(f, 1, Tightening::UB)));
//        TS_ASSERT(entailedTightenings.exists(Tightening(f, -1, Tightening::LB)));
//        TS_ASSERT(entailedTightenings.exists(Tightening(f, -1, Tightening::UB)));
//        TS_ASSERT(entailedTightenings.exists(Tightening(b, 0, Tightening::UB)));
//
//        entailedTightenings.clear();
//
//
//        sign.notifyLowerBound(b, -1);
//        sign.notifyUpperBound(b, -0.5);
//        sign.notifyLowerBound(f, -1);
//        sign.notifyUpperBound(f, 0.5); // the important line
//
//        sign.getEntailedTightenings(entailedTightenings);
//
//        // negative phase - because of f
//        TS_ASSERT_EQUALS(entailedTightenings.size(), 4U);
//        TS_ASSERT(entailedTightenings.exists(Tightening(f, 1, Tightening::UB)));
//        TS_ASSERT(entailedTightenings.exists(Tightening(f, -1, Tightening::LB)));
//        TS_ASSERT(entailedTightenings.exists(Tightening(f, -1, Tightening::UB)));
//        TS_ASSERT(entailedTightenings.exists(Tightening(b, 0, Tightening::UB)));
//
//        entailedTightenings.clear();
//
//        sign.notifyLowerBound(b, 0);
//        sign.notifyUpperBound(b, 7);
//        sign.notifyLowerBound(f, -1);
//        sign.notifyUpperBound(f, 1);
//
//        sign.getEntailedTightenings(entailedTightenings);
//
//        // positive phase - because of b
//        TS_ASSERT_EQUALS(entailedTightenings.size(), 4U);
//        TS_ASSERT(entailedTightenings.exists(Tightening(f, 1, Tightening::UB)));
//        TS_ASSERT(entailedTightenings.exists(Tightening(f, -1, Tightening::LB)));
//        TS_ASSERT(entailedTightenings.exists(Tightening(f, 1, Tightening::LB)));
//        TS_ASSERT(entailedTightenings.exists(Tightening(b, 0, Tightening::LB)));
//
//
//        entailedTightenings.clear();
//
//
//        sign.notifyLowerBound(b, -5);
//        sign.notifyUpperBound(b, 5);
//        sign.notifyLowerBound(f, -0.5);
//        sign.notifyUpperBound(f, 1);
//
//        sign.getEntailedTightenings(entailedTightenings);
//
//        // positive phase - because of f
//        TS_ASSERT_EQUALS(entailedTightenings.size(), 4U);
//        TS_ASSERT(entailedTightenings.exists(Tightening(f, 1, Tightening::UB)));
//        TS_ASSERT(entailedTightenings.exists(Tightening(f, -1, Tightening::LB)));
//        TS_ASSERT(entailedTightenings.exists(Tightening(f, 1, Tightening::LB)));
//        TS_ASSERT(entailedTightenings.exists(Tightening(b, 0, Tightening::LB)));
//
//        entailedTightenings.clear();
//
//
//        unsigned b2 = 1;
//        unsigned f2 = 4;
//
//        InputQuery dontCare2;
//        dontCare.setNumberOfVariables(500);
//
//        SignConstraint sign2(b2, f2);
//
//        sign2.notifyLowerBound(b2, -1);
//        sign2.notifyUpperBound(b2, 1);
//
//        sign2.notifyLowerBound(f2, -1);
//        sign2.notifyUpperBound(f2, 1);
//
//        List<Tightening> entailedTightenings2;
//        sign2.getEntailedTightenings(entailedTightenings2);
//
//        // no phase fixed - only 2 trivial tightening -1<=f, f<=1
//        TS_ASSERT_EQUALS(entailedTightenings2.size(), 2U);
//        TS_ASSERT(entailedTightenings2.exists(Tightening(f2, 1, Tightening::UB)));
//        TS_ASSERT(entailedTightenings2.exists(Tightening(f2, -1, Tightening::LB)));
//
//        entailedTightenings2.clear();
//
//        // new case
//        sign2.notifyUpperBound(b, 0);
//
//        sign2.getEntailedTightenings(entailedTightenings2);
//
//        // no phase fixed - only 2 trivial tightening -1<=f, f<=1
//        TS_ASSERT_EQUALS(entailedTightenings2.size(), 2U);
//        TS_ASSERT(entailedTightenings2.exists(Tightening(f2, 1, Tightening::UB)));
//        TS_ASSERT(entailedTightenings2.exists(Tightening(f2, -1, Tightening::LB)));
//
//        entailedTightenings2.clear();
//
//
//
//    }
//
//
//    SignConstraint prepareSign(unsigned b, unsigned f, IConstraintBoundTightener *tightener) {
//        SignConstraint sign(b, f);
//
//        InputQuery dontCare;
////        dontCare.setNumberOfVariables();
//
//        sign.notifyLowerBound(b, -10);
//        sign.notifyUpperBound(b, 15);
//
//        sign.notifyLowerBound(f, -1);
//        sign.notifyUpperBound(f, 1);
//
//
//
//        sign.registerConstraintBoundTightener(tightener);
//
//        return sign;
//    }
//
//
//
//    void test_notify_bounds()  // TODO CHECK
//    {
//        unsigned b = 1;
//        unsigned f = 4;
//
//        MockConstraintBoundTightener tightener;
//        List<Tightening> tightenings;
//
//        tightener.getConstraintTightenings(tightenings);
//
//        SignConstraint sign = prepareSign( b, f, &tightener );
//
//
//
////        SignConstraint sign(b, f);
////        sign.notifyLowerBound(b, -5);
////        sign.notifyUpperBound(b, 5);
//
//
//        {
//            sign.notifyLowerBound(b, -5);
//            tightener.getConstraintTightenings(tightenings);
//            TS_ASSERT(tightenings.empty());
//
//            sign.notifyLowerBound( b, -2 );
//            tightener.getConstraintTightenings( tightenings );
//            TS_ASSERT( tightenings.empty() );
//
//            sign.notifyLowerBound( f, -3 );
//            tightener.getConstraintTightenings( tightenings );
//            TS_ASSERT( tightenings.empty() );
//
//
//            sign.notifyUpperBound( b, 20 );
//            tightener.getConstraintTightenings( tightenings );
//            TS_ASSERT( tightenings.empty() );
//
//            sign.notifyUpperBound( f, 23 );
//            tightener.getConstraintTightenings( tightenings );
//            TS_ASSERT( tightenings.empty() );
//        }
//
//
//        {
//            // Tighter upper bound for b/f that is positive
//            SignConstraint sign = prepareSign( b, f, &tightener );
//            sign.notifyUpperBound( b, 13 );
//            tightener.getConstraintTightenings( tightenings );
//            TS_ASSERT( tightenings.exists( Tightening( f, 13, Tightening::UB ) ) );
//
//            sign.notifyUpperBound( f, 12 );
//            tightener.getConstraintTightenings( tightenings );
//            TS_ASSERT( tightenings.exists( Tightening( b, 12, Tightening::UB ) ) );
//        }
//
//        {
//            // Tighter upper bound 0 for f
//            SignConstraint sign = prepareSign( b, f, &tightener );
//            sign.notifyUpperBound( f, 0 );
//            tightener.getConstraintTightenings( tightenings );
//
//            TS_ASSERT( tightenings.exists( Tightening( b, 0, Tightening::UB ) ) );
//        }
//
//        {
//            // Tighter negative upper bound for b
//            SignConstraint sign = prepareSign( b, f, &tightener );
//            sign.notifyUpperBound( b, -1 );
//            tightener.getConstraintTightenings( tightenings );
//
//            TS_ASSERT( tightenings.exists( Tightening( f, 0, Tightening::UB ) ) );
//        }
//
//
//
//    }
//

};









#endif //MARABOU_TEST_SIGNCONSTRAINT_H
