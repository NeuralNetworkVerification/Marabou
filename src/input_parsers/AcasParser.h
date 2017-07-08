/*********************                                                        */
/*! \file AcasParser.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __AcasParser_h__
#define __AcasParser_h__

#include "AcasNeuralNetwork.h"
#include "InputQuery.h"
#include "String.h"

class AcasParser
{
public:
    AcasParser( const String &path );

    void generateQuery( InputQuery &inputQuery );

private:
    AcasNeuralNetwork _acasNeuralNetwork;
};

#endif // __AcasParser_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
