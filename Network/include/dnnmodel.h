#ifndef DNNMODEL_H
#define DNNMODEL_H

#include <torch/torch.h>

struct DNNModel : torch::nn::Module{
    private:
        //Input layer
        torch::nn::BatchNorm1d inNormLayer{nullptr};
        torch::nn::Linear inputLayer{nullptr};
        torch::nn::Tanh reluInLayer{nullptr};

        //Hidden layer
        std::vector<torch::nn::BatchNorm1d> normLayers;
        std::vector<torch::nn::Linear> hiddenLayers;
        std::vector<torch::nn::Tanh> activationLayers;
        std::vector<torch::nn::Dropout> dropLayers;

        //Output layer
        torch::nn::BatchNorm1d outNormLayer{nullptr};
        torch::nn::Linear outLayer{nullptr};

        //Other stuff
        int nInput;
        int nNodes;
        float dropOut;
        torch::Device device;
        bool isParametrized;

        std::ostringstream modelSummary;
        
    public:
        DNNModel(const int& nInput, const int& nNodes, const int& nHidden, const float& dropOut, const bool& isParametrized, const int& nClasses, torch::Device& device);
        torch::Tensor forward(const torch::Tensor& input, const torch::Tensor& chargedMasses, const torch::Tensor& neutralMasses, const bool& predict = false);
        void Print();
        int GetNWeights();
};

#endif
