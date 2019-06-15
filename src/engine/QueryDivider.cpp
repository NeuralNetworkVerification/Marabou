/*********************                                                        */
/*! \file QueryDivider.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include <QueryDivider.h>

void QueryDivider::bisectInputRegion( const InputRegion &inputRegion,
                                      unsigned dimensionToBisect,
                                      List<InputRegion> &inputRegions )
{
    InputRegion inputRegion1;
    InputRegion inputRegion2;

    double mid = ( inputRegion._lowerBounds[dimensionToBisect] +
                   inputRegion._upperBounds[dimensionToBisect] ) / 2;

    inputRegion1 = inputRegion;
    inputRegion1._upperBounds[dimensionToBisect] = mid;
    inputRegion2 = inputRegion;
    inputRegion2._lowerBounds[dimensionToBisect] = mid;

    inputRegions.append( inputRegion1 );
    inputRegions.append( inputRegion2 );
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
