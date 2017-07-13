#include "EtaMatrix.h"
#include "LPContainer.h"

#include <utility>
#include <cstdio>
#include <cstdlib>
#include <cstring>

LPContainer::LPContainer( void *X, bool isP ) 
	: _eta( NULL )
	, _pair( NULL )
{
	_isP = isP;
	if ( isP )
		_pair = (std::pair<int, int> *)  X;
	else 
		_eta = (EtaMatrix *) X;
}

LPContainer::~LPContainer()
{
	if ( _pair )
	{
		delete[] _pair;
		_pair = NULL;
	} 
	if ( _eta )
	{
		delete[] _eta;
		_eta = NULL;
	}
}

