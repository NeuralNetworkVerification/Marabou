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
#include "GlobalConfiguration.h"
#include "LPElement.h"

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
	_factorizationEnabled = true;
    if ( !_B0 )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "BasisFactorization::B0" );
	if ( !_U )
        throw ReluplexError( ReluplexError::ALLOCATION_FAILED, "BasisFactorization::U" );
}

BasisFactorization::~BasisFactorization()
{
    freeIfNeeded();
}

void BasisFactorization::freeIfNeeded()
{
    if ( _U )
    {
        delete[] _U;
        _U = NULL;
    }
	if ( _B0 )
	{
		delete[] _B0;
		_B0 = NULL;
	}


    List<EtaMatrix *>::iterator it;
    for ( it = _etas.begin(); it != _etas.end(); ++it )
        delete *it;

    _etas.clear();

	for (unsigned  i = 0; i < _LP.size(); i++)
		delete _LP.at(i);	

	_LP.clear();
}

const double *BasisFactorization::getU() const
{
	return _U;
}

const std::vector<LPElement *> BasisFactorization::getLP() const 
{
	return _LP;
}

const double *BasisFactorization::getB0() const
{
	return _B0;
}

const List<EtaMatrix *> BasisFactorization::getEtas() const
{
	return _etas;
}

void BasisFactorization::pushEtaMatrix( unsigned columnIndex, double *column )
{
    EtaMatrix *matrix = new EtaMatrix( _m, columnIndex, column );
    _etas.append( matrix );
	if ( _etas.size() > GlobalConfiguration::REFACTORIZATION_THRESHOLD && _factorizationEnabled )
	{
		condenseEtas();
		factorization( _B0 );
	}
}


void BasisFactorization::matrixMultiply( const double *L, const double *B, double *R){
	for (unsigned n = 0; n < _m; ++n){ 
		for (unsigned i = 0; i < _m; ++i){ 
			double sum = 0.;
			for (unsigned k = 0; k < _m; ++k) 
				sum += L[i*_m+k] * B[n+k*_m];
			R[i*_m+n]=sum;
		}
	}
}

void BasisFactorization::LFactorizationMultiply( const EtaMatrix *L, double *_U )
{
    unsigned colIndex = L->_columnIndex;
    for (unsigned col = colIndex; col < _m; ++col) {
        for (unsigned row = colIndex + 1; row < _m; ++row){
			_U[row*_m+col] += L->_column[row]*_U[colIndex*_m+col];
        }
    }
	for (unsigned i = colIndex; i < _m; ++i) {
		_U[colIndex*_m+i] *= L->_column[colIndex];
	}
}

void BasisFactorization::LMultiplyRight( const EtaMatrix *L, const double *X, double *R)
{
	memcpy( R, X, sizeof(double) * _m );
	double sum = 0.;
	for (unsigned i = 0; i < _m; ++i) {
		sum += L->_column[i] * X[i];
	}
	R[L->_columnIndex] = sum;
}

void BasisFactorization::LMultiplyLeft( const EtaMatrix *L, const double *X, double *R)
{
	unsigned col = L->_columnIndex;
		for (unsigned j = 0; j < _m; ++j){
			if ( j == col ) {
				R[j] = X[j] * L->_column[col];
			} else {
				R[j] = X[j] + X[col] * L->_column[j];
			}
		}
}

void BasisFactorization::setB0( const double *B0 )
{
	memcpy ( _B0, B0, sizeof(double) * _m * _m);
	factorization( _B0 );
}

void BasisFactorization::condenseEtas()
{
	double *Y = new double[_m*_m];
    if ( _LP.empty() ) { 
        std::fill_n( _B0, _m * _m, 0. ); 
        for (unsigned row = 0; row < _m; ++row) { 
            _B0[row*_m+row] = 1.; 
        }
    }	
	for ( auto &eta : _etas) {
		memcpy ( Y, _B0, sizeof(double) * _m * _m );
		for (unsigned n = 0; n < _m; ++n){ //columns
				double sum = 0.;
				for (unsigned i = 0; i < _m; ++i){ // rows of L
						sum += Y[n*_m+i] * eta->_column[i];
				}
				_B0[n*_m+eta->_columnIndex] = sum;
		}
		delete eta;
	}	
	delete[] Y;
	_etas.clear();
	clearLPU();
}

void BasisFactorization::forwardTransformation( const double *y, double *x )
{
    if ( _etas.empty() && _LP.empty() )
    {
        memcpy( x, y, sizeof(double) * _m );
        return;
    }
    double *tempY = new double[_m];
    memcpy( tempY, y, sizeof(double) * _m );
    std::fill( x, x + _m, 0.0 );

	for ( int i = _LP.size() - 1; i >= 0; --i) { //multiplication by U 
		auto container = _LP.at(i);
		if ( container->_isP ) {
			double temp = x[container->_pair->first];
			x[container->_pair->first] = x[container->_pair->second];
			x[container->_pair->second] = temp;
		} else {
			double temp[_m];
			LMultiplyLeft( container->_eta , tempY, temp);
			memcpy ( tempY, temp, sizeof(double) * _m);
		}
	}

	if ( !_LP.empty() ) { //multiplication by L and P
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
	
	for ( const auto &eta : _etas ) //multiplication by etas
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
    if ( _etas.empty() && _LP.empty() )
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

	if ( !_LP.empty() ) //multiplication by U
	{
		x[0] = tempY[0];
		for (unsigned i = 1; i < _m; ++i) {
			double sum = 0.;
			for (unsigned j = 0; j < i; j++) {
				sum += _U[i+j*_m] * x[j];
			}
			x[i] = tempY[i] - sum;
		}
	}
 
	for (auto *d : _LP) //multiplication by L and P
	{
		if ( d->_isP ){
			double temp = x[d->_pair->first];
			x[d->_pair->first] = x[d->_pair->second];
			x[d->_pair->second] = temp;
		} else {
			double temp[_m];
			LMultiplyRight ( d->_eta, x , temp );
			memcpy ( x, temp, sizeof(double) * _m );
		}
		
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

void BasisFactorization::coutMatrix( const double*m )
{
   std::cout<<'\n';
   for (unsigned i = 0; i < _m; ++i){
      for (unsigned j = 0; j < _m; ++j)	std::cout << std::setw(14) << m[i*_m+j];
      std::cout<<'\n';
   }
}
void BasisFactorization::rowSwap( int p, int n, double *A ){
    double buf = 0;
    for (unsigned i = 0; i < _m; i++){
	    buf = A[p*_m+i];
	    A[p*_m+i] = A[n*_m+i];
        A[n*_m+i] = buf;
    }
}


void BasisFactorization::clearLPU()
{
	for (unsigned  i = 0; i < _LP.size(); i++)
		delete _LP.at(i);	

	_LP.clear();
	std::fill_n( _U, _m*_m, 0 );
}

void BasisFactorization::factorization( double *S )
{   
	clearLPU();
	memcpy ( _U, S, sizeof(double) * _m * _m );

	for (unsigned i = 0; i < _m; ++i){
		std::pair <int, int> *P = new std::pair <int, int>;
		double *L_col = new double[_m];
	
		if (_U[i*_m+i] == 0){ //check for pivot
			unsigned tempi = i;
			
			while (_U[tempi*_m+i] == 0 && tempi < _m) tempi++;
			
			if (tempi == _m) throw ReluplexError( ReluplexError::NO_AVAILABLE_CANDIDATES, "No Pivot" ); 
			
			rowSwap(i, tempi, _U);
			P->first = i;
			P->second = tempi;
			LPElement *P_elem = new LPElement( NULL, P, true );
			_LP.insert( _LP.begin(), P_elem ); //add permutation matrix only if necessary
		}
	
		double div = _U[i*_m+i];
		for (unsigned j = i; j < _m; ++j){ //Gaussian elimination
			if (j == i) L_col[j] = 1/div;
			else L_col[j] = -_U[i+j*_m]/div;   
		}
    	
		EtaMatrix *L = new EtaMatrix( _m, i, L_col );
		LPElement *L_elem = new LPElement( L, NULL, false);
		_LP.insert( _LP.begin(), L_elem );
			
		LFactorizationMultiply( L, _U );
	}
}
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
