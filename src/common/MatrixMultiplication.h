
#ifndef __MatrixMultiplication_h__
#define __MatrixMultiplication_h__

/*
  The size of matA is rowsA x columnsA,
  and the size of matB is columnsA x columnsB.
  Compute matA * matB + matC and store the result in matC
*/
void matrixMultiplication( const double *matA, const double *matB, double *matC,
                           unsigned rowsA, unsigned columnsA,
                           unsigned columnsB );

#endif // __MatrixMultiplication_h__
