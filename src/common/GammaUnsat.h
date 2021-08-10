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

#include <List.h>
#include <Map.h>

class GammaUnsat
{
public:
    enum ActivationType {
        INACTIVE = -1,
        ACTIVE = 1,
    };

    struct UnsatSequence
    {
        UnsatSequence() {}
        Map<unsigned, ActivationType> activations;
    };

    GammaUnsat() {}
    ~GammaUnsat() {}

    void addUnsatSequence( UnsatSequence unsatSeq ) { unsatSequences.append(unsatSeq); }
    List<UnsatSequence> getUnsatSequences( ) { return unsatSequences; }
    void reset() { unsatSequences = List<UnsatSequence>(); }
    void dump() {}
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
