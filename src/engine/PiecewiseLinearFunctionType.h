/*********************                                                        */
/*! \file Engine.h
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

#ifndef __PiecewiseLinearFunctionType_h__
#define __PiecewiseLinearFunctionType_h__

enum PiecewiseLinearFunctionType {
    RELU = 0,
    ABSOLUTE_VALUE = 1,
    MAX = 2,
    DISJUNCTION = 3,
};

#endif // __PiecewiseLinearFunctionType_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
