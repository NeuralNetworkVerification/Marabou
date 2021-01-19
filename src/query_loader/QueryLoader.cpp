/*********************                                                        */
/*! \file QueryLoader.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Christopher Lazarus, Kyle Julian
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief [[ Add one-line brief description here ]]
 **
 ** [[ Add lengthier description here ]]
 **/

#include "AutoFile.h"
#include "Debug.h"
#include "Equation.h"
#include "GlobalConfiguration.h"
#include "InputQuery.h"
#include "MStringf.h"
#include "MarabouError.h"
#include "MaxConstraint.h"
#include "QueryLoader.h"
#include "ReluConstraint.h"
#include "SignConstraint.h"

InputQuery QueryLoader::loadQuery( const String &fileName )
{
    if ( !IFile::exists( fileName ) )
    {
        throw MarabouError( MarabouError::FILE_DOES_NOT_EXIST, Stringf( "File %s not found.\n", fileName.ascii() ).ascii() );
    }

    InputQuery inputQuery;
    AutoFile input( fileName );
    input->open( IFile::MODE_READ );

    unsigned numVars = atoi( input->readLine().trim().ascii() );
    unsigned numLowerBounds = atoi( input->readLine().trim().ascii() );
    unsigned numUpperBounds = atoi( input->readLine().trim().ascii() );
    unsigned numEquations = atoi( input->readLine().trim().ascii() );
    unsigned numConstraints = atoi( input->readLine().trim().ascii() );

    QL_LOG( Stringf( "Number of variables: %u\n", numVars ).ascii() );
    QL_LOG( Stringf( "Number of lower bounds: %u\n", numLowerBounds ).ascii() );
    QL_LOG( Stringf( "Number of upper bounds: %u\n", numUpperBounds ).ascii() );
    QL_LOG( Stringf( "Number of equations: %u\n", numEquations ).ascii() );
    QL_LOG( Stringf( "Number of constraints: %u\n", numConstraints ).ascii() );

    inputQuery.setNumberOfVariables( numVars );

    // Input Variables
    unsigned numInputVars = atoi( input->readLine().trim().ascii() );
    for ( unsigned i = 0; i < numInputVars; ++i )
    {
        String line = input->readLine();
        List<String> tokens = line.tokenize( "," );
        auto it = tokens.begin();
        unsigned inputIndex = atoi( it->ascii() );
        it++;
        unsigned variable = atoi( it->ascii() );
        it++;
        inputQuery.markInputVariable( variable, inputIndex );
    }

    // Output Variables
    unsigned numOutputVars = atoi( input->readLine().trim().ascii() );
    for ( unsigned i = 0; i < numOutputVars; ++i )
    {
        String line = input->readLine();
        List<String> tokens = line.tokenize( "," );
        auto it = tokens.begin();
        unsigned outputIndex = atoi( it->ascii() );
        it++;
        unsigned variable = atoi( it->ascii() );
        it++;
        inputQuery.markOutputVariable( variable, outputIndex );
    }

    // Lower Bounds
    for ( unsigned i = 0; i < numLowerBounds; ++i )
    {
        QL_LOG( Stringf( "Bound: %u\n", i ).ascii() );
        String line = input->readLine();
        List<String> tokens = line.tokenize( "," );

        // format: <var, lb>
        ASSERT( tokens.size() == 2 );

        auto it = tokens.begin();
        unsigned varToBound = atoi( it->ascii() );
        ++it;

        double lb = atof( it->ascii() );
        ++it;

        QL_LOG( Stringf( "Var: %u, L: %f\n", varToBound, lb ).ascii() );
        inputQuery.setLowerBound( varToBound, lb );
    }

    // Upper Bounds
    for ( unsigned i = 0; i < numUpperBounds; ++i )
    {
        QL_LOG( Stringf( "Bound: %u\n", i ).ascii() );
        String line = input->readLine();
        List<String> tokens = line.tokenize( "," );

        // format: <var, ub>
        ASSERT( tokens.size() == 2 );

        auto it = tokens.begin();
        unsigned varToBound = atoi( it->ascii() );
        ++it;

        double ub = atof( it->ascii() );
        ++it;

        QL_LOG( Stringf( "Var: %u, U: %f\n", varToBound, ub ).ascii() );
        inputQuery.setUpperBound( varToBound, ub );
    }

    // Equations
    for( unsigned i = 0; i < numEquations; ++i )
    {
        QL_LOG( Stringf( "Equation: %u ", i ).ascii() );
        String line = input->readLine();

        List<String> tokens = line.tokenize( "," );
        ASSERT( tokens.size() > 4 );

        auto it = tokens.begin();

        // Skip equation number
        ++it;
        int eqType = atoi( it->ascii() );
        QL_LOG( Stringf("Type: %u ", eqType ).ascii() );
        ++it;
        double eqScalar = atof( it->ascii() );
        QL_LOG( Stringf( "Scalar: %f\n", eqScalar ).ascii() );

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
            throw MarabouError( MarabouError::INVALID_EQUATION_TYPE, Stringf( "Invalid Equation Type\n" ).ascii() );
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

            QL_LOG( Stringf( "\tVar_no: %i, Coeff: %f\n", varNo, coeff ).ascii() );

            equation.addAddend( coeff, varNo );
        }

        inputQuery.addEquation( equation );
    }

    // Constraints
    for ( unsigned i = 0; i < numConstraints; ++i )
    {
        String line = input->readLine();

        List<String> tokens = line.tokenize( "," );
        auto it = tokens.begin();

        // Skip constraint number
        ++it;
        String coType = *it;
        String serializeConstraint;
        // include type in serializeConstraint as well
        while ( it != tokens.end() ) {
            serializeConstraint += *it + String( "," );
            it++;
        }
        serializeConstraint = serializeConstraint.substring( 0, serializeConstraint.length() - 1 );

        PiecewiseLinearConstraint *constraint = NULL;
        QL_LOG( Stringf( "Constraint: %u, Type: %s \n", i, coType.ascii() ).ascii() );
        QL_LOG( Stringf( "\tserialized:\t%s \n", serializeConstraint.ascii() ).ascii() );
        if ( coType == "relu" )
        {
            constraint = new ReluConstraint( serializeConstraint );
        }
        else if ( coType == "max" )
        {
            serializeConstraint += String(",e,0,0");
            constraint = new MaxConstraint( serializeConstraint );
        }
        else if ( coType == "absoluteValue" )
        {
            constraint = new AbsoluteValueConstraint( serializeConstraint );
        }
        else if ( coType == "sign" )
        {
            constraint = new SignConstraint( serializeConstraint );
        }
        else
        {
            throw MarabouError( MarabouError::UNSUPPORTED_PIECEWISE_LINEAR_CONSTRAINT, Stringf( "Unsupported piecewise constraint: %s\n", coType.ascii() ).ascii() );
        }

        ASSERT( constraint );
        inputQuery.addPiecewiseLinearConstraint( constraint );
    }

    inputQuery.constructNetworkLevelReasoner();
    return inputQuery;
}


//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
