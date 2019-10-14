/*********************                                                        */
/*! \file SignalHandler.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
** Copyright (c) 2016-2017 by the authors listed in the signalHandler AUTHORS
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "SignalHandler.h"
#include <cstring>
#include <signal.h>

void got_signal( int signalNumber )
{
    SignalHandler::getInstance()->signalReceived( signalNumber );
}

SignalHandler *SignalHandler::getInstance()
{
    static SignalHandler handler;
    return &handler;
}

void SignalHandler::registerClient( Signalable *client )
{
    _clients.append( client );
}

void SignalHandler::initialize()
{

#ifdef _WIN32
    signal( SIGINT, got_signal );
    signal( SIGTERM, got_signal );
    signal( SIGABRT, got_signal );

#else
    struct sigaction sa;
    memset( &sa, 0, sizeof(sa) );
    sa.sa_handler = got_signal;
    sigfillset( &sa.sa_mask );
    sigaction( SIGQUIT, &sa, NULL );
    sigaction( SIGTERM, &sa, NULL );
#endif

}

void SignalHandler::signalReceived( unsigned /* signalNumber */ )
{
    for ( const auto &signalable : _clients )
        signalable->quitSignal();
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-signalHandler-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
