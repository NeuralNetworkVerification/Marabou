/*********************                                                        */
/*! \file NonlinearFunctionType.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Teruhiro Tagomori, Andrew Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifndef __NonlinearFunctionType_h__
#define __NonlinearFunctionType_h__

enum NonlinearFunctionType {
    SIGMOID = 0,
    SOFTMAX = 1,
    BILINEAR = 2,
    ROUND = 3,
};

#endif // __NonlinearFunctionType_h__
