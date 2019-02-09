/*********************                                                        */
/*! \file InfeasibleQueryException.h
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

#ifndef __InfeasibleQueryException_h__
#define __InfeasibleQueryException_h__

#include "Fact.h"
#include "List.h"

class InfeasibleQueryException
{
public:
    InfeasibleQueryException( const List<const Fact*>& explanations );
    List<const Fact*> getExplanations() const;
private:
    List<const Fact*> _explanations;
};

#endif // __InfeasibleQueryException_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
