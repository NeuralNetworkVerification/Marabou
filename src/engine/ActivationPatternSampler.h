/*********************                                                        */
/*! \file ActivationPatternSampler.h
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

#ifndef __ActivationPatternSampler_h__
#define __ActivationPatternSampler_h__

#include "List.h"
#include "QueryDivider.h"
#include "NetworkLevelReasoner.h"
#include "ReluConstraint.h"

#include <math.h>
#include <random>

// Takes in a convex input region and sample points in that region
class ActivationPatternSampler
{
public:



    struct InputRegion
    {
        Map<unsigned, double> _lowerBounds;
        Map<unsigned, double> _upperBounds;
    };

    ActivationPatternSampler( const List<unsigned> &inputVariables,
                              NetworkLevelReasoner *networkLevelReasoner );
    ~ActivationPatternSampler();

    /*
      Sample n points and store them in _samplePoints
      Return true if there are points in the inputRegion, false otherwise
    */
    bool samplePoints( const InputRegion &inputRegion, unsigned numberOfPoints );

    /*
      Compute the activation pattern of each points in _samplePoints and store them
      in _patterns
    */
    void computeActivationPatterns();

    /*
      Update _indexToPhaseStatusEstimate according to the current sampled points
    */
    void updatePhaseEstimate();

    /*
      Print the sampled points
    */
    void dumpSampledPoints();

    /*
      Print the activation pattern
    */
    void dumpActivationPatterns();

    // Getters
    unsigned getNumberOfPoints() const;
    const Vector<Vector<double>> &getSampledPoints() const;
    const Vector<NetworkLevelReasoner::ActivationPattern> &getActivationPatterns() const;
    const Map<NetworkLevelReasoner::Index, ReluConstraint:: PhaseStatus>
        &getIndexToPhaseStatusEstimate() const;

    void clearIndexToPhaseStatusEstimate();

private:

    /*
      Allocate memory for the sample points and activation patterns.
    */
    void allocateMemory();

    /*
      Free _samplePoints and _patterns.
    */
    void freeMemoryIfNeeded();

    // All input variables of the network
    const List<unsigned> _inputVariables;

    // NetworkLevelReasoner for get the Relu pattern
    NetworkLevelReasoner *_networkLevelReasoner;

    unsigned _numberOfInputVariables;
    unsigned _numberOfPoints;
    Vector<Vector<double>> _samplePoints;
    Vector<NetworkLevelReasoner::ActivationPattern> _patterns;
    Vector<std::uniform_real_distribution<double>> _samplers;
    Map<NetworkLevelReasoner::Index, ReluConstraint:: PhaseStatus> _indexToPhaseStatusEstimate;
};

#endif // __ActivationVarianceSampler_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
