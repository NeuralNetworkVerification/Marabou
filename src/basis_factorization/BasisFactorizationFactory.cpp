/*********************                                                        */
/*! \file BasisFactorizationFactory.cpp
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

#include "BasisFactorizationError.h"
#include "BasisFactorizationFactory.h"
#include "ForrestTomlinFactorization.h"
#include "GlobalConfiguration.h"
#include "LUFactorization.h"
#include "SparseFTFactorization.h"
#include "SparseLUFactorization.h"

IBasisFactorization *BasisFactorizationFactory::createBasisFactorization( unsigned basisSize, const IBasisFactorization::BasisColumnOracle &basisColumnOracle )
{
    // LU
    if ( GlobalConfiguration::BASIS_FACTORIZATION_TYPE ==
         GlobalConfiguration::LU_FACTORIZATION )
        return new LUFactorization( basisSize, basisColumnOracle );

    // Sparse LU
    if ( GlobalConfiguration::BASIS_FACTORIZATION_TYPE ==
         GlobalConfiguration::SPARSE_LU_FACTORIZATION )
        return new SparseLUFactorization( basisSize, basisColumnOracle );

    // FT
    else if ( GlobalConfiguration::BASIS_FACTORIZATION_TYPE ==
              GlobalConfiguration::FORREST_TOMLIN_FACTORIZATION )
        return new ForrestTomlinFactorization( basisSize, basisColumnOracle );

    // Sparse FT
    else if ( GlobalConfiguration::BASIS_FACTORIZATION_TYPE ==
              GlobalConfiguration::SPARSE_FORREST_TOMLIN_FACTORIZATION )
        return new SparseFTFactorization( basisSize, basisColumnOracle );

    throw BasisFactorizationError( BasisFactorizationError::UNKNOWN_BASIS_FACTORIZATION_TYPE );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
