
#ifndef _ALPHACROWN_H_
#define _ALPHACROWN_H_

#include "CustomDNN.h"
#include "LayerOwner.h"
#include "NetworkLevelReasoner.h"
#include <vector>

#undef Warning
#include <torch/torch.h>

namespace NLR {
class AlphaCrown
{
public:
    AlphaCrown( LayerOwner *layerOwner );

    void findBounds();
    void optimizeBounds( int loops = 50 );
    void run(){
        findBounds();
        optimizeBounds(2);
    }

private:
    LayerOwner *_layerOwner;
    CustomDNN *_network;
    void GDloop( int loops, const std::string val_to_opt, std::vector<torch::Tensor> &alphaSlopes );
    std::tuple<torch::Tensor, torch::Tensor>
    computeBounds( std::vector<torch::Tensor> &alphaSlopes );
    int _inputSize;
    torch::Tensor _lbInput;
    torch::Tensor _ubInput;

    std::vector<torch::nn::Linear> _linearLayers;
    std::vector<torch::Tensor> _positiveWeights;
    std::vector<torch::Tensor> _negativeWeights;
    std::vector<torch::Tensor> _biases;

    std::vector<torch::Tensor> _upperRelaxationSlopes;
    std::vector<torch::Tensor> _upperRelaxationIntercepts;

    std::vector<torch::Tensor> _alphaSlopes;

    torch::Tensor createSymbolicVariablesMatrix();

    static torch::Tensor addVecToLastColumnValue( const torch::Tensor &matrix,
                                                  const torch::Tensor &vec )
    {
        auto result = matrix.clone();
        result.slice( 1, result.size( 1 ) - 1, result.size( 1 ) ) += vec.unsqueeze( 1 );
        return result;
    }
    static torch::Tensor lower_ReLU_relaxation( const torch::Tensor &u, const torch::Tensor &l );

    static std::tuple<torch::Tensor, torch::Tensor> upper_ReLU_relaxation( const torch::Tensor &u,
                                                                           const torch::Tensor &l );

    torch::Tensor getMaxOfSymbolicVariables( const torch::Tensor &matrix );
    torch::Tensor getMinOfSymbolicVariables( const torch::Tensor &matrix );


    void log( const String &message );
};
} // namespace NLR


#endif //_ALPHACROWN_H_
