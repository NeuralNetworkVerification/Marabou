/*********************                                                        */
/*! \file AbstractionManager.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __AbstractionManager_h__
#define __AbstractionManager_h__

#include "InputQuery.h"
#include "Simulator.h"

class AbstractionManager
{
public:
    bool run( InputQuery &inputQuery );

private:
    InputQuery _originalQuery;
    InputQuery _copyOfOriginalQuery;
    InputQuery _abstractQuery;

    Map<unsigned, double> _satAssignment;

    Simulator _simulator;

    void storeOriginalQuery( InputQuery &inputQuery );
    void createInitialAbstraction();
    bool checkSatisfiability();
    bool spurious();
    void refineAbstraction();
    void extractSatAssignment( InputQuery &inputQuery );
    void runSimulations();

    void log( const String &message ) const;
};

#endif // __AbstractionManager_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
