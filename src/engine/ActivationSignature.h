/*********************                                                        */
/*! \file ActivationSignature.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __ActivationSignature_h__
#define __ActivationSignature_h__

class PiecewiseLinearConstraint;

class ActivationSignature
{
public:
    //    Map<PiecewiseLinearConstraint *, bool> _signature;
    List<bool> _signature;
};

#endif // __ActivationSignature_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
