/*********************                                                        */
/*! \file PropertyParser.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Andrew Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#ifndef __PropertyParser_h__
#define __PropertyParser_h__

#include "Equation.h"
#include "IQuery.h"
#include "MString.h"
#include "Query.h"

/*
  This class reads a property from a text file, and stores the property's
  constraints within an IQuery object.
  Currently, properties can involve the input variables (X's) and the output
  variables (Y's) of a neural network.
*/
class PropertyParser
{
public:
    void parse( const String &propertyFilePath, IQuery &inputQuery );

private:
    void processSingleLine( const String &line, IQuery &inputQuery );
    Equation::EquationType extractRelationSymbol( const String &token );

    // This is created if needed to construct a network level reasoner to add
    // constraints on hidden neurons.
    std::unique_ptr<Query> _helperQuery = nullptr;
};

#endif // __PropertyParser_h__
