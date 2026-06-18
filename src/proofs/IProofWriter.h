/**
** \verbatim
** Top contributors (to current version):
**   Omri Isac, Guy Katz
** This file is part of the Marabou project.
** Copyright (c) 2017-2026 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**
** [[ Add lengthier description here ]]
**/

#ifndef __IProofWriter_h__
#define __IProofWriter_h__

#include "GroundBoundManager.h"
#include "IFile.h"
#include "PiecewiseLinearConstraint.h"
#include "SparseUnsortedList.h"
#include "UnsatCertificateNode.h"

class IProofWriter
{
public:
    virtual ~IProofWriter() = default;

    /*
     Write whole proof info to a file
    */
    virtual void writeInstanceToFile( IFile &file ) = 0;

    /*
     Write steps to conclude UNSAT of a node from the UNSAT of its children
    */
    virtual void writeChildrenConclusion( const UnsatCertificateNode *node ) = 0;

    /*
     Write proof hole for a delegated leaf node
    */
    virtual void writeDelegatedLeaf( const UnsatCertificateNode *node ) = 0;

    /*
     Add proof steps to prove a PLC lemma
    */
    virtual void
    writeLemma( const std::shared_ptr<GroundBoundManager::GroundBoundEntry> &lemmaEntry ) = 0;

    /*
     Add proof steps to prove the UNSAT of a leaf
    */
    virtual void writeContradiction( const SparseUnsortedList &contradiction,
                                     UnsatCertificateNode *node ) = 0;

    /*
     Delete the content of the proof
    */
    virtual void deleteProof() = 0;
};
#endif //__IProofWriter_h__
