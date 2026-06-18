/*********************                                                        */
/*! \file PlcExplanation.h
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

#ifndef __PlcExplanation_h__
#define __PlcExplanation_h__

#include "PiecewiseLinearFunctionType.h"
#include "SparseUnsortedList.h"
#include "Tightening.h"
#include "Vector.h"

/*
  Contains all necessary info of a ground bound update during a run (i.e from ReLU phase-fixing)
*/
class PLCLemma
{
public:
    PLCLemma( const List<unsigned> &causingVars,
              unsigned affectedVar,
              double bound,
              Tightening::BoundType causingVarBound,
              Tightening::BoundType affectedVarBound,
              const Vector<SparseUnsortedList> &explanation,
              PiecewiseLinearFunctionType constraintType,
              double minTargetBound,
              unsigned id );

    ~PLCLemma();

    /*
     Getters for all fields
    */
    const List<unsigned> &getCausingVars() const;
    unsigned getAffectedVar() const;
    double getBound() const;
    Tightening::BoundType getCausingVarBound() const;
    Tightening::BoundType getAffectedVarBound() const;
    const List<SparseUnsortedList> &getExplanations() const;
    PiecewiseLinearFunctionType getConstraintType() const;
    bool getToCheck() const;
    double getMinTargetBound() const;
    unsigned getId() const;

    void setToCheck();

private:
    const List<unsigned> _causingVars;
    unsigned _affectedVar;
    double _bound;
    Tightening::BoundType _causingVarBound;
    Tightening::BoundType _affectedVarBound;
    List<SparseUnsortedList> _explanations;
    PiecewiseLinearFunctionType _constraintType;
    bool _toCheck;
    double _minTargetBound;
    unsigned _id;
};

#endif //__PlcExplanation_h__
