/*********************                                                        */
/*! \file PropertyParser.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __PropertyParser_h__
#define __PropertyParser_h__

#include "InputQuery.h"
#include "MString.h"

/*
  This class reads a property from a text file, and stores the property's
  constraints within an InputQuery object.
  Currently, properties can involve the input variables (X's) and the output
  variables (Y's) of a neural network.

  The format is as follows: in each line, var <= constant or var >= constant,
  e.g.

    X0 <= -5
    Y1 >= 3
*/
class PropertyParser
{
public:
    void parse( const String &propertyFilePath, InputQuery &inputQuery );

private:
    enum Sign {
        GEQ = 0,
        LEQ
    };

    void processSingleLine( const String &line, InputQuery &inputQuery );

    unsigned extractVariable( const String &token, const InputQuery &inputQuery );
    Sign extractSign( const String &token );
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
