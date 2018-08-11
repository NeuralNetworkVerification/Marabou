#include "Debug.h"
#include "Equation.h"
#include "File.h"
#include "InputQuery.h"
#include "MStringf.h"
#include "MaxConstraint.h"
#include "QueryLoader.h"
#include "ReluConstraint.h"

// TODO: add a unit test for a sanity check

InputQuery QueryLoader::loadQuery( const String &fileName )
{
    if ( !File::exists( fileName ) )
    {
        printf("File %s not found.\n", fileName.ascii());
    }

    InputQuery inputQuery;
    File input = File( fileName );
    input.open( File::MODE_READ );

    unsigned numVars = atoi( input.readLine().trim().ascii() );
    unsigned numBounds = atoi( input.readLine().trim().ascii() );
    unsigned numEquations = atoi( input.readLine().trim().ascii() );
    unsigned numConstraints = atoi( input.readLine().trim().ascii() );

     printf("Number of variables: %u\n", numVars);
     printf("Number of bounds: %u\n", numBounds);
     printf("Number of equations: %u\n", numEquations);
     printf("Number of constraints: %u\n", numConstraints);

    inputQuery.setNumberOfVariables( numVars );

    // Bounds
    for ( unsigned i = 0; i < numBounds; ++i )
    {   
        printf("Bound: %u\n", i);
        // First bound
        String line = input.readLine();
        List<String> tokens = line.tokenize( "," );

        // Todo: check something about the number of tokens

        auto it = tokens.begin();
        unsigned varToBound = atoi( it->ascii() );
        ++it;

        double lb = atof( it->ascii() );
        ++it;

        double ub = atof( it->ascii() );
        ++it;

        printf("Var: %u, L: %f, U: %f\n", varToBound, lb, ub);
        inputQuery.setLowerBound( varToBound, lb );
        inputQuery.setUpperBound( varToBound, ub );
    }

    // Equations
    for( unsigned i = 0; i< numEquations; ++i )
    {
        printf("Equation: %u ", i);
        String line = input.readLine();

        List<String> tokens = line.tokenize( "," );
        auto it = tokens.begin();

        // Skip equation number
        ++it;
        int eqType = atoi( it->ascii() );
        printf("Type: %u ", eqType);
        ++it;
        double eqScalar = atof( it->ascii() );
        printf("Scalar: %f\n", eqScalar);

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
            // Todo: add error handling
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

            printf("\tVar_no: %i, Coeff: %f\n", varNo, coeff);

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
        while ( ++it != tokens.end() ) {
            serializeConstraint += *it + String(",");
        }
        serializeConstraint = serializeConstraint.substring(0, serializeConstraint.length()-1);
        //++it;
        

        PiecewiseLinearConstraint *constraint = NULL;
        printf("Constraint: %u, Type: %s \n", i, coType.ascii());
        printf("\tserialized:\t%s \n", serializeConstraint.ascii());
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
            // Todo: Unsupport constraint, add error handling
        }


        ASSERT( constraint );
        inputQuery.addPiecewiseLinearConstraint( constraint );
    }

    return inputQuery;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
