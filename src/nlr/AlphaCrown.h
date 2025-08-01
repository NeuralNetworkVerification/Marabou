#ifndef ALPHACROWN_H
#define ALPHACROWN_H

#include "CustomDNN.h"
#include "LayerOwner.h"
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
    void run()

    {
        findBounds();
        updateBounds(_initialAlphaSlopes);
        optimizeBounds( 2 );

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
    std::vector<Layer::Type> _layersOrder;
    std::map<unsigned, torch::Tensor> _positiveWeights;
    std::map<unsigned, torch::Tensor> _negativeWeights;
    std::map<unsigned, torch::Tensor> _biases;
    std::map<unsigned, unsigned> _indexAlphaSlopeMap;
    std::map<unsigned, unsigned> _linearIndexMap;

    std::map<unsigned, torch::Tensor> _upperRelaxationSlopes;
    std::map<unsigned, torch::Tensor> _upperRelaxationIntercepts;

    std::vector<torch::Tensor> _initialAlphaSlopes;

    torch::Tensor createSymbolicVariablesMatrix();
    void relaxReluLayer(unsigned layerNumber, torch::Tensor &EQ_up, torch::Tensor &EQ_low);
    void computeWeightedSumLayer(unsigned i, torch::Tensor &EQ_up, torch::Tensor &EQ_low);
    void computeReluLayer(unsigned i, torch::Tensor &EQ_up, torch::Tensor &EQ_low, std::vector<torch::Tensor> &alphaSlopes);

    void updateBounds(std::vector<torch::Tensor> &alphaSlopes);
    void updateBoundsOfLayer(unsigned layerIndex, torch::Tensor &upBounds, torch::Tensor &lowBounds);

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


#endif //ALPHACROWN_H