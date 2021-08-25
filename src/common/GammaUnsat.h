/*********************                                                        */
/*! \file Pair.h
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

#ifndef __GammaUnsat_h__
#define __GammaUnsat_h__

#include "List.h"
#include "PLCaseSplitRawData.h"

class GammaUnsat
{
public:
  
    static GammaUnsat readFromFile( std::string const& path );
    void saveToFile( std::string const& path ) const;

    struct UnsatSequence
    {
        UnsatSequence();
        List<PLCaseSplitRawData> activations;
    };

    GammaUnsat();
    ~GammaUnsat();

    void addUnsatSequence( UnsatSequence unsatSeq );
    List<UnsatSequence> getUnsatSequences( ) const;
private:
    List<UnsatSequence> unsatSequences;
};

#endif // __GammaUnsat_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
