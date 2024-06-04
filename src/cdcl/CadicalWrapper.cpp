#include "CadicalWrapper.h"

CadicalWrapper::CadicalWrapper()
    : d_solver( new CaDiCaL::Solver() )
{
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
    {
        model[var] = d_solver->val( var );
    }

    return model;
}

void CadicalWrapper::flip( int lit )
{
    d_solver->flip( lit );
}

void CadicalWrapper::connectTheorySolver()
{
    d_solver->connect_external_propagator( nullptr ); // TODO: add Theory Solver
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

bool CadicalWrapper::isDecision( int observedVar )
{
    return d_solver->is_decision( observedVar );
}
