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
        _upperGroundBounds.append(
            new ( true ) CVC4::context::CDList<std::shared_ptr<GroundBoundEntry>>( &_context ) );
        _lowerGroundBounds.append(
            new ( true ) CVC4::context::CDList<std::shared_ptr<GroundBoundEntry>>( &_context ) );
    }
}

void GroundBoundManager::addGroundBound( unsigned index,
                                         double value,
                                         Tightening::BoundType boundType )
{
    const Vector<CVC4::context::CDList<std::shared_ptr<GroundBoundEntry>> *> &temp =
        boundType == Tightening::UB ? _upperGroundBounds : _lowerGroundBounds;
    std::shared_ptr<GroundBoundEntry> groundBoundEntry(
        new GroundBoundEntry( _counter->get(), value, nullptr, Set<int>() ) );
    temp[index]->push_back( groundBoundEntry );
    _counter->set( _counter->get() + 1 );
}

void GroundBoundManager::addGroundBound( const std::shared_ptr<PLCLemma> &lemma )
{
    Tightening::BoundType isUpper = lemma->getAffectedVarBound();
    unsigned index = lemma->getAffectedVar();
    const Vector<CVC4::context::CDList<std::shared_ptr<GroundBoundEntry>> *> &temp =
        isUpper == Tightening::UB ? _upperGroundBounds : _lowerGroundBounds;
    std::shared_ptr<GroundBoundEntry> groundBoundEntry(
        new GroundBoundEntry( _counter->get(), lemma->getBound(), lemma, Set<int>() ) );
    temp[index]->push_back( groundBoundEntry );
    _counter->set( _counter->get() + 1 );
}

double GroundBoundManager::getGroundBound( unsigned index, Tightening::BoundType boundType ) const
{
    const Vector<CVC4::context::CDList<std::shared_ptr<GroundBoundEntry>> *> &temp =
        boundType == Tightening::UB ? _upperGroundBounds : _lowerGroundBounds;
    return temp[index]->back()->val;
}

std::shared_ptr<GroundBoundManager::GroundBoundEntry>
GroundBoundManager::getGroundBoundEntryUpToId( unsigned index,
                                               Tightening::BoundType boundType,
                                               unsigned id ) const
{
    const Vector<CVC4::context::CDList<std::shared_ptr<GroundBoundEntry>> *> &temp =
        boundType == Tightening::UB ? _upperGroundBounds : _lowerGroundBounds;

    for ( int i = temp[index]->size() - 1; i >= 0; --i )
    {
        const std::shared_ptr<GroundBoundEntry> entry = ( *temp[index] )[i];
        if ( entry->id < id )
            return entry;
    }

    ASSERT( false );
    printf( "\noops. current id is %d\n", id );
    return temp[index]->operator[]( ( 0 ) );
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

void GroundBoundManager::addClauseToGroundBoundEntry( unsigned int index,
                                                      Tightening::BoundType boundType,
                                                      unsigned int id,
                                                      const Set<int> &clause )
{
    const Vector<CVC4::context::CDList<std::shared_ptr<GroundBoundEntry>> *> &temp =
        boundType == Tightening::UB ? _upperGroundBounds : _lowerGroundBounds;

    for ( unsigned int i = 0; i < temp[index]->size(); ++i )
    {
        if ( ( i + 1 == temp[index]->size() ) || ( ( *temp[index] )[i + 1]->id >= id ) )
        {
            (*temp[index])[i]->clause = clause;
            return;
        }
    }
}
