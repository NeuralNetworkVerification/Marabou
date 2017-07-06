/*********************                                                        */
/*! \file IEngine.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __IEngine_h__
#define __IEngine_h__

class Equation;
class TableauState;

class IEngine
{
public:
    virtual ~IEngine() {};

    /*
      Methods for tightening lower/upper variable bounds.
    */
    virtual void tightenLowerBound( unsigned variable, double bound ) = 0;
    virtual void tightenUpperBound( unsigned variable, double bound ) = 0;

    /*
      Add a new equation to the tableau.
    */
    virtual void addNewEquation( const Equation &equation ) = 0;

    /*
      Methods for storing and restoring the tableau.
    */
    virtual void storeTableauState( TableauState &state ) const = 0;
    virtual void restoreTableauState( const TableauState &state ) = 0;
};

#endif // __IEngine_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
