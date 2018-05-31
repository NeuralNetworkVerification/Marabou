#include "QueryLoader.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include "MStringf.h"
#include "InputQuery.h"
#include "Equation.h"
#include "ReluConstraint.h"
#include "MaxConstraint.h"

InputQuery load_query(const char* filename){
    InputQuery inputquery;
    FILE *fstream = fopen(filename,"r");
    if (fstream == NULL)
    {
        return inputquery;
    }

    int bufferSize = 10240;
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

    printf("Number of variables: %u\n", numVars);
    printf("Number of bounds: %u\n", numBounds);
    printf("Number of equations: %u\n", numEquations);
    printf("Number of constraints: %u\n", numConstraints);

    inputquery.setNumberOfVariables(numVars);

    // bounds
    for (int i=0; i<numBounds; i++){
        //first bound
        line=fgets(buffer,bufferSize,fstream);
        int var_to_bound = atoi(strtok(line,","));
        float l = atof(strtok(NULL,","));
        float u = atof(strtok(NULL,","));

        printf("Var: %u L:%f, U:%f\n", var_to_bound,l,u);

        inputquery.setLowerBound(var_to_bound, l);
        inputquery.setUpperBound(var_to_bound, u);
    }

    // Equations
    for(int i=0; i<numEquations; i++){
        
        line = fgets(buffer, bufferSize, fstream);
        int eq_number = atoi(strtok(line,","));
        int eq_type = atoi(strtok(NULL,","));
        float eq_scalar = atof(strtok(NULL,","));

        Equation::EquationType type;
        switch(eq_type) {
            case 0 : type = Equation::EQ;
            case 1 : type = Equation::GE;
            case 2 : type = Equation::LE;
        }
        Equation equation(type);
        equation.setScalar(eq_scalar);
        printf("Equation No: %u, Type: %u, Scalar: %f\n", eq_number, eq_type, eq_scalar);
        auto token_v = strtok(NULL,",");
        auto token_c = strtok(NULL,",");
        while(token_v != NULL){
            int var_no = atoi(token_v);
            float coeff = atof(token_c);
            printf("Var:%u \t coeff:%f\n",var_no,coeff);
            //int
            equation.addAddend(coeff, var_no);
            token_v = strtok(NULL, ",");
            token_c = strtok(NULL, ",");
        }
        inputquery.addEquation(equation);
    }

    // Constraints
    for(int i=0; i<5; i++){
        line = fgets(buffer, bufferSize, fstream);
        int co_no = atoi(strtok(line,","));
        String co_type = String(strtok(NULL,","));
        printf("C_no: %u\t Type:%s\n", co_no,co_type.ascii());

        if(co_type == "relu"){
            // Relu Constraint
            String serial_relu = String(strtok(NULL,""));
            printf("serial relu: %s", serial_relu.ascii());

            ReluConstraint relu(serial_relu);
            printf("NEW SERIAL:\n%s\n", relu.serializeToString().ascii());

            PiecewiseLinearConstraint* r = new ReluConstraint(serial_relu);
            inputquery.addPiecewiseLinearConstraint(r);
        } else if (co_type == "max"){
            // Max Constraint
        }
    }

    //printf(Stringf( "Num Variables:%u", numVars).ascii());    
    return inputquery;
}