/********************                                                        */
/*! \file BasisFactorizationFactory.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "BasisFactorizationError.h"
#include "BasisFactorizationFactory.h"
#include "ForrestTomlinFactorization.h"
#include "GlobalConfiguration.h"
#include "LUFactorization.h"
#include "SparseLUFactorization.h"

IBasisFactorization *BasisFactorizationFactory::createBasisFactorization( unsigned basisSize, const IBasisFactorization::BasisColumnOracle &basisColumnOracle )
{
    if ( GlobalConfiguration::BASIS_FACTORIZATION_TYPE ==
         GlobalConfiguration::LU_FACTORIZATION )
        return new LUFactorization( basisSize, basisColumnOracle );
    if ( GlobalConfiguration::BASIS_FACTORIZATION_TYPE ==
         GlobalConfiguration::SPARSE_LU_FACTORIZATION )
        return new SparseLUFactorization( basisSize, basisColumnOracle );
    else if ( GlobalConfiguration::BASIS_FACTORIZATION_TYPE ==
              GlobalConfiguration::FORREST_TOMLIN_FACTORIZATION )
        return new ForrestTomlinFactorization( basisSize, basisColumnOracle );

    throw BasisFactorizationError( BasisFactorizationError::UNKNOWN_BASIS_FACTORIZATION_TYPE );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
