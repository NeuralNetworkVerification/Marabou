/*********************                                                        */
/*! \file EtaMatrix.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __EtaMatrix_h__
#define __EtaMatrix_h__

class EtaMatrix
{
public:
    EtaMatrix( unsigned m, unsigned index, double *column );
    ~EtaMatrix();

    unsigned _m;
    unsigned _columnIndex;
    double *_column;
};

#endif // __EtaMatrix_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
