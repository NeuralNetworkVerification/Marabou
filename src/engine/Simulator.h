/*********************                                                        */
/*! \file Simulator.h
** \verbatim
** Top contributors (to current version):
**   Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __Simulator_h__
#define __Simulator_h__

#include "InputQuery.h"
#include "Simulator.h"

/*
  This class takes an input query, with marked input variables,
  and runs simulations of the neural network that it describes.
  The simulations are performed by selecting values for the
  input variables uniformly at random, evaluating the network,
  and then storing the results.
*/

class Simulator
{
public:
    typedef Map<unsigned, double> Result;

    /*
      Perform a given number of simulation runs on the input query.
      The seed will be used to initialize randomness. A specific seed can be
      passed for determinism; otherwise, time() will be used.
    */
    void runSimulations( const InputQuery &inputQuery, unsigned numberOfSimulations );
    void runSimulations( const InputQuery &inputQuery, unsigned numberOfSimulations, unsigned seed );

    /*
      Obtain the results from previously-run simulations.
    */
    const List<Simulator::Result> *getResults();

private:
    InputQuery _originalQuery;
    List<Simulator::Result> _results;

    /*
      Store a copy of the original input query, and run an initial
      round of preprocessing
    */
    void storeOriginalQuery( const InputQuery &inputQuery );

    /*
      Run a single simulation of the stored and preprocessed input
      query.
    */
    void runSingleSimulation();
};

#endif // __Simulator_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
