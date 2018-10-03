#include "Debug.h"
#include "Equation.h"
#include "File.h"
#include "InputQuery.h"
#include "MStringf.h"
#include "MaxConstraint.h"
#include "QueryLoader.h"
#include "ReluConstraint.h"
#include "GlobalConfiguration.h"
#include "ReluplexError.h"
// TODO: add a unit test for a sanity check

InputQuery QueryLoader::loadQuery( const String &fileName )
{
    if ( !File::exists( fileName ) )
    {
        throw ReluplexError(ReluplexError::FILE_DOES_NOT_EXIST, Stringf( "File %s not found.\n", fileName.ascii() ).ascii() );
    }

    InputQuery inputQuery;
    File input = File( fileName );
    input.open( File::MODE_READ );

    unsigned numVars = atoi( input.readLine().trim().ascii() );
    unsigned numBounds = atoi( input.readLine().trim().ascii() );
    unsigned numEquations = atoi( input.readLine().trim().ascii() );
    unsigned numConstraints = atoi( input.readLine().trim().ascii() );

    log(Stringf("Number of variables: %u\n", numVars));
    log(Stringf("Number of bounds: %u\n", numBounds));
    log(Stringf("Number of equations: %u\n", numEquations));
    log(Stringf("Number of constraints: %u\n", numConstraints));

    inputQuery.setNumberOfVariables( numVars );

    // Bounds
    for ( unsigned i = 0; i < numBounds; ++i )
    {   
        log(Stringf("Bound: %u\n", i));
        String line = input.readLine();
        List<String> tokens = line.tokenize( "," );

        // format: <var, lb, ub>
        assert(tokens.size() == 3);

        auto it = tokens.begin();
        unsigned varToBound = atoi( it->ascii() );
        ++it;

        double lb = atof( it->ascii() );
        ++it;

        double ub = atof( it->ascii() );
        ++it;

        log(Stringf("Var: %u, L: %f, U: %f\n", varToBound, lb, ub));
        inputQuery.setLowerBound( varToBound, lb );
        inputQuery.setUpperBound( varToBound, ub );
    }

    // Equations
    for( unsigned i = 0; i< numEquations; ++i )
    {
        log(Stringf("Equation: %u ", i));
        String line = input.readLine();

        List<String> tokens = line.tokenize( "," );
        assert(tokens.size() > 4);
        
        auto it = tokens.begin();

        // Skip equation number
        ++it;
        int eqType = atoi( it->ascii() );
        log(Stringf("Type: %u ", eqType));
        ++it;
        double eqScalar = atof( it->ascii() );
        log(Stringf("Scalar: %f\n", eqScalar));

        Equation::EquationType type = Equation::EQ;

        switch ( eqType )
        {
        case 0:
            type = Equation::EQ;
            break;

        case 1:
            type = Equation::GE;
            break;

        case 2:
            type = Equation::LE;
            break;

        default:
            // Throw exception
            throw ReluplexError(ReluplexError::INVALID_EQUATION_TYPE, Stringf("Invalid Equation Type\n").ascii() );
            break;
        }

        Equation equation( type );
        equation.setScalar( eqScalar );

        while ( ++it != tokens.end() )
        {
            int varNo = atoi( it->ascii() );
            ++it;
            ASSERT( it != tokens.end() );
            double coeff = atof( it->ascii() );

            log(Stringf("\tVar_no: %i, Coeff: %f\n", varNo, coeff));

            equation.addAddend( coeff, varNo );
        }
        inputQuery.addEquation( equation );

    }

    // Constraints
    for( unsigned i = 0; i < numConstraints; ++i )
    {
        String line = input.readLine();

        List<String> tokens = line.tokenize( "," );
        auto it = tokens.begin();

        // Skip constraint number
        ++it;
        String coType = *it;
        String serializeConstraint;
        // include type in serializeConstraint as well
        while ( it != tokens.end() ) {
            serializeConstraint += *it + String(",");
            it++;
        }
        serializeConstraint = serializeConstraint.substring(0, serializeConstraint.length()-1);

        PiecewiseLinearConstraint *constraint = NULL;
        log(Stringf("Constraint: %u, Type: %s \n", i, coType.ascii()));
        log(Stringf("\tserialized:\t%s \n", serializeConstraint.ascii()));
        if ( coType == "relu" )
        {
            constraint = new ReluConstraint( serializeConstraint );
        }
        else if ( coType == "max" )
        {
            constraint = new MaxConstraint( serializeConstraint );
        }
        else
        {
            throw ReluplexError(ReluplexError::UNSUPPORTED_PIECEWISE_CONSTRAINT, Stringf("UNSUPPORTED PIECEWISE CONSTRAINT: %s\n", coType.ascii()).ascii() );
        }

        ASSERT( constraint );
        inputQuery.addPiecewiseLinearConstraint( constraint );
    }

    return inputQuery;
}

void QueryLoader::log( const String &message )
{
    if ( GlobalConfiguration::QUERY_LOADER_LOGGING )
        printf( "Engine: %s\n", message.ascii() );
}
//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
