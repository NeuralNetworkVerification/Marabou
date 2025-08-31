/*********************                                                        */
/*! \file GroundBoundManager.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Idan Refaeli, Omri Isac
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2025 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "GroundBoundManager.h"

#include "FloatUtils.h"


GroundBoundManager::GroundBoundManager( CVC4::context::Context &ctx )
    : _context( ctx )
    , _size( 0 )
    , _upperGroundBounds( 0 )
    , _lowerGroundBounds( 0 )
{
    _counter = new ( true ) CVC4::context::CDO<unsigned>( &_context, 0 );
}
GroundBoundManager::~GroundBoundManager()
{
    _counter->deleteSelf();
    for ( unsigned i = 0; i < _size; ++i )
    {
        _upperGroundBounds[i]->deleteSelf();
        _lowerGroundBounds[i]->deleteSelf();
    }
}

void GroundBoundManager::initialize( unsigned size )
{
    _counter->set( 0 );
    _size = size;

    for ( unsigned i = 0; i < size; ++i )
    {
        _upperGroundBounds.append(
            new ( true ) CVC4::context::CDList<std::shared_ptr<GroundBoundEntry>>( &_context ) );
        _lowerGroundBounds.append(
            new ( true ) CVC4::context::CDList<std::shared_ptr<GroundBoundEntry>>( &_context ) );
    }
}

std::shared_ptr<GroundBoundManager::GroundBoundEntry>
GroundBoundManager::addGroundBound( unsigned index,
                                    double value,
                                    Tightening::BoundType boundType,
                                    bool isPhaseFixing )
{
    const Vector<CVC4::context::CDList<std::shared_ptr<GroundBoundEntry>> *> &temp =
        boundType == Tightening::UB ? _upperGroundBounds : _lowerGroundBounds;
    std::shared_ptr<GroundBoundEntry> groundBoundEntry(
        new GroundBoundEntry( _counter->get(), value, nullptr, Set<int>(), isPhaseFixing ) );


    if ( !temp[index]->empty() )
    {
        ASSERT( boundType == Tightening::UB ? FloatUtils::lte( value, temp[index]->back()->val )
                                            : FloatUtils::gte( value, temp[index]->back()->val ) )
    }

    temp[index]->push_back( groundBoundEntry );
    _counter->set( _counter->get() + 1 );

    return temp[index]->back();
}

std::shared_ptr<GroundBoundManager::GroundBoundEntry>
GroundBoundManager::addGroundBound( const std::shared_ptr<PLCLemma> &lemma, bool isPhaseFixing )
{
    Tightening::BoundType isUpper = lemma->getAffectedVarBound();
    unsigned index = lemma->getAffectedVar();
    const Vector<CVC4::context::CDList<std::shared_ptr<GroundBoundEntry>> *> &temp =
        isUpper == Tightening::UB ? _upperGroundBounds : _lowerGroundBounds;
    std::shared_ptr<GroundBoundEntry> groundBoundEntry( new GroundBoundEntry(
        _counter->get(), lemma->getBound(), lemma, Set<int>(), isPhaseFixing ) );

    if ( !temp[index]->empty() )
    {
        ASSERT( isUpper == Tightening::UB
                    ? FloatUtils::lte( lemma->getBound(), temp[index]->back()->val )
                    : FloatUtils::gte( lemma->getBound(), temp[index]->back()->val ) )
    }

    temp[index]->push_back( groundBoundEntry );
    _counter->set( _counter->get() + 1 );

    return temp[index]->back();
}

double GroundBoundManager::getGroundBound( unsigned index, Tightening::BoundType boundType ) const
{
    const Vector<CVC4::context::CDList<std::shared_ptr<GroundBoundEntry>> *> &temp =
        boundType == Tightening::UB ? _upperGroundBounds : _lowerGroundBounds;
    return temp[index]->back()->val;
}

std::shared_ptr<GroundBoundManager::GroundBoundEntry>
GroundBoundManager::getGroundBoundEntry( unsigned int index, Tightening::BoundType boundType ) const
{
    const Vector<CVC4::context::CDList<std::shared_ptr<GroundBoundEntry>> *> &temp =
        boundType == Tightening::UB ? _upperGroundBounds : _lowerGroundBounds;
    return temp[index]->back();
}

std::shared_ptr<GroundBoundManager::GroundBoundEntry>
GroundBoundManager::getGroundBoundEntryUpToId( unsigned index,
                                               Tightening::BoundType boundType,
                                               unsigned id ) const
{
    const Vector<CVC4::context::CDList<std::shared_ptr<GroundBoundEntry>> *> &temp =
        boundType == Tightening::UB ? _upperGroundBounds : _lowerGroundBounds;

    if ( id == 0 )
        return ( *temp[index] )[0];

    for ( int i = temp[index]->size() - 1; i >= 0; --i )
    {
        const std::shared_ptr<GroundBoundEntry> entry = ( *temp[index] )[i];
        if ( entry->id < id )
            return entry;
    }
    ASSERT( id <= 2 * temp.size() );
    return ( *temp[index] )[0];
}

double GroundBoundManager::getGroundBoundUpToId( unsigned index,
                                                 Tightening::BoundType boundType,
                                                 unsigned id ) const
{
    return getGroundBoundEntryUpToId( index, boundType, id )->val;
}

Vector<double> GroundBoundManager::getAllGroundBounds( Tightening::BoundType boundType ) const
{
    const Vector<CVC4::context::CDList<std::shared_ptr<GroundBoundEntry>> *> &temp =
        boundType == Tightening::UB ? _upperGroundBounds : _lowerGroundBounds;
    Vector<double> tops = Vector<double>( 0 );

    for ( const auto &GBList : temp )
        tops.append( GBList->back()->val );

    return tops;
}

unsigned GroundBoundManager::getCounter() const
{
    return _counter->get();
}

void GroundBoundManager::addClauseToGroundBoundEntry(
    const std::shared_ptr<GroundBoundManager::GroundBoundEntry> &entry,
    const Set<int> &clause )
{
    entry->clause = clause;
}

Vector<double>
GroundBoundManager::getAllInitialGroundBounds( Tightening::BoundType boundType ) const
{
    const Vector<CVC4::context::CDList<std::shared_ptr<GroundBoundEntry>> *> &temp =
        boundType == Tightening::UB ? _upperGroundBounds : _lowerGroundBounds;
    Vector<double> bots = Vector<double>( 0 );

    for ( const auto &GBList : temp )
        bots.append( ( *GBList->begin() )->val );

    return bots;
}
