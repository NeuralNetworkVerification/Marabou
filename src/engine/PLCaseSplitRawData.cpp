#include "PLCaseSplitRawData.h"
#include <tuple>

PLCaseSplitRawData::PLCaseSplitRawData() {}

PLCaseSplitRawData::PLCaseSplitRawData(unsigned int b, unsigned int f, ActivationType activation) {
    _b = b; 
    _f = f; 
    _activation = activation;
}

bool PLCaseSplitRawData::operator==(PLCaseSplitRawData const &o) const
{
    return (_f == o._f) && (_b == o._b) && (_activation == o._activation);
}

bool PLCaseSplitRawData::operator<(PLCaseSplitRawData const &o) const
{
    return std::make_tuple(_f, _b, _activation) < std::make_tuple(o._f, o._b, o._activation);
}

