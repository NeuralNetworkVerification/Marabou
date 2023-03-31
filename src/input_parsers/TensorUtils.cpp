/*********************                                                        */
/*! \file TensorUtils.cpp
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

#include "TensorUtils.h"
#include "Debug.h"
#include <math.h>

TensorIndices packIndex( TensorShape shape, int flatIndex )
{
    ASSERT ( flatIndex < tensorSize( shape ) );

    TensorIndices indices;
    int currentIndex = flatIndex;
    for ( int i = shape.size() - 1; i >= 0; i-- )
    {
        int dimension = shape[i];
        int index = currentIndex % dimension;
        currentIndex = (currentIndex - index) / dimension;
        indices.insertHead( index );
    }
    return indices;
}

int unpackIndex ( TensorShape shape, TensorIndices indices )
{
    ASSERT( shape.size() == indices.size() );

    int sizeSoFar = 1;
    int index = 0;
    for ( size_t i = 0; i < shape.size(); i++ )
    {
        ASSERT ( indices[i] <= shape[i] );
        index += sizeSoFar*indices[i];
        sizeSoFar *= shape[i];
    }
    ASSERT( tensorSize ( shape ) == sizeSoFar );
    return index;
}

int tensorSize( TensorShape shape )
{
    int size = 1;
    for ( int dimSize : shape )
    {
        size *= dimSize;
    }
    return size;
}

// See https://github.com/onnx/onnx/blob/main/docs/Broadcasting.md#multidirectional-broadcasting
TensorShape getMultidirectionalBroadcastShape( TensorShape shape1, TensorShape shape2 )
{
    TensorShape output;
    auto it1 = shape1.rbegin();
    auto it2 = shape2.rbegin();
    while ( it1 != shape1.rend() || it2 != shape2.rend() )
    {
        int d1 = it1 == shape1.rend() ? 1 : *(it1++);
        int d2 = it2 == shape2.rend() ? 1 : *(it2++);

        ASSERT ( d1 == d2 || d2 == 1 || d1 == 1 );

        int d;
        if ( d1 == d2 || d2 == 1 )
        {
            d = d1;
        }
        else
        {
            d = d2;
        }

        output.insertHead( d );
    }
    return output;
}

TensorIndices broadcastIndex ( TensorShape currentShape, TensorIndices broadcastIndices, TensorShape broadcastShape )
{
    ASSERT ( broadcastIndices.size() == broadcastShape.size() );

    int dimOffset = broadcastShape.size() - currentShape.size();
    TensorIndices result;
    for ( size_t i = dimOffset; i < broadcastShape.size(); i++ )
    {
        int cd = currentShape[i - dimOffset];
        int bd = broadcastShape[i];

        ASSERT ( cd == 1 || cd == bd );

        if ( cd == bd )
        {
            result.append( broadcastIndices[i] );
        }
        else
        {
            result.append ( 0 );
        }
    }
    return result;
}

Padding::Padding ( int padFront, int padBack )
    : padFront( padFront )
    , padBack( padBack )
{}

Padding calculatePaddingNeeded( int inputSize, int filterSize, int stride, bool padFrontPreferentially )
{
    int overrun = inputSize % stride;
    int paddingNeeded;
    if ( overrun == 0 )
    {
        paddingNeeded = std::max( filterSize - stride, 0 );
    }
    else
    {
        paddingNeeded = std::max( filterSize - overrun, 0 );
    }

    if ( paddingNeeded % 2 == 0 )
    {
        int halfPadding = paddingNeeded/2;
        return Padding( halfPadding, halfPadding );
    }
    else
    {
        float halfPadding = paddingNeeded/2.0;
        int biggerPad = (int) std::ceil(halfPadding);
        int smallerPad = (int) std::floor(halfPadding);
        if ( padFrontPreferentially )
        {
            return Padding ( biggerPad, smallerPad );
        }
        else
        {
            return Padding ( smallerPad, biggerPad );
        }
    }
}