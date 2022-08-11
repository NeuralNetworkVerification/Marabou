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
#include "Vector.h"

class UNSATCertificateUtils
{
public:
    constexpr static const double CERTIFICATION_TOLERANCE = 0.0025;

    /*
      Use explanation to compute a bound (aka explained bound)
      Given a variable, an explanation, initial tableau and ground bounds.
    */
    static double computeBound( unsigned var,
                                bool isUpper,
                                const double *explanation,
                                const Vector<Vector<double>> &initialTableau,
                                const Vector<double> &groundUpperBounds,
                                const Vector<double> &groundLowerBounds );

    /*
      Given a var, a tableau and a column vector, create a linear combination used to explain a bound
    */
    static void getExplanationRowCombination( unsigned var,
                                              Vector<double> &explanationRowCombination,
                                              const double *explanation,
                                              const Vector<Vector<double>> &initialTableau );
};

#endif //__UnsatCertificateUtils_h__
