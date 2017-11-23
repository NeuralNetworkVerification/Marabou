/*********************                                                        */
/*! \file TfNnet.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Kyle Julian
 ** This file is part of the Marabou project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/



#include "TfNnet.h"
#include "tensorflow/core/public/session.h"
#include "tensorflow/core/platform/env.h"
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/framework/graph_def_util.h"
#include "tensorflow/core/protobuf/meta_graph.pb.h"

using namespace tensorflow;

// Remove /read from variable names
string cleanName(string name){
    size_t pos =0 ;
    if ((pos = name.find("/read")) != std::string::npos) {
        return name.substr(0,pos);
    }
    return name;
}

// Print the data of the network variables
int printVariables(std::map<string, Eigen::Tensor<float,2>>& varValuesMatrix, std::map<string, Eigen::Tensor<float,1>>& varValuesVector) {
    
    string ret;
    ret = "VARIABLE DATA:\nWeights:\n";
    for (auto& x : varValuesMatrix){
        strings::StrAppend(&ret,x.first);
        strings::StrAppend(&ret,":\n");
        
        int dim1 =x.second.dimension(0);
        int dim2 =x.second.dimension(1);
        for (int j=0; j<dim1; j++) {
            for (int k=0; k<dim2; k++) {
                strings::StrAppend(&ret,x.second(j,k));
                strings::StrAppend(&ret,", ");
            }
            strings::StrAppend(&ret,"\n");
        }
        strings::StrAppend(&ret,"\n");
    }
    strings::StrAppend(&ret,"\nBiases:\n");
    for (auto& x : varValuesVector){
        strings::StrAppend(&ret,x.first);
        strings::StrAppend(&ret,":\n");
        
        int dim1 =x.second.dimension(0);
        for (int j=0; j<dim1; j++) {
            strings::StrAppend(&ret,x.second(j));
            strings::StrAppend(&ret,", ");
        }
        strings::StrAppend(&ret,"\n\n");
        
    }
    std::cout<<ret;
    return 0;   
}

// Print the network weights and biases. This is after the network operations have been read and placed in the correct order
int printVariables(int num_inputs, std::vector<Eigen::Tensor<float,2>>& weights, std::vector<Eigen::Tensor<float,1>>& biases, std::vector<string> activations) {
    
    string ret;
    ret = "ORDERED VARIABLE DATA:\nNum Inputs: " + std::to_string(num_inputs) + "\nWeights:\n";
    for (unsigned i=0; i<weights.size(); i++){
        strings::StrAppend(&ret,"Weight " + std::to_string(i) + ":\n");        
        int dim1 =weights[i].dimension(0);
        int dim2 =weights[i].dimension(1);
        for (int j=0; j<dim1; j++) {
            for (int k=0; k<dim2; k++) {
                strings::StrAppend(&ret,weights[i](j,k));
                strings::StrAppend(&ret,", ");
            }
            strings::StrAppend(&ret,"\n");
        }
        strings::StrAppend(&ret,"\n");
    }
    strings::StrAppend(&ret,"\nBiases:\n");
    for (unsigned i=0; i<biases.size(); i++){ 
        strings::StrAppend(&ret,"Bias " + std::to_string(i) + ":\n");
        int dim1 =biases[i].dimension(0);
        for (int j=0; j<dim1; j++) {
            strings::StrAppend(&ret,biases[i](j));
            strings::StrAppend(&ret,", ");
        }
        strings::StrAppend(&ret,"\n\n");
        
    }
    strings::StrAppend(&ret,"\nActivations:\n");
    for (unsigned i=0; i<activations.size(); i++){
        strings::StrAppend(&ret,"Activation " + std::to_string(i) + ": " + activations[i] + "\n");    
    }
    
    std::cout<<ret;
    return 0;   
}

// Extract data from the tensors, extracted to an Eigen Tensor
int convertTensorData(Session* session, std::vector<string> varNames, std::map<string, Eigen::Tensor<float,2>>& varValuesMatrix, std::map<string, Eigen::Tensor<float,1>>& varValuesVector) {
    
    // Extract values of variables
    std::vector<Tensor> outputs;  
    session->Run({},varNames,{},&outputs);

    for (unsigned i=0; i < varNames.size(); i++){
        if (outputs[i].dims()==1){
            int dim1 =outputs[i].dim_size(0);
            Eigen::Tensor<float,1> temp(dim1);
            auto output = outputs[i].vec<float>();
            for (int j=0; j<dim1; j++) {
                temp(j) = output(j);
            }
            varValuesVector[varNames[i]] = temp;
        } else if (outputs[i].dims()==2){
            int dim1 =outputs[i].dim_size(0);
            int dim2 =outputs[i].dim_size(1);
            Eigen::Tensor<float,2> temp(dim1,dim2);
            auto output = outputs[i].matrix<float>();
            for (int j=0; j<dim1; j++) {
                for (int k=0; k<dim2; k++) {
                    temp(j,k) = output(j,k);
                }
            }
            varValuesMatrix[varNames[i]] = temp;
        } else {
            std::cout<<"ERROR! Could not extract data from variable";
            return 1;
        }
    }
    return 0;   
}


// Function for loading a frozen pb network produced by tensorflow
TfNnet *load_network(const char* filename) {
  // Initialize a tensorflow session
  Session* session;
  Status status = NewSession(SessionOptions(), &session);
  if (!status.ok()) {
    std::cout << status.ToString() << "\n";
    return NULL;
  }

  // Read in the protobuf graph we exported
  // (The path seems to be relative to the cwd. Keep this in mind
  // when using `bazel run` since the cwd isn't where you call
  // `bazel run` but from inside a temp folder.)
  GraphDef graph_def;
  status = ReadBinaryProto(Env::Default(), filename, &graph_def);
  if (!status.ok()) {
    std::cout << status.ToString() << "\n";
    std::cout << "ERROR! Could not load network.\n";
    return NULL;
  }

  // Add the graph to the session
  status = session->Create(graph_def);
  if (!status.ok()) {
    std::cout << status.ToString() << "\n";
    return NULL;
  }
  TfNnet *nnet = new TfNnet(); 
  // Define imoprtant variables
  std::vector<Tensor> outputs;  
  std::vector<string> varNames;
  std::vector<int>    varDims;
  int node_count = graph_def.node_size();
    
  // Loop through all nodes in graph and find the names of variables and placeholders. 
  // The placeholders are where the user can feed in information for training (inputs or outputs)
  // The variables are the network weights/biases/etc that we are interested in
  
  //std::cout << SummarizeGraphDef(graph_def); //This line prints out all nodes in the graph
  std::cout << "VARIABLES:\n";
  for (int i=0; i < node_count; i++) {
    auto n = graph_def.node(i);
    if (n.op().find("Const") != std::string::npos) {
      varNames.push_back(n.name());
      std::cout << n.name();
      std::cout << "\n";
    } else if (n.op().find("Placeholder") != std::string::npos) {
      nnet->input_names.push_back(n.name());
    }
  }
  std::cout << "\n";  
   
  // Extract data from tensorflow tensors  
  std::map<string, Eigen::Tensor<float,2>> varValuesMatrix;
  std::map<string, Eigen::Tensor<float,1>> varValuesVector;
  std::cout << "HERE 1\n";
  convertTensorData(session,varNames,varValuesMatrix,varValuesVector);
  printVariables(varValuesMatrix,varValuesVector);
  std::cout << SummarizeGraphDef(graph_def);  
    
  std::cout << "VARIABLE NAMES:\n";
  for (unsigned j=0; j<varNames.size(); j++){
      std::cout << varNames[j];
      std::cout << "\n";
  }
  std::cout << "\n";

  // Loop through graph and order weights and biases correctly for a dense ReLU network  
  // We assume that the graph only uses:
  //   MatMul
  //   Add
  //   Relu
  // Using networks with more operations means this section must be changed, and other functions
  // might need updates as well.
  nnet->num_inputs = -1;
  for (int i = 0; i < node_count; i++) {
    auto n = graph_def.node(i);
    if (n.op().find("MatMul") != std::string::npos) {
      string input1 = n.input()[0];
      string input2 = n.input()[1];
      int num_Placeholders = 0;
      int num_Variables   = 0;
      for (const string& input : n.input()) {
        string inputClean = cleanName(input);
        for (unsigned j=0; j<nnet->input_names.size(); j++){
            if (nnet->input_names[j].compare(inputClean)==0){
                num_Placeholders++;
            }
        }
        for (unsigned j=0; j<varNames.size(); j++){
            if (varNames[j].compare(inputClean)==0){
                num_Variables++;
                nnet->weights.push_back(varValuesMatrix[inputClean]);
            }
        }
      }

      // We assume that only 1 user defined varaible is used in an operation
      if (num_Variables>1) {
          std::cout << "Error: 2 variables used in this matrix multiplication!";
          return NULL;
      }
    } else if (n.op().find("Add") != std::string::npos) {
        string input1 = n.input()[0];
        string input2 = n.input()[1];
        int num_Placeholders = 0;
        int num_Variables   = 0;
        for (const string& input : n.input()) {
            string inputClean = cleanName(input);
            for (unsigned j=0; j<nnet->input_names.size(); j++){
                if (nnet->input_names[j].compare(inputClean)==0){
                    num_Placeholders++;
                }
            }
            for (unsigned j=0; j<varNames.size(); j++){
                if (varNames[j].compare(inputClean)==0){
                    num_Variables++;
                    nnet->biases.push_back(varValuesVector[inputClean]);
                }
            }
        }
        if (num_Variables>1) {
            std::cout << "Error: 2 variables used in this addition!\n";
            std::cout << num_Variables;
            std::cout << "\n";
            return NULL;
        }
    } else if (n.op().find("Relu") != std::string::npos) {
        nnet->activations.push_back("Relu");
    }
  }

  // The last variable in the graph is the output variable
  nnet->output_name= graph_def.node(node_count-1).name();

  // Save important network attributes 
  nnet->num_inputs = nnet->weights[0].dimension(0);
  nnet->num_layers = nnet->weights.size();
  nnet->num_outputs= nnet->weights[(nnet->num_layers) -1].dimension(1);

  nnet->layer_sizes= new int[(nnet->num_layers)+1];
  for (int i=0; i<nnet->num_layers; i++){
    nnet->layer_sizes[i] = nnet->weights[i].dimension(0); 
  }
  nnet->layer_sizes[nnet->num_layers] = nnet->num_outputs;
  nnet->session = session;
  
  // Print out Variables of network, if desired
  printVariables(nnet->num_inputs,nnet->weights,nnet->biases,nnet->activations);

  return nnet;
}

int evaluate_network(TfNnet *network, double *input, double *output, bool normInputs, bool normOutputs)
{
    if (network==NULL) {
        return 0;
    }
    // Create an input Tensor and populate with values from input 
    Tensor inputTensor(DT_FLOAT, TensorShape({1,network->num_inputs}));    
    auto input_map = inputTensor.tensor<float, 2>();
    for (int i=0; i<network->num_inputs; i++)
    {
        input_map(0,i) = (float) (input)[i];
    }

    // Prepare the inputs to the Run method of session
    // Outputs are stored as a vector of tensors
    // Inputs are given as pairs of variable names and tensor values
    std::vector<Tensor> outputTensor;
    std::vector<std::pair<std::string, Tensor>> inputRun = {
        {network->input_names[0],inputTensor},
    };

    // Run the session to get the output tensors
    network->session->Run(inputRun,{network->output_name},{},&outputTensor);
    
    // If the outputTensor is empty, then there was an error computing the output!
    if (outputTensor.size()==0){
        printf("ERROR! Could not cmopute network output given inputs!\n");
        return 0;
    }

    // Populate the output array with the values from the tensor
    auto outputVector = outputTensor[0].matrix<float>();
    for (int i=0; i<network->num_outputs; i++) {
        output[i] = outputVector(0,i);
    }
    return 1;
}

// Methods for getting data about the neural network
int num_inputs(TfNnet *network)
{
    if (network == NULL) {
        return -1;
    }
    return network->num_inputs;
}

int num_outputs(TfNnet *network)
{
    if (network == NULL) {
        return -1;
    }
    return network->num_outputs;
}

int num_layers(TfNnet *network)
{
    if (network == NULL) {
        return -1;
    }
    return network->num_layers;
}

// Destroy network
void destroy_network(TfNnet *network)
{
    network->session->Close();
    delete(network->layer_sizes);
    delete(network);
}
