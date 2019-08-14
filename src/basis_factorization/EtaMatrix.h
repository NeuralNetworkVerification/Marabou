/*********************                                                        */
/*! \file EtaMatrix.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifndef __EtaMatrix_h__
#define __EtaMatrix_h__

class EtaMatrix
{
public:
    EtaMatrix( unsigned m, unsigned index, const double *column );

    /*
      Initializees the matrix to the identity matrix
    */
    EtaMatrix( unsigned m, unsigned index );

    EtaMatrix( const EtaMatrix &other );
    EtaMatrix &operator=( const EtaMatrix &other );

    ~EtaMatrix();
    void dump() const;
	void toMatrix( double *A ) const;

    void resetToIdentity();

    bool operator==( const EtaMatrix &other ) const;

    unsigned _m;
    unsigned _columnIndex;
    double *_column;
};

#endif // __EtaMatrix_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
