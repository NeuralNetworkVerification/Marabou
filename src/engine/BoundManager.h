/*********************                                                        */
/*! \file BoundManager.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu, Aleksandar Zeljic
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#ifndef __BoundManager_h__
#define __BoundManager_h__

#include "context/cdlist.h"
#include "context/context.h"

#include "Vector.h"

class BoundManager
{
public:
    BoundManager( CVC4::context::Context &ctx );
    ~BoundManager();

    void initialize( unsigned numberOfVariables );
    bool updateLowerBound( unsigned variable, double value );
    bool updateUpperBound( unsigned variable, double value );

    double getLowerBound( unsigned variable );
    double getUpperBound( unsigned variable );

    unsigned getSize();

private:

    CVC4::context::Context &_context;
    unsigned _size;
    // For now, assume variable number is the vector index
    Vector<CVC4::context::CDList<double> *> _lowerBounds;
    Vector<CVC4::context::CDList<double> *> _upperBounds;

};


#endif // __BoundManager_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
