#include "CadicalWrapper.h"

#include <utility>

CadicalWrapper::CadicalWrapper()
    : d_solver( new CaDiCaL::Solver() )
{
    d_solver->set( "walk", 0 );
    d_solver->set( "lucky", 0 );
    d_solver->set( "log", 0 );
}


void CadicalWrapper::addLiteral( int lit )
{
    d_solver->add( lit );
}

void CadicalWrapper::addClause( const Set<int> &clause )
{
    d_solver->clause( std::vector<int>( clause.begin(), clause.end() ) );
}

bool CadicalWrapper::inconsistent()
{
    return d_solver->inconsistent();
}

void CadicalWrapper::assume( int lit )
{
    d_solver->assume( lit );
}

void CadicalWrapper::phase( int lit )
{
    d_solver->phase( lit );
}

int CadicalWrapper::solve()
{
    return d_solver->solve();
}

int CadicalWrapper::val( int lit )
{
    return d_solver->val( lit );
}

Map<int, int> CadicalWrapper::getModel()
{
    Map<int, int> model;
    int numVars = d_solver->vars();
    for ( int var = 1; var <= numVars; ++var )
        model[var] = d_solver->val( var );

    return model;
}

void CadicalWrapper::flip( int lit )
{
    d_solver->flip( lit );
}

void CadicalWrapper::connectTheorySolver( CaDiCaL::ExternalPropagator *externalPropagator )
{
    d_solver->connect_external_propagator( externalPropagator );
}

void CadicalWrapper::disconnectTheorySolver()
{
    d_solver->disconnect_external_propagator();
}

void CadicalWrapper::addObservedVar( int var )
{
    d_solver->add_observed_var( var );
}

void CadicalWrapper::removeObservedVar( int var )
{
    d_solver->remove_observed_var( var );
}

bool CadicalWrapper::isDecision( int observedVar ) const
{
    return d_solver->is_decision( observedVar );
}

int CadicalWrapper::vars()
{
    return d_solver->vars();
}

void CadicalWrapper::connectTerminator( CaDiCaL::Terminator *terminator )
{
    d_solver->connect_terminator( terminator );
}

void CadicalWrapper::disconnectTerminator()
{
    d_solver->disconnect_terminator();
}

void CadicalWrapper::connectFixedListener( CaDiCaL::FixedAssignmentListener *fixedListener )
{
    d_solver->connect_fixed_listener( fixedListener );
}

void CadicalWrapper::disconnectFixedListener()
{
    d_solver->disconnect_fixed_listener();
}

void CadicalWrapper::forceBacktrack( size_t newLevel )
{
    d_solver->force_backtrack( newLevel );
}

void CadicalWrapper::addExternalNAPClause( const String &externalNAPClauseFilename )
{
    if ( File::exists( externalNAPClauseFilename ) )
    {
        File externalNAPClauseFile( externalNAPClauseFilename );
        externalNAPClauseFile.open( IFile::MODE_READ );

        Set<int> clause;

        while ( true )
        {
            String lit = externalNAPClauseFile.readLine().trim();

            if ( lit == "" )
                break;

            clause.insert( -atoi( lit.ascii() ) );
        }

        addClause( clause );
    }
}
