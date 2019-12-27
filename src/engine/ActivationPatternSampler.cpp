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
    , _numberOfInputVariables( inputVariables.size() )
{
}

bool ActivationPatternSampler::samplePoints( const InputRegion &inputRegion,
                                              unsigned numberOfPoints )
{
    _samplers.clear();
    std::default_random_engine randomEngine( 0 );
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
    for ( auto &point : _samplePoints )
    {
        NetworkLevelReasoner::ActivationPattern pattern;
        _networkLevelReasoner->getActivationPattern( point,
                                                     pattern );
        _patterns.append( pattern );
    }
}

void ActivationPatternSampler::updatePhaseEstimate()
{
    for ( const auto &pattern :  _patterns )
    {
        for ( const auto &entry : pattern )
        {
            auto id = entry.first;
            auto currentPhase = entry.second == 1 ?
                ReluConstraint::PHASE_ACTIVE : ReluConstraint::PHASE_INACTIVE;
            if ( _idToPhaseStatusEstimate.exists( id ) )
            {
                auto prevPhase = _idToPhaseStatusEstimate[id];
                if ( prevPhase == ReluConstraint::PHASE_NOT_FIXED )
                    continue;
                else if ( prevPhase != currentPhase )
                    _idToPhaseStatusEstimate[id] = ReluConstraint::PHASE_NOT_FIXED;
            }
            else
            {
                _idToPhaseStatusEstimate[id] = currentPhase;
            }
        }
    }
}

void ActivationPatternSampler::dumpSampledPoints()
{
    for ( auto &point : _samplePoints )
    {
        for ( const auto &val : point )
            std::cout << val << " ";
        std::cout << std::endl;
    }
}

void ActivationPatternSampler::dumpActivationPatterns()
{
    for ( const auto& pattern : _patterns )
    {
        for ( auto &act :  pattern )
            std::cout << act.second << " ";
        std::cout << std::endl;
    }
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

const Map<unsigned, ReluConstraint:: PhaseStatus>
&ActivationPatternSampler::getIdToPhaseStatusEstimate() const
{
    return _idToPhaseStatusEstimate;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
