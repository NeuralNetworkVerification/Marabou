/*********************                                                        */
/*! \file SignalHandler.h
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

#ifndef __SignalHandler_h__
#define __SignalHandler_h__

#include "List.h"

class SignalHandler
{
public:
    class Signalable
    {
    public:
        virtual void quitSignal() = 0;
    };

    /*
      Get the singleton signal handler
    */
    static SignalHandler *getInstance();

    /*
      Register a client to receive signals
    */
    void registerClient( Signalable *client );

    /*
      Initialize the signal handling
    */
    void initialize();

    /*
      Called when a signal is received
    */
    void signalReceived( unsigned signalNumber );

private:
    List<Signalable *> _clients;

    /*
      Prevent additional instantiations of the class
    */
    SignalHandler() {}
    SignalHandler( const SignalHandler & ) {}
};

#endif // __SignalHandler_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
