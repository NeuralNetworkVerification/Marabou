/*********************                                                        */
/*! \file SODDivider.h
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

#include "LargestIntervalDivider.h"

LargestIntervalDivider::LargestIntervalDivider(List<unsigned>& inputVariables,
                                               double timeout_factor)
  : _timeout_factor (timeout_factor)

{
  for (const auto& var : inputVariables){
    _inputVariables.push_back(var);
  }
}

void LargestIntervalDivider::createSubQueries( unsigned num_new_subqueries,
                                               SubQuery& previousSubquery,
                                               SubQueries& subqueries ){
  unsigned num_bisects = (unsigned) log2(num_new_subqueries);

  // Get the query id, previous case split, and the previous timeout
  const std::string query_id_prefix = *( std::get<0>( previousSubquery ) );
  PiecewiseLinearCaseSplit previousSplit = *std::get<1>( previousSubquery );
  unsigned timeoutInSeconds = std::get<2>( previousSubquery );


  std::vector<InputRegion> inputRegions;

  // Create the first input region from the previous case split
  InputRegion region;
  List<Tightening> bounds = previousSplit.getBoundTightenings();
  for ( auto& bound : bounds ){
    if ( bound._type == Tightening::LB ){
      region.lowerbounds[ bound._variable ] = bound._value;
    } else {
      ASSERT( bound._type == Tightening::UB );
      region.upperbounds[ bound._variable ] = bound._value;
    }
  }
  inputRegions.push_back( region );

  // Repeatedly bisect the dimension with the largest interval
  for ( unsigned i = 0; i < num_bisects; i++ ){
    std::vector<InputRegion> newInputRegions;
    for ( auto& inputRegion : inputRegions ){
      unsigned dimensionToSplit = getLargestInterval( inputRegion );
      bisectInputRegion( inputRegion, dimensionToSplit, newInputRegions );
    }
    inputRegions = newInputRegions;
  }

  unsigned query_id_suffix = 1; // For query id
  // Create a new subquery for each newly created input region
  for ( auto& inputRegion : inputRegions ){
    // Create a new query id
    std::unique_ptr<std::string> query_id = std::unique_ptr<std::string>
      ( new std::string );
    if ( query_id_prefix == "" )
      query_id->assign( query_id_prefix + std::to_string( query_id_suffix++ ) );
    else
      query_id->assign( query_id_prefix + "-" + std::to_string
                        ( query_id_suffix++ ) );
    // Create a new case split
    auto split = std::unique_ptr<PiecewiseLinearCaseSplit>
      ( new PiecewiseLinearCaseSplit() );
    // Add bound as equations for each input variable
    for ( auto& variable : _inputVariables ){
      double lb = inputRegion.lowerbounds[variable];
      double ub = inputRegion.upperbounds[variable];
      split->storeBoundTightening( Tightening( variable, lb, Tightening::LB ) );
      split->storeBoundTightening( Tightening( variable, ub, Tightening::UB ) );
    }

    // Construct the new subquery and add it to subqueries
    SubQuery* subquery = new SubQuery( std::move(query_id), std::move(split),
                                      (unsigned) (timeoutInSeconds *
                                                  _timeout_factor) );
    subqueries.push_back( subquery );
  }
}

unsigned LargestIntervalDivider::getLargestInterval( InputRegion& inputRegion ){
  ASSERT(inputRegion.lowerbounds.size() == inputRegion.lowerbounds.size());
  unsigned dimensionToSplit = 0;
  double largest_interval = 0;
  for ( auto& i : _inputVariables ){
    double interval = inputRegion.upperbounds[i] - inputRegion.lowerbounds[i];
    assert (interval > 0);
    if (interval > largest_interval){
      dimensionToSplit = i;
      largest_interval = interval;
    }
  }
  return dimensionToSplit;
}
