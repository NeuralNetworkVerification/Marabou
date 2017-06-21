/*********************                                                        */
/*! \file ITableau.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __ITableau_h__
#define __ITableau_h__

#include "Set.h"

class ITableau
{
public:
    virtual ~ITableau() {};

    virtual void setDimensions( unsigned m, unsigned n ) = 0;
    virtual void setEntryValue( unsigned row, unsigned column, double value ) = 0;
    virtual void setRightHandSide( const double *b ) = 0;
    virtual void setRightHandSide( unsigned index, double value ) = 0;
    virtual void markAsBasic( unsigned variable ) = 0;
    virtual void initializeTableau() = 0;
    virtual double getValue( unsigned variable ) = 0;
    virtual void setLowerBound( unsigned variable, double value ) = 0;
    virtual void setUpperBound( unsigned variable, double value ) = 0;
    virtual unsigned getBasicStatus( unsigned basic ) = 0;
    virtual bool existsBasicOutOfBounds() const = 0;
    virtual void computeBasicStatus() = 0;
    virtual void computeBasicStatus( unsigned basic ) = 0;
    virtual bool pickEnteringVariable() = 0;
    virtual bool eligibleForEntry( unsigned nonBasic ) = 0;
    virtual unsigned getEnteringVariable() const = 0;
    virtual void pickLeavingVariable() = 0;
    virtual void pickLeavingVariable( double *d ) = 0;
    virtual unsigned getLeavingVariable() const = 0;
    virtual double getChangeRatio() const = 0;
    virtual void performPivot() = 0;
    virtual double ratioConstraintPerBasic( unsigned basicIndex, double coefficient, bool decrease ) = 0;
    virtual bool isBasic( unsigned variable ) const = 0;
    virtual void computeCostFunction() = 0;
    virtual const double *getCostFunction() = 0;
    virtual void dumpCostFunction() const = 0;
    virtual void computeD() = 0;
    virtual void computeAssignment() = 0;
    virtual void dump() const = 0;
    virtual void dumpAssignment() = 0;
};

#endif // __ITableau_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
