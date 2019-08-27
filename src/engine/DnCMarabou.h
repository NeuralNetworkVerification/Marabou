/*********************                                                        */
/*! \file DnCMarabou.h
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

#ifndef __DnCMarabou_h__
#define __DnCMarabou_h__

#include "DnCManager.h"
#include "Options.h"
#include "InputQuery.h"

class DnCMarabou
{
public:
    DnCMarabou();
    ~DnCMarabou();

    /*
      Entry point of this class. The inputQuery is either supplied
      externally (e.g., through the Python interface), or is extracted
      according to the option values (e.g., if run from the command
      line)
    */
    void run();
    void run( InputQuery &inputQuery );

    /*
      Return the exit code of the input query that was run
    */
    InputQuery::ExitCode getExitCode() const;

    /*
      Return the Engine Statistics
    */
    const Statistics *getStatistics() const;

private:
    std::unique_ptr<DnCManager> _dncManager;

    InputQuery *_inputQuery;

    /*
      Indicates whether the input query should be freed() when the object dies
    */
    bool _allocatedInputQuery;

    /*
      Display the results
    */
    void displayResults( unsigned long long microSecondsElapsed ) const;
};

#endif // __DnCMarabou_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
