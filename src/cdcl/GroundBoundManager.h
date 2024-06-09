#include "PlcLemma.h"
#include "Tightening.h"
#include "Vector.h"
#include "context/cdo.h"
#include "context/context.h"

#ifndef MARABOU_GROUNDBOUNDMANAGER_H
#define MARABOU_GROUNDBOUNDMANAGER_H


class GroundBoundManager
{
public:
    struct GroundBoundEntry
    {
        unsigned id;
        double val;
        const std::shared_ptr<PLCLemma> lemma;
        Set<int> clause;
    };

    GroundBoundManager( CVC4::context::Context &ctx );
    ~GroundBoundManager();

    void initialize( unsigned size );
    void addGroundBound( unsigned index, double value, Tightening::BoundType boundType );
    void addGroundBound( const std::shared_ptr<PLCLemma> lemma );

    double getGroundBound( unsigned index, Tightening::BoundType boundType ) const;
    const GroundBoundEntry &
    getGroundBoundEntryUpToId( unsigned index, Tightening::BoundType boundType, unsigned id ) const;

    double
    getGroundBoundUpToId( unsigned index, Tightening::BoundType boundType, unsigned id ) const;
    const Vector<double> getAllGroundBounds( Tightening::BoundType boundType ) const;

    unsigned getCounter() const;

    void addClauseToGroundBoundEntry( unsigned int index,
                                      Tightening::BoundType boundType,
                                      unsigned int id,
                                      const Set<int> &clause );

private:
    CVC4::context::CDO<unsigned> *_counter;

    CVC4::context::Context &_context;
    unsigned _size;

    Vector<CVC4::context::CDList<GroundBoundEntry> *> _upperGroundBounds;
    Vector<CVC4::context::CDList<GroundBoundEntry> *> _lowerGroundBounds;
};


#endif // MARABOU_GROUNDBOUNDMANAGER_H
