
#ifndef __LPElement_h__
#define __LPElement_h__
#include <utility>

class EtaMatrix;


class LPElement 
{
public:
	LPElement( const EtaMatrix *_eta, const std::pair<int, int> *_pair, bool isP );
	~LPElement();

	EtaMatrix *_eta;
	std::pair<int, int>  *_pair;
	bool _isP;
};

#endif //__LPElement_h__
