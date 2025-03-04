/*********************                                                        */
/*! \file CadicalWrapper.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Idan Refaeli, Omri Isac
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "CadicalWrapper.h"

CadicalWrapper::CadicalWrapper( CaDiCaL::ExternalPropagator *externalPropagator,
                                CaDiCaL::Terminator *terminator,
                                CaDiCaL::FixedAssignmentListener *fixedListener )
    : _solver( new CaDiCaL::Solver() )
{
    _solver->set( "walk", 0 );
    _solver->set( "lucky", 0 );
    _solver->set( "log", 0 );

    _solver->connect_external_propagator( externalPropagator );
    _solver->connect_terminator( terminator );
    _solver->connect_fixed_listener( fixedListener );
}

void CadicalWrapper::addLiteral( int lit )
{
    _solver->add( lit );
}

void CadicalWrapper::addClause( const Set<int> &clause )
{
    _solver->clause( std::vector<int>( clause.begin(), clause.end() ) );
}

void CadicalWrapper::assume( int lit )
{
    _solver->assume( lit );
}

void CadicalWrapper::phase( int lit )
{
    _solver->phase( lit );
}

int CadicalWrapper::solve()
{
    return _solver->solve();
}

int CadicalWrapper::val( int lit )
{
    return _solver->val( lit );
}

void CadicalWrapper::flip( int lit )
{
    _solver->flip( lit );
}

void CadicalWrapper::addObservedVar( int var )
{
    _solver->add_observed_var( var );
}

bool CadicalWrapper::isDecision( int observedVar ) const
{
    return _solver->is_decision( observedVar );
}

int CadicalWrapper::vars()
{
    return _solver->vars();
}

void CadicalWrapper::forceBacktrack( size_t newLevel )
{
    _solver->force_backtrack( newLevel );
}

Set<int> CadicalWrapper::addExternalNAPClause( const String &externalNAPClauseFilename,
                                               bool isNegated )
{
    Set<int> clause;

    if ( externalNAPClauseFilename != "" && File::exists( externalNAPClauseFilename ) )
    {
        File externalNAPClauseFile( externalNAPClauseFilename );
        externalNAPClauseFile.open( IFile::MODE_READ );

        while ( true )
        {
            String lit = externalNAPClauseFile.readLine().trim();

            if ( lit == "" )
                break;

            if ( isNegated )
                clause.insert( -atoi( lit.ascii() ) );
            else
                clause.insert( atoi( lit.ascii() ) );
        }

        addClause( clause );
    }

    return clause;
}

CadicalWrapper::~CadicalWrapper()
{
    _solver->disconnect_fixed_listener();
    _solver->disconnect_terminator();
    _solver->disconnect_external_propagator();
}
