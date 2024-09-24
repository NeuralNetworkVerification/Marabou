/*********************                                                        */
/*! \file QueryLoader.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Christopher Lazarus, Kyle Julian, Teruhiro Tagomori
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief [[ Add one-line brief description here ]]
 **
 ** [[ Add lengthier description here ]]
 **/

#include "QueryLoader.h"

#include "AutoFile.h"
#include "BilinearConstraint.h"
#include "Debug.h"
#include "DisjunctionConstraint.h"
#include "Equation.h"
#include "GlobalConfiguration.h"
#include "IQuery.h"
#include "LeakyReluConstraint.h"
#include "MStringf.h"
#include "MarabouError.h"
#include "MaxConstraint.h"
#include "ReluConstraint.h"
#include "RoundConstraint.h"
#include "SignConstraint.h"
#include "SoftmaxConstraint.h"

void QueryLoader::loadQuery( const String &fileName, IQuery &inputQuery )
{
    if ( !IFile::exists( fileName ) )
    {
        throw MarabouError( MarabouError::FILE_DOES_NOT_EXIST,
                            Stringf( "File %s not found.\n", fileName.ascii() ).ascii() );
    }

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
    QL_LOG( Stringf( "Number of non-linear constraints: %u\n", numConstraints ).ascii() );

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
    for ( unsigned i = 0; i < numEquations; ++i )
    {
        QL_LOG( Stringf( "Equation: %u ", i ).ascii() );
        String line = input->readLine();

        List<String> tokens = line.tokenize( "," );
        ASSERT( tokens.size() > 4 );

        auto it = tokens.begin();

        // Skip equation number
        ++it;
        int eqType = atoi( it->ascii() );
        QL_LOG( Stringf( "Type: %u ", eqType ).ascii() );
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
            throw MarabouError( MarabouError::INVALID_EQUATION_TYPE,
                                Stringf( "Invalid Equation Type\n" ).ascii() );
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

    // Non-Linear(Piecewise and Nonlinear) Constraints
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
        while ( it != tokens.end() )
        {
            serializeConstraint += *it + String( "," );
            it++;
        }
        serializeConstraint = serializeConstraint.substring( 0, serializeConstraint.length() - 1 );

        QL_LOG( Stringf( "Non-Linear Constraint: %u, Type: %s \n", i, coType.ascii() ).ascii() );
        QL_LOG( Stringf( "\tserialized:\t%s \n", serializeConstraint.ascii() ).ascii() );
        if ( coType == "relu" )
        {
            inputQuery.addPiecewiseLinearConstraint( new ReluConstraint( serializeConstraint ) );
        }
        else if ( coType == "leaky_relu" )
        {
            inputQuery.addPiecewiseLinearConstraint(
                new LeakyReluConstraint( serializeConstraint ) );
        }
        else if ( coType == "max" )
        {
            inputQuery.addPiecewiseLinearConstraint( new MaxConstraint( serializeConstraint ) );
        }
        else if ( coType == "absoluteValue" )
        {
            inputQuery.addPiecewiseLinearConstraint(
                new AbsoluteValueConstraint( serializeConstraint ) );
        }
        else if ( coType == "sign" )
        {
            inputQuery.addPiecewiseLinearConstraint( new SignConstraint( serializeConstraint ) );
        }
        else if ( coType == "disj" )
        {
            inputQuery.addPiecewiseLinearConstraint(
                new DisjunctionConstraint( serializeConstraint ) );
        }
        else if ( coType == "sigmoid" )
        {
            inputQuery.addNonlinearConstraint( new SigmoidConstraint( serializeConstraint ) );
        }
        else if ( coType == "softmax" )
        {
            SoftmaxConstraint *softmax = new SoftmaxConstraint( serializeConstraint );
            inputQuery.addNonlinearConstraint( softmax );
            Equation eq;
            for ( const auto &output : softmax->getOutputs() )
                eq.addAddend( 1, output );
            eq.setScalar( 1 );
            inputQuery.addEquation( eq );
        }
        else if ( coType == "bilinear" )
        {
            BilinearConstraint *bilinear = new BilinearConstraint( serializeConstraint );
            inputQuery.addNonlinearConstraint( bilinear );
        }
        else if ( coType == "round" )
        {
            inputQuery.addNonlinearConstraint( new RoundConstraint( serializeConstraint ) );
        }
        else
        {
            throw MarabouError(
                MarabouError::UNSUPPORTED_NON_LINEAR_CONSTRAINT,
                Stringf( "Unsupported non-linear constraint: %s\n", coType.ascii() ).ascii() );
        }
    }
}
