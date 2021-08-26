#include "GammaUnsat.h"
#include <fstream>
#include "MString.h"

GammaUnsat::UnsatSequence::UnsatSequence() {}

ActivationType deserializeActivation( int serializedAct )
{
    switch ( serializedAct )
    {
    case 1:
        return ActivationType::ACTIVE;
    case -1:
        return ActivationType::INACTIVE;
    default:
        throw std::runtime_error( "invalid serialized activation value of: " + std::to_string( serializedAct ) );
    }
}

int serializeActivation( ActivationType act )
{
    switch ( act )
    {
    case ActivationType::ACTIVE:
        return 1;
    case ActivationType::INACTIVE:
        return -1;
    }
}

GammaUnsat GammaUnsat::readFromFile( std::string const& path ) {
    std::ifstream ifs{ path };

    GammaUnsat gammaUnsat;
    std::string sLine;
    UnsatSequence clause;
    while ( std::getline( ifs, sLine ) )
    {
        auto const line = String( sLine ).trim();

        if ( line.length() == 0 || line[0] == '#' ) {
            continue;
        }
        else if ( line[0] == '*' )
        {
            gammaUnsat.addUnsatSequence( clause );
            clause.activations.clear();
            continue;
        }
        else
        {
            auto const parts = line.tokenize( "," );
            if ( parts.size() != 3 ) {
                throw std::runtime_error( "invalid line: " + std::string( line.ascii() ) );
            }
            auto it = parts.begin();
            unsigned const b = std::stoul( ( it++ )->ascii() );
            unsigned const f = std::stoul( ( it++ )->ascii() );
            int const a = std::stoi( ( it++ )->ascii() );
            auto const activation = deserializeActivation( a );
            clause.activations.append( PLCaseSplitRawData( b, f, activation ) );
            continue;
        }
    }
    return gammaUnsat;
}

void GammaUnsat::saveToFile( std::string const& path ) const {
    std::ofstream ofs{ path };

    // write header explains the file
    auto const fileHead = (
        "# Gamma Unsat file format:\n"
        "# each line contains a single 'literal' of a gamma clause, in the format of 'b,f,activation', where:\n"
        "# b = b-variable of a neuron; f = f-variable of a neuron; activation of 1 means 'active' while -1 means 'inactive'\n"
        "# clauses are separated by '*' (single asterisk)\n"
        );
    ofs << fileHead;

    for ( auto const& clause : unsatSequences ) {
        for ( auto const& csRawData : clause.activations ) {
            ofs << csRawData._b <<
                "," << csRawData._f <<
                "," << serializeActivation( csRawData._activation ) <<
                "\n";
        }
        ofs << "*\n";
    }
}


GammaUnsat::GammaUnsat() {}

GammaUnsat::~GammaUnsat() {}

void GammaUnsat::addUnsatSequence( UnsatSequence unsatSeq ) {
    unsatSequences.append( unsatSeq );
}

List<GammaUnsat::UnsatSequence> GammaUnsat::getUnsatSequences() const {
    return unsatSequences;
}
