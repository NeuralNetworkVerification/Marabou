/*********************                                                        */
/*! \file GroundBoundManager.h
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

#ifndef __GroundBoundManager_h__
#define __GroundBoundManager_h__

#include "PlcLemma.h"
#include "Set.h"
#include "Tightening.h"
#include "Vector.h"
#include "context/cdlist.h"
#include "context/cdo.h"
#include "context/context.h"

class GroundBoundManager
{
public:
    struct GroundBoundEntry
    {
        GroundBoundEntry( unsigned id,
                          double val,
                          const std::shared_ptr<PLCLemma> &lemma,
                          const Set<int> &clause,
                          bool isPhaseFixing )
            : id( id )
            , val( val )
            , lemma( lemma )
            , clause( clause )
            , isPhaseFixing( isPhaseFixing )
        {
        }
        unsigned id;
        double val;
        const std::shared_ptr<PLCLemma> lemma;
        Set<int> clause;
        bool isPhaseFixing;
        Set<std::shared_ptr<GroundBoundManager::GroundBoundEntry>> depList;
    };

    GroundBoundManager( CVC4::context::Context &ctx );
    ~GroundBoundManager();

    void initialize( unsigned size );
    std::shared_ptr<GroundBoundManager::GroundBoundEntry>
    addGroundBound( unsigned index,
                    double value,
                    Tightening::BoundType boundType,
                    bool isPhaseFixing );
    std::shared_ptr<GroundBoundManager::GroundBoundEntry>
    addGroundBound( const std::shared_ptr<PLCLemma> &lemma, bool isPhaseFixing );

    double getGroundBound( unsigned index, Tightening::BoundType boundType ) const;

    std::shared_ptr<GroundBoundManager::GroundBoundEntry>
    getGroundBoundEntry( unsigned index, Tightening::BoundType boundType ) const;

    std::shared_ptr<GroundBoundManager::GroundBoundEntry>
    getGroundBoundEntryUpToId( unsigned index, Tightening::BoundType boundType, unsigned id ) const;

    double
    getGroundBoundUpToId( unsigned index, Tightening::BoundType boundType, unsigned id ) const;
    Vector<double> getAllGroundBounds( Tightening::BoundType boundType ) const;
    Vector<double> getAllInitialGroundBounds( Tightening::BoundType boundType ) const;

    unsigned getCounter() const;

    void
    addClauseToGroundBoundEntry( const std::shared_ptr<GroundBoundManager::GroundBoundEntry> &entry,
                                 const Set<int> &clause );

private:
    CVC4::context::CDO<unsigned> *_counter;

    CVC4::context::Context &_context;
    unsigned _size;

    Vector<CVC4::context::CDList<std::shared_ptr<GroundBoundEntry>> *> _upperGroundBounds;
    Vector<CVC4::context::CDList<std::shared_ptr<GroundBoundEntry>> *> _lowerGroundBounds;
};


#endif // __GroundBoundManager_h__
