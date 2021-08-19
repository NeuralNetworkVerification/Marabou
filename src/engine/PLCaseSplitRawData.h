#ifndef PLCASESPLITRAWDATA_H
#define PLCASESPLITRAWDATA_H

/*
    case split raw information (variables and activation)
*/
enum class ActivationType
{
    INACTIVE = -1,
    ACTIVE = 1,
};

struct PLCaseSplitRawData
{

    PLCaseSplitRawData();
    PLCaseSplitRawData( unsigned int b, unsigned int f, ActivationType activation );
    unsigned int _f;
    unsigned int _b;
    ActivationType _activation;

    bool operator==( PLCaseSplitRawData const& o ) const;
    bool operator<( PLCaseSplitRawData const& o ) const;
};

#endif // PLCASESPLITRAWDATA_H
