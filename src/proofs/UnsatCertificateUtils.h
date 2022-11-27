/*********************                                                        */
/*! \file UnsatCertificateUtils.h
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

#ifndef __UnsatCertificateUtils_h__
#define __UnsatCertificateUtils_h__

#include "FloatUtils.h"
#include "SparseMatrix.h"
#include "SparseUnsortedList.h"
#include "Vector.h"

class UNSATCertificateUtils
{
public:
    /*
      Use explanation to compute a bound (aka explained bound)
      Given a variable, an explanation, initial tableau and ground bounds.
    */
    static double computeBound( unsigned var,
                                bool isUpper,
                                const double *explanation,
                                const SparseMatrix *initialTableau,
                                const double *groundUpperBounds,
                                const double *groundLowerBounds,
                                unsigned numberOfRows,
                                unsigned numberOfVariables );

    /*
      Given a var, a tableau and a column vector, create a linear combination used to explain a bound
    */
    static void getExplanationRowCombination( unsigned var,
                                              const double *explanation,
                                              Vector<double> &explanationRowCombination,
                                              const SparseMatrix *initialTableau,
                                              unsigned numberOfVariables,
                                              unsigned numberOfRows );

    static double computeCombinationUpperBound( const double *explanation,
                                                const SparseMatrix *initialTableau,
                                                const double *groundUpperBounds,
                                                const double *groundLowerBounds,
                                                unsigned numberOfRows,
                                                unsigned numberOfVariables );
};

#endif //__UnsatCertificateUtils_h__
