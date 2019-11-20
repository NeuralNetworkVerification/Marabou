/*********************                                                        */
/*! \file ActivationPatternSampler.cpp
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

#include "Debug.h"
#include "ActivationPatternSampler.h"
#include "MarabouError.h"
#include "MStringf.h"
#include "PiecewiseLinearCaseSplit.h"

ActivationPatternSampler::ActivationPatternSampler( const List<unsigned>
                                                    &inputVariables,
                                                    NetworkLevelReasoner
                                                    *networkLevelReasoner )
    : _inputVariables( inputVariables )
    , _networkLevelReasoner( networkLevelReasoner )
    , __numberOfInputVariables( inputVariables.size() )
{
}

ActivationPatternSampler::~ActivationPatternSampler()
{
    freeMemoryIfNeeded();
    // The sampler does not own the networkLeverReasoner
}

bool ActivationPatternSampler::samplePoints( const InputRegion &inputRegion,
                                              unsigned numberOfPoints )
{
    _samplers.clear();
    std::default_random_engine randomEngine( 0 );
    unsigned varIndex = 0;
    for ( const auto &variable : _inputVariables )
    {
        float lowerBound = inputRegion._lowerBounds[variable];
        float upperBound = inputRegion._upperBounds[variable];
        if ( lowerBound > upperBound )
            return false;
        _samplers.append( std::uniform_real_distribution<double>
                          ( lowerBound, upperBound ) );
    }
    for ( unsigned i = 0; i < numberOfPoints; ++i )
    {
        Vector<double> point;
        for ( unsigned varIndex = 0; varIndex < _numberOfInputVariables;
              ++varIndex )
            point.append( _samplers[varIndex]( randomEngine ) );
        _samplePoints.append( point );
    }
    return true;
}

void ActivationPatternSampler::computeActivationPatterns()
{
    _patterns.clear();
    NetworkLevelReasoner::ActivationPattern pattern;
    for ( unsigned i = 0; i < _numberOfPoints; ++i )
        _networkLevelReasoner->getActivationPattern( _samplePoints[i],
                                                     pattern );
    _patterns.append( pattern );
}

void ActivationPatternSampler::updatePhaseEstimate()
{
    for ( unsigned i = 0; i < _numberOfPoints; ++i )
    {
        for ( const auto &entry : _patterns[i] )
        {
            auto index = entry.first;
            auto currentPhase = entry.second > 0 ?
                ReluConstraint::PHASE_ACTIVE : ReluConstraint::PHASE_INACTIVE;
            if ( _indexToPhaseStatusEstimate.exists( index ) )
            {
                auto prevPhase = _indexToPhaseStatusEstimate[index];
                if ( prevPhase == ReluConstraint::PHASE_NOT_FIXED )
                    continue;
                else if ( prevPhase != currentPhase )
                    _indexToPhaseStatusEstimate[index] = ReluConstraint::PHASE_NOT_FIXED;
            }
            else
                _indexToPhaseStatusEstimate[index] = currentPhase;
        }
    }
}

void ActivationPatternSampler::dumpSampledPoints()
{
    for ( unsigned i = 0; i < _numberOfPoints; ++i )
    {
        for ( unsigned j = 0; j < _inputVariables.size(); ++j )
            std::cout << _samplePoints[i][j] << " ";
        std::cout << std::endl;
    }
}

void ActivationPatternSampler::dumpActivationPatterns()
{
    for ( unsigned i = 0; i < _numberOfPoints; ++i )
    {
        for ( auto &act : *( _patterns[i] ) )
            std::cout << act.second << " ";
        std::cout << std::endl;
    }
}

unsigned ActivationPatternSampler::getNumberOfPoints() const
{
    return _numberOfPoints;
}

const Vector<Vector<double>> &ActivationPatternSampler::getSampledPoints() const
{
    return _samplePoints;
}

const Vector<NetworkLevelReasoner::ActivationPattern> &ActivationPatternSampler::
getActivationPatterns() const
{
    return _patterns;
}

const Map<NetworkLevelReasoner::Index, ReluConstraint:: PhaseStatus>
&ActivationPatternSampler::getIndexToPhaseStatusEstimate() const
{
    return _indexToPhaseStatusEstimate;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
