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

#include "Debug.h"
#include "Vector.h"
#include "MString.h"

/**
 * @brief Represents the dimensions of a tensor, e.g. [10,3,2] is a 3D tensor of dimensions 10 x 3 x 2.
 */
typedef Vector<uint> TensorShape;

/**
 * @brief An single index into one-dimension of a tensor.
 */
typedef uint TensorIndex;

/**
 * @brief A n-dimensional index into an n-dimensional tensor,
 * e.g. if the tensor X is indexed into at [3,1,2] than the element X[3][1][2]
 * is returned.
 */
typedef Vector<TensorIndex> TensorIndices;

/**
 * @brief A packed representation of an n-dimension index into an n-dimensional tensor.
 * Requires the tensor shape to be known in order to interpret it.
 */
typedef TensorIndex PackedTensorIndices;

typedef Vector<uint> Permutation;

TensorIndices unpackIndex( TensorShape shape, PackedTensorIndices packedIndex );

PackedTensorIndices packIndex ( TensorShape shape, TensorIndices indices );

template <typename T>
T tensorLookup( Vector<T> tensor, TensorShape shape, TensorIndices indices)
{
    return tensor[ packIndex( shape, indices ) ];
}

template <typename T>
Vector<T> transposeVector ( Vector<T> values, Permutation permutation )
{
    Vector<T> result;
    for ( uint i : permutation )
    {
        ASSERT( i < values.size() );
        result.append(values[i]);
    }
    return result;
}

template <typename T>
Vector<T> transposeTensor( Vector<T> tensor, TensorShape shape, Permutation permutation )
{
    // NOTE this implementation is *very* inefficient. Eventually we might want to
    // switch to a similar implementation as NumPy arrays with internal strides etc.
    ASSERT ( shape.size() == permutation.size() );
    TensorShape transposedShape = transposeVector( shape, permutation );
    Vector<T> result;
    for ( PackedTensorIndices rawOutputIndex = 0; rawOutputIndex < tensor.size(); rawOutputIndex++ )
    {
        TensorIndices outputIndex = unpackIndex( transposedShape, rawOutputIndex );
        TensorIndices inputIndex = transposeVector( outputIndex, permutation );
        PackedTensorIndices rawInputIndex = packIndex( shape, inputIndex );
        result.append( tensor[rawInputIndex] );
    }
    return result;
}

uint tensorSize( TensorShape shape );

// See https://github.com/onnx/onnx/blob/main/docs/Broadcasting.md#multidirectional-broadcasting
TensorShape getMultidirectionalBroadcastShape( TensorShape shape1, TensorShape shape2 );

TensorIndices broadcastIndex ( TensorShape currentShape, TensorShape broadcastShape, TensorIndices broadcastIndices );

Permutation reversePermutation( uint size );

struct Padding
{
    public:
      int padFront;
      int padBack;

      Padding ( int padFront, int padBack );
};

Padding calculatePaddingNeeded( int inputSize, int filterSize, int stride, bool padFrontPreferentially );

#endif // __TensorUtils_h__