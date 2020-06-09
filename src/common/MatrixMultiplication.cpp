#include<MatrixMultiplication.h>

#ifdef USE_OPENBLAS 
#include "cblas.h"
    void matrixMultiplication ( const double *matA, const double *matB, double *matC,
                               unsigned rowsA, unsigned columnsA,
                               unsigned columnsB )
    {
            double alpha = 1;
            double beta = 1;
            // cblas_dgemm is executing: C <- alpha * A B + beta * C 
            // See https://developer.apple.com/documentation/accelerate/1513282-cblas_dgemm?language=objc
            // for the documentation of cblas_dgemm.

            cblas_dgemm( CblasRowMajor, CblasNoTrans, CblasNoTrans, rowsA, columnsB,
                         columnsA, alpha, matA, columnsA, matB, columnsB, beta, matC, columnsB);
    }
#else
    void matrixMultiplication( double *matA, double *matB, double *matC,
                               unsigned rowsA, unsigned columnsA,
                               unsigned columnsB )
    {
        for ( unsigned i = 0; i < rowsA; ++i )
        {
            for ( unsigned j = 0; j < columnsB; ++j )
            {
                for ( unsigned k = 0; k < columnsA; ++k )
                {
                    matC[i * columnsB + j] += matA[i * columnsA + k]
                                                * matB[k * columnsB + j];
                }
            }
        }
    }
#endif
