/*
* SCRIPT DESCRIPTION
* This script helps define neural networks in marabou format for direct cpp integration
*/
    /* class representing general Marabou network

    Attributes:
        numVars (int): Total number of variables to represent network
        equList (list of :class:`~maraboupy.Equation`): Network equations
        reluList (list of tuples): List of relu constraint tuples, where each tuple contains the backward and forward variables
        sigmoidList (list of tuples): List of sigmoid constraint tuples, where each tuple contains the backward and forward variables
        maxList (list of tuples): List of max constraint tuples, where each tuple conatins the set of input variables and output variable
        absList (list of tuples): List of abs constraint tuples, where each tuple conatins the input variable and the output variable
        signList (list of tuples): List of sign constraint tuples, where each tuple conatins the input variable and the output variable
        lowerBounds (Dict[int, float]): Lower bounds of variables
        upperBounds (Dict[int, float]): Upper bounds of variables
        inputVars (list of numpy arrays): Input variables
        outputVars (numpy array): Output variables
    */


    /*

 TODO:
    - discuss self and implement properly
        - currently just return updated of type (depends on usage)
    - change C types to Marabou types.

 */

#ifndef __NetworkParser_h__
#define __NetworkParser_h__

#include "Map.h"
#include "List.h"
#include "Vector.h"
#include "Equation.h"
#include "InputQuery.h"
#include "ReluConstraint.h"
#include "DisjunctionConstraint.h"
#include "MaxConstraint.h"
#include "PiecewiseLinearConstraint.h"
#include "SigmoidConstraint.h"
#include "SignConstraint.h"
#include "TranscendentalConstraint.h"
#include <utility>

typedef unsigned int Variable;
typedef Vector<int> TensorShape;

class NetworkParser {
private:
    unsigned int _numVars;

protected:
    List<Variable> _inputVars;
    List<Variable> _outputVars;

    Vector<Equation> _equationList;
    List<ReluConstraint*> _reluList;
    List<SigmoidConstraint*> _sigmoidList;
    List<MaxConstraint*> _maxList;
    List<AbsoluteValueConstraint*> _absList;
    List<SignConstraint*> _signList;
    Map<Variable,float> _lowerBounds;
    Map<Variable,float> _upperBounds;

    NetworkParser();
    void initNetwork();

    void addEquation( Equation &eq );
    void setLowerBound( Variable var, float value );
    void setUpperBound( Variable var, float value );
    void addRelu( Variable var1, Variable var2 );
    void addSigmoid( Variable var1, Variable var2 );
    void addSignConstraint( Variable var1, Variable var2 );
    void addMaxConstraint( Variable maxVar, Set<Variable> elements );
    void addAbsConstraint( Variable var1, Variable var2 );

    Variable getNewVariable();
    void getMarabouQuery( InputQuery& query );

    int findEquationWithOutputVariable( Variable variable );
};

#endif // __NetworkParser_h__