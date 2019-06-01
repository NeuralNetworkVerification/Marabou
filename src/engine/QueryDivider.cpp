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

void QueryDivider::bisectInputRegion(InputRegion& inputRegion, unsigned
                                     dimensionToBisect, std::vector<InputRegion>&
                                     inputRegions){
  InputRegion inputRegion1;
  InputRegion inputRegion2;
  double mid = (inputRegion.lowerbounds[dimensionToBisect] +
                inputRegion.upperbounds[dimensionToBisect]) / 2;
  inputRegion1 = inputRegion;
  inputRegion1.upperbounds[dimensionToBisect] = mid;
  inputRegion2 = inputRegion;
  inputRegion2.lowerbounds[dimensionToBisect] = mid;

  inputRegions.push_back(inputRegion1);
  inputRegions.push_back(inputRegion2);
}
