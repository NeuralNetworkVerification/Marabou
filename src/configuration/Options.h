/*********************                                                        */
/*! \file Options.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __Options_h__
#define __Options_h__

#include "MString.h"
#include "Map.h"
#include "boost/program_options.hpp"

/*
  A singleton class that contains all the options and their values.
*/
class Options
{
public:
    /*
      The singleton instance
    */
    static Options *get();

private:
    /*
      Disable default constructor and copy constructor
    */
    Options() {};
    Options( const Options & ) {};
};

#endif // __Options_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
