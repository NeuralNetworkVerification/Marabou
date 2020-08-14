/*********************                                                        */
/*! \file LayerOwner.h
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

#ifndef __LayerOwner_h__
#define __LayerOwner_h__

#include "Tightening.h"

namespace NLR {

class Layer;

class LayerOwner
{
public:
    virtual ~LayerOwner() {}
    virtual const Layer *getLayer( unsigned index ) const = 0;
    virtual const ITableau *getTableau() const = 0;
    virtual unsigned getNumberOfLayers() const = 0;
    virtual void receiveTighterBound( Tightening tightening ) = 0;
};

} // namespace NLR

#endif // __LayerOwner_h__
