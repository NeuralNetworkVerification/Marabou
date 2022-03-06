/*********************                                                        */
/*! \file UnsatCertificateProblemConstraint.h
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

#ifndef __CertificateProblemConstraint_h__
#define __CertificateProblemConstraint_h__

#include "PiecewiseLinearConstraint.h"

/*
  A representation of a problem constraint, smaller than a PiecewiseLinearConstraint instance
*/
struct UnsatCertificateProblemConstraint
{
    PiecewiseLinearFunctionType _type;
    List<unsigned> _constraintVars;
    PhaseStatus _status;

    inline bool operator==( const UnsatCertificateProblemConstraint &other ) const
    {
        return _type == other._type && _constraintVars == other._constraintVars;
    }

friend class UnsatCertificateNode;
};

#endif //__CertificateProblemConstraint_h__