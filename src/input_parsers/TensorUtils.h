/*********************                                                        */
/*! \file TensorUtils.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Matthew Daggitt
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** Utilities for working with tensors while parsing networks.
 **/

#ifndef __TensorUtils_h__
#define __TensorUtils_h__

#include "Vector.h"

typedef Vector<int> TensorShape;
typedef Vector<int> TensorIndices;

TensorIndices packIndex( TensorShape shape, int flatIndex );

int unpackIndex ( TensorShape shape, TensorIndices indices );

template <typename T>
T lookup2D( Vector<T> array, int i1, int i2, [[maybe_unused]] int rows, int cols )
{
    ASSERT( array.size() == (uint) (rows * cols) );
    return array[i1 * cols + i2];
}

template <typename T>
T tensorLookup( Vector<T> tensor, TensorShape shape, TensorIndices indices)
{
    return tensor[ unpackIndex( shape, indices ) ];
}

int tensorSize( TensorShape shape );

// See https://github.com/onnx/onnx/blob/main/docs/Broadcasting.md#multidirectional-broadcasting
TensorShape getMultidirectionalBroadcastShape( TensorShape shape1, TensorShape shape2 );

TensorIndices broadcastIndex ( TensorShape currentShape, TensorIndices broadcastIndices, TensorShape broadcastShape );


struct Padding
{
    public:
      int padFront;
      int padBack;

      Padding ( int padFront, int padBack );
};

Padding calculatePaddingNeeded( int inputSize, int filterSize, int stride, bool padFrontPreferentially );

#endif // __TensorUtils_h__