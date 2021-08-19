#include "GammaUnsat.h"

GammaUnsat::UnsatSequence::UnsatSequence() {}

GammaUnsat::GammaUnsat() {}

GammaUnsat::~GammaUnsat() {}

void GammaUnsat::addUnsatSequence( UnsatSequence unsatSeq ) { 
    unsatSequences.append(unsatSeq); 
}

List<GammaUnsat::UnsatSequence> GammaUnsat::getUnsatSequences( ) const {
    return unsatSequences;
}
