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

#include "context/cdo.h"
#include "context/context.h"
#include "Vector.h"

class Tableau;

class BoundManager
{
public:
    BoundManager( CVC4::context::Context &ctx );
    ~BoundManager();


    /*
     * Registers a new variable, grows the BoundManager size and bound vectors,
     * initializes bounds to +/-inf, and returns the new index as the new variable.
     */
    unsigned registerNewVariable( );

    void initialize( unsigned numberOfVariables );

    bool setLowerBound( unsigned variable, double value );
    bool setUpperBound( unsigned variable, double value );

    double getLowerBound( unsigned variable );
    double getUpperBound( unsigned variable );

    unsigned getSize(); //TODO: Rename to getNumberOfVariables

    void registerTableauReference( Tableau *tableau );

private:

    CVC4::context::Context &_context;
    Tableau *_tableau;
    unsigned _size; // TODO: Make context sensitive, to account for growing 
    // For now, assume variable number is the vector index
    Vector<CVC4::context::CDO<double> *> _lowerBounds;
    Vector<CVC4::context::CDO<double> *> _upperBounds;

};


#endif // __BoundManager_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
