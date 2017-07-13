
#ifndef __LPContainer_h__
#define __LPContainer_h__
#include <utility>

class EtaMatrix;


class LPContainer 
{
public:
	LPContainer( void *X, bool isP );
	~LPContainer();

	EtaMatrix *_eta;
	std::pair<int, int>  *_pair;
	bool _isP;
};

#endif //__LPContainer_h__
