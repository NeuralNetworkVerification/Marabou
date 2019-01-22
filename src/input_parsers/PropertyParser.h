/*********************                                                        */
/*! \file PropertyParser.h
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

#ifndef __PropertyParser_h__
#define __PropertyParser_h__

#include "Equation.h"
#include "InputQuery.h"
#include "MString.h"

/*
  This class reads a property from a text file, and stores the property's
  constraints within an InputQuery object.
  Currently, properties can involve the input variables (X's) and the output
  variables (Y's) of a neural network.
*/
class PropertyParser
{
public:
    void parse( const String &propertyFilePath, InputQuery &inputQuery );

private:
    void processSingleLine( const String &line, InputQuery &inputQuery );
    Equation::EquationType extractSign( const String &token );
    double extractScalar( const String &token );
};

#endif // __PropertyParser_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
