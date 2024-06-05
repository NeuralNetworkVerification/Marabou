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
    };

    GroundBoundManager( CVC4::context::Context &ctx );
    ~GroundBoundManager();

    void initialize( unsigned size );
    void addGroundBound( unsigned index, double value, Tightening::BoundType isUpper );
    void addGroundBound( const std::shared_ptr<PLCLemma> lemma );

    double getGroundBound( unsigned index, Tightening::BoundType isUpper ) const;
    double getGroundBoundUpToId( unsigned index, Tightening::BoundType isUpper, unsigned id ) const;
    const Vector<double> getAllGroundBounds( Tightening::BoundType isUpper ) const;


private:
    CVC4::context::CDO<unsigned> *_counter;

    CVC4::context::Context &_context;
    unsigned _size;

    Vector<CVC4::context::CDList<GroundBoundEntry> *> _upperGroundBounds;
    Vector<CVC4::context::CDList<GroundBoundEntry> *> _lowerGroundBounds;
};


#endif // MARABOU_GROUNDBOUNDMANAGER_H
