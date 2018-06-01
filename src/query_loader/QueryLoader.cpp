#include "Equation.h"
#include "InputQuery.h"
#include "MStringf.h"
#include "MaxConstraint.h"
#include "QueryLoader.h"
#include "ReluConstraint.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

// TODO: please use our File.h and MString.h classes
// for file access and string manipulation

// TODO: add a unit test for a sanity check

InputQuery load_query(const char* filename){
    InputQuery inputquery;
    FILE *fstream = fopen(filename,"r");
    if (fstream == NULL)
    {
        return inputquery;
    }

    int bufferSize = 10240;

    // Guy: TODO: should buffer be deleted afterwards?
    char *buffer = new char[bufferSize];

    char *record, *line;
    //int i=0, layer=0, row=0, j=0, param=0;
    //AcasNnet *nnet = new AcasNnet();
    line=fgets(buffer,bufferSize,fstream);
    record = strtok(line,"\n");
    int numVars = atoi(record);

    line=fgets(buffer,bufferSize,fstream);
    record = strtok(line,"\n");
    int numBounds = atoi(record);

    line=fgets(buffer,bufferSize,fstream);
    record = strtok(line,"\n");
    int numEquations = atoi(record);

    line=fgets(buffer,bufferSize,fstream);
    record = strtok(line,"\n");
    int numConstraints = atoi(record);

    //printf("Number of variables: %u\n", numVars);
    //printf("Number of bounds: %u\n", numBounds);
    //printf("Number of equations: %u\n", numEquations);
    //printf("Number of constraints: %u\n", numConstraints);

    inputquery.setNumberOfVariables(numVars);

    // bounds
    for (int i=0; i<numBounds; i++){
        //first bound
        line=fgets(buffer,bufferSize,fstream);
        int var_to_bound = atoi(strtok(line,","));
        double l = atof(strtok(NULL,","));
        double u = atof(strtok(NULL,","));

        //printf("Var: %u L:%f, U:%f\n", var_to_bound,l,u);

        inputquery.setLowerBound(var_to_bound, l);
        inputquery.setUpperBound(var_to_bound, u);
    }

    // Equations
    for(int i=0; i<numEquations; i++){

        line = fgets(buffer, bufferSize, fstream);
        int eq_number = atoi(strtok(line,","));
        (void) eq_number;
        int eq_type = atoi(strtok(NULL,","));
        double eq_scalar = atof(strtok(NULL,","));

        Equation::EquationType type;
        type = Equation::EQ;

        if(eq_type == 0){
            type = Equation::EQ;
        } else if (eq_type == 1){
            type = Equation::GE;
        } else if (eq_type ==2){
            type = Equation::LE;
        }

        Equation equation(type);
        equation.setScalar(eq_scalar);
        //printf("Equation No: %u, Type: %d, Scalar: %f\n", eq_number, eq_type, eq_scalar);
        auto token_v = strtok(NULL,",");
        auto token_c = strtok(NULL,",");
        while(token_v != NULL){
            int var_no = atoi(token_v);
            double coeff = atof(token_c);
            //printf("Var:%u \t coeff:%f\n",var_no,coeff);
            //int
            equation.addAddend(coeff, var_no);
            token_v = strtok(NULL, ",");
            token_c = strtok(NULL, ",");
        }
        inputquery.addEquation(equation);
    }

    // Constraints
    for(int i=0; i<numConstraints; i++){
        line = fgets(buffer, bufferSize, fstream);
        int co_no = atoi(strtok(line,","));
        (void) co_no;
        String co_type = String(strtok(NULL,","));
        //printf("C_no: %u\t Type:%s\n", co_no,co_type.ascii());
        String serial_constraint = String(strtok(NULL,""));
        if(co_type == "relu"){
            PiecewiseLinearConstraint* r = new ReluConstraint(serial_constraint);
            inputquery.addPiecewiseLinearConstraint(r);
        } else if (co_type == "max"){
            PiecewiseLinearConstraint* r = new MaxConstraint(serial_constraint);
            inputquery.addPiecewiseLinearConstraint(r);
        }
    }

    //printf(Stringf( "Num Variables:%u", numVars).ascii());
    return inputquery;
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
