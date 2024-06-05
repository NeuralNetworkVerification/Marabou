#include "GroundBoundManager.h"


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
        _upperGroundBounds.append( new ( true )
                                       CVC4::context::CDList<GroundBoundEntry>( &_context ) );
        _lowerGroundBounds.append( new ( true )
                                       CVC4::context::CDList<GroundBoundEntry>( &_context ) );
    }
}

void GroundBoundManager::addGroundBound( unsigned index,
                                         double value,
                                         Tightening::BoundType isUpper )
{
    Vector<CVC4::context::CDList<GroundBoundEntry> *> temp =
        isUpper == Tightening::UB ? _upperGroundBounds : _lowerGroundBounds;
    temp[index]->push_back( { _counter->get(), value, NULL } );
    _counter->set( _counter->get() + 1 );
}

void GroundBoundManager::addGroundBound( const std::shared_ptr<PLCLemma> lemma )
{
    Tightening::BoundType isUpper = lemma->getAffectedVarBound();
    unsigned index = lemma->getAffectedVar();
    Vector<CVC4::context::CDList<GroundBoundEntry> *> temp =
        isUpper == Tightening::UB ? _upperGroundBounds : _lowerGroundBounds;
    temp[index]->push_back( { _counter->get(), lemma->getBound(), lemma } );
    _counter->set( _counter->get() + 1 );
}

double GroundBoundManager::getGroundBound( unsigned index, Tightening::BoundType isUpper ) const
{
    Vector<CVC4::context::CDList<GroundBoundEntry> *> temp =
        isUpper == Tightening::UB ? _upperGroundBounds : _lowerGroundBounds;
    return temp[index]->back().val;
}
double GroundBoundManager::getGroundBoundUpToId( unsigned index,
                                                 Tightening::BoundType isUpper,
                                                 unsigned id ) const
{
    Vector<CVC4::context::CDList<GroundBoundEntry> *> temp =
        isUpper == Tightening::UB ? _upperGroundBounds : _lowerGroundBounds;
    for ( auto entry = temp[index]->end(); entry != temp[index]->begin(); --entry )
    {
        if ( ( *entry ).id <= id )
            return ( *entry ).val;
    }

    // Should not happen
    ASSERT( false );
    return ( *temp[index]->begin() ).val;
}

const Vector<double> GroundBoundManager::getAllGroundBounds( Tightening::BoundType isUpper ) const
{
    Vector<CVC4::context::CDList<GroundBoundEntry> *> temp =

        isUpper == Tightening::UB ? _upperGroundBounds : _lowerGroundBounds;
    Vector<double> tops = Vector<double>( 0 );

    for ( const auto &GBList : temp )
        tops.append( GBList->back().val );

    return tops;
}
