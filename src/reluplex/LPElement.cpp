#include "EtaMatrix.h"
#include "LPElement.h"

#include <utility>
#include <cstdio>
#include <cstdlib>
#include <cstring>

LPElement::LPElement( const EtaMatrix *eta, const std::pair<int, int> *pair, bool isP ) 
	: _eta( NULL )
	, _pair( NULL )
{
	_isP = isP;
	if ( isP ){
		_pair = new std::pair<int, int>;
		memcpy( _pair, pair, sizeof(std::pair<int, int>) );
	} else 
		_eta = new EtaMatrix( eta->_m, eta->_columnIndex, eta->_column );
}

LPElement::~LPElement()
{
	if ( _eta )
	{
		delete _eta;
		_eta = NULL;
	}
	if ( _pair )
	{
		delete _pair;
		_pair = NULL;
	}
}

