/*********************                                                        */
/*! \file BasisFactorizationFactory.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __BasisFactorizationFactory_h__
#define __BasisFactorizationFactory_h__

#include "IBasisFactorization.h"

class BasisFactorizationFactory
{
public:
    static IBasisFactorization *createBasisFactorization( unsigned basisSize );
};

#endif // __BasisFactorizationFactory_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
