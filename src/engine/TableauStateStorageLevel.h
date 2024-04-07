/*********************                                                        */
/*! \file TableauStateStorageLevel.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifndef __TableauStateStorageLevel_h__
#define __TableauStateStorageLevel_h__

enum class TableauStateStorageLevel {
    STORE_NONE = 0,
    STORE_BOUNDS_ONLY = 1,
    STORE_ENTIRE_TABLEAU_STATE = 2,
};

#endif // __TableauStateStorageLevel_h__
