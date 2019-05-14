/*********************                                                        */
/*! \file GurobiManager.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifndef __GurobiManager_h__
#define __GurobiManager_h__

#include <iostream>
#include "ITableau.h"
#include "TableauRow.h"
#include "gurobi_c++.h"

class ITableau;
class TableauRow;

class GurobiManager
{
public:
    GurobiManager(ITableau &tableau);
    ~GurobiManager();

	void tightenBoundsOfVar(unsigned objectiveVar);
private:
    ITableau &_tableau;
};

#endif // __GurobiManager_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
