/*********************                                                        */
/*! \file BasisFactorization.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#include "BasisFactorization.h"
#include "Debug.h"
#include "EtaMatrix.h"
#include "FloatUtils.h"
#include "ReluplexError.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>

BasisFactorization::BasisFactorization( unsigned m )
    : _B0( NULL) 
	, _m( m )
{
    _B0 = new double[m*m];
	_U = new double [m*m];
	_I = new double [m*m];
	_start = true;
	_factorFlag = false;
	constructIdentity();
    if ( !_B0 && !_U )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "BasisFactorization::B0" );
}

BasisFactorization::~BasisFactorization()
{
    freeIfNeeded();
}

void BasisFactorization::freeIfNeeded()
{
    if ( _B0 )
    {
        delete[] _B0;
        _B0 = NULL;
    }
	if ( _U )
	{
		delete[] _U;
		_U = NULL;
	}
	if ( _I )
	{
		delete[] _I;
		_I = NULL;
	}

    List<EtaMatrix *>::iterator it;
    for ( it = _etas.begin(); it != _etas.end(); ++it )
        delete *it;

    _etas.clear();
}

void BasisFactorization::pushEtaMatrix( unsigned columnIndex, double *column )
{
    EtaMatrix *matrix = new EtaMatrix( _m, columnIndex, column );
    _etas.append( matrix );
}

void BasisFactorization::constructIdentity () 
{
	std::fill_n(_I, _m*_m, 0.);
	std::fill_n(_B0, _m*_m, 0.);
	for (unsigned int i = 0; i < _m; ++i) {
		_I[i*_m+i] = 1;
		_B0[i*_m+i] = 1;
	}
}	

void BasisFactorization::matrixMultiply ( const double *L, const double *B, double *R){
	for (unsigned int n = 0; n < _m; ++n){ //columns of U
		for (unsigned int i = 0; i < _m; ++i){ // rows of L
			double sum = 0.;
			for (unsigned int k = 0; k < _m; ++k) //can be optimized by replacing d with i + 1
				sum += L[i*_m+k] * B[n+k*_m];
			R[i*_m+n]=sum;
		}
	}
}

void BasisFactorization::LPMultiplyBack ( const double *X, const double *LP, double *R)
{
	for (unsigned int i = 0; i < _m; ++i) {
		double sum = 0.;
		for (unsigned int j = i; j < _m; ++j) {
			sum += X[j] * LP[i+j*_m];
		}
		R[i] = sum;
	}
}

void BasisFactorization::LPMultiplyFront ( const double *LP, const double *X, double *R)
{
	for (unsigned int i = 0; i < _m; ++i) {
		double sum = 0.;
		for (unsigned int j = 0; j <= i; ++j) {
			sum += X[j] * LP[i*_m+j];
		}
		R[i] = sum;
	}
}

void BasisFactorization::setB0 ( double *B0 ){
	_start = false;
	memcpy ( _B0, B0, sizeof(double) * _m * _m);
}

void BasisFactorization::refactor ()
{
	for ( auto &eta : _etas) {
		double Y[_m*_m];
		memcpy ( Y, _B0, sizeof(double) * _m * _m );
		for (unsigned int n = 0; n < _m; ++n){ //columns
				double sum = 0.;
				for (unsigned int i = 0; i < _m; ++i){ // rows of L
						sum += Y[n*_m+i] * eta->_column[i];
				}
				_B0[n*_m+eta->_columnIndex] = sum;
		}
		//coutMatrix ( _B0 );
		delete eta;
	}	
	_etas.clear();
	_start = false;
}

void BasisFactorization::forwardTransformation( const double *y, double *x )
{
    if ( _etas.empty() )
    {
        memcpy( x, y, sizeof(double) * _m );
        return;
    }
	if ( _etas.size() > 5 && _factorFlag )
	{
		//std::cout << _etas.size() << std::endl;
		refactor();
		//coutMatrix ( _B0 );
	}
    double *tempY = new double[_m];
    memcpy( tempY, y, sizeof(double) * _m );
    std::fill( x, x + _m, 0.0 );

	if (!_start) {
		factorization ( _B0 );
	}
	
	for ( int i = _LP.size() - 1; i >= 0; --i) {
		auto elem = _LP.at(i);
		if (elem != _I) {
			double temp[_m];
			LPMultiplyFront( elem , tempY, temp);
			memcpy ( tempY, temp, sizeof(double) * _m);
		}
		delete[] elem;
	}

	if (!_start) {
		x[_m-1] = tempY[_m-1];
		for (int i = _m - 2; i >= 0; --i){
			double sum = 0.;
			for (int j = _m - 1; j > i; --j){
				sum += _U[i*_m+j] * x[j];
			}
			x[i] = tempY[i] - sum;
		}
		memcpy ( tempY, x, sizeof(double) * _m );	
	}
	
	for ( const auto &eta : _etas )
    {
        x[eta->_columnIndex] = tempY[eta->_columnIndex] / eta->_column[eta->_columnIndex];

        // Solve the rows above columnIndex
        for ( unsigned i = eta->_columnIndex + 1; i < _m; ++i )
            x[i] = tempY[i] - ( x[eta->_columnIndex] * eta->_column[i] );

        // Solve the rows below columnIndex
        for ( int i = eta->_columnIndex - 1; i >= 0; i-- )
            x[i] = tempY[i] - ( x[eta->_columnIndex] * eta->_column[i] );

        // The x from this iteration becomes a for the next iteration
        memcpy( tempY, x, sizeof(double) *_m );
    }
	
}	
	


void BasisFactorization::backwardTransformation( const double *y, double *x )
{
    if ( _etas.empty() )
    {
        memcpy( x, y, sizeof(double) * _m );
        return;
    }

    double *tempY = new double[_m];
    memcpy( tempY, y, sizeof(double) * _m );
    std::fill( x, x + _m, 0.0 );

    for ( auto eta = _etas.rbegin(); eta != _etas.rend(); ++eta )
    {
        // x is going to equal y in all entries except columnIndex
        memcpy( x, tempY, sizeof(double) * _m );

        // Compute the special column
        unsigned columnIndex = (*eta)->_columnIndex;
        x[columnIndex] = tempY[columnIndex];
        for ( unsigned i = 0; i < _m; ++i )
        {
            if ( i != columnIndex )
                x[columnIndex] -= (x[i] * (*eta)->_column[i]);
        }
        x[columnIndex] = x[columnIndex] / (*eta)->_column[columnIndex];

        // To handle -0.0
        if ( FloatUtils::isZero( x[columnIndex] ) )
            x[columnIndex] = 0.0;

        // The x from this iteration becomes y for the next iteration
        memcpy( tempY, x, sizeof(double) *_m );
    }

	if  (!_start) {
		//std::cout << "Calculated U" << std::endl;
		factorization( _B0 );
		x[0] = tempY[0];
		for (unsigned int i = 1; i < _m; ++i) {
			double sum = 0.;
			for (unsigned int j = 0; j < i; j++) {
				sum += _U[i+j*_m] * x[j];
			}
			x[i] = tempY[i] - sum;
		}
	}
	
	for (double *d : _LP){
		//std::cout << "Calculated LP" << std::endl;
		if (d != _I){
			double temp[_m];
			LPMultiplyBack ( x, d, temp );
			memcpy ( x, temp, sizeof(double) * _m );
		}
		delete[] d;
	}
}

void BasisFactorization::storeFactorization( BasisFactorization *other ) const
{
    ASSERT( _m == other->_m );
    ASSERT( other->_etas.size() == 0 );

    memcpy( other->_B0, _B0, sizeof(double) * _m * _m );

    for ( const auto &eta : _etas )
        other->pushEtaMatrix( eta->_columnIndex, eta->_column );
}

void BasisFactorization::restoreFactorization( const BasisFactorization *other )
{
    ASSERT( _m == other->_m );

    for ( const auto &it : _etas )
        delete it;

    _etas.clear();

    memcpy( _B0, other->_B0, sizeof(double) * _m * _m );

    for ( const auto &eta : other->_etas )
        _etas.append( new EtaMatrix( eta->_m, eta->_columnIndex, eta->_column ) );
}

void BasisFactorization::coutMatrix( const double*m ){
   std::cout<<'\n';
   for (unsigned int i = 0; i < _m; ++i){
      for (unsigned int j = 0; j < _m; ++j)	std::cout << std::setw(14) << m[i*_m+j];
      std::cout<<'\n';
   }
}
void BasisFactorization::rowSwap ( int p, int n, double *A){
    double buf = 0;
    for (unsigned int i = 0; i < _m; i++){
	    buf = A[p*_m+i];
	    A[p*_m+i] = A[n*_m+i];
        A[n*_m+i] = buf;
    }
}

void BasisFactorization::clear ()
{
	queue<double *>().swap(_LPd);
	_LP.clear();
	std::fill_n (_U, _m*_m, 0);
}

void BasisFactorization::factorization( double *S ){   
	clear();
	for (unsigned int i = 0; i < _m; ++i){
		double *P = new double[_m*_m];
		double *L = new double[_m*_m];
		std::fill_n (P, _m*_m, 0);
		std::fill_n (L, _m*_m, 0);
		for (unsigned int a = 0; a < _m; ++a){
				P[a*_m+a] = 1.;  
				L[a*_m+a] = 1.;
		}
		if (S[i*_m+i] == 0){
			unsigned int tempi = i;
			while (S[tempi*_m+i] == 0 && tempi < _m) tempi++;
			if (tempi == _m) throw ReluplexError( ReluplexError::NO_AVAILABLE_CANDIDATES, "No Pivot" ); 
			rowSwap(i, tempi, S);
			rowSwap(i, tempi, P);
		}
		double div = S[i*_m+i];
		for (unsigned int j = i; j < _m; ++j){
			if (j == i) L[j*_m+i] = 1/div;
			else L[j*_m+i] = -S[i+j*_m]/div;   
		}
		if (P != _I)	_LP.insert(_LP.begin(), P);
		_LP.insert(_LP.begin(), L);
		_LPd.push(P);
		_LPd.push(L);
		double R[_m*_m];
		matrixMultiply(L, S, R);
		memcpy(S, R, sizeof(double)*_m*_m);
	}
	memcpy(_U, S, sizeof(double)*_m*_m);
}
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
